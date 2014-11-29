//
// apa.io.c
//
// APA I/O node
//
// set lfuse to 0x7E for 20 MHz xtal
//
// Neil Gershenfeld
// CBA MIT 7/3/11
//
// (c) Massachusetts Institute of Technology 2011
// Permission granted for experimental and personal use;
// license for commercial sale available from MIT.
//

#include <apa.h>
#include <stdbool.h>

#define PWM_pin (1 << PA5)
#define PWM_port PORTA
#define PWM_direction  DDRA

//#define BLINK PINA = _BV(5)

volatile int16_t position = 0;
volatile bool position_changed = false;
static volatile uint8_t state = 0;

unsigned char path_to_host[apa_max_packet];
unsigned char path_to_host_length = 0;
bool path_to_host_stored = false;

void blink3(){
   unsigned char i = 0;
   for (i; i <3; i++){
      PORTA |= _BV(PA5); // turn LED ON
      _delay_ms(100);  // wait 1 sec
      PORTA &= ~(_BV(PA5)); // turn LED OFF
      _delay_ms(100);
   }
}

//
// process the packet
//
void apa_process_packet(struct apa_port_type *port) {
   uint16_t pwm; //,ad;
   unsigned char pwml, pwmh, i; //, adl, adh;
   //
   // execute command
   //
   switch (port->payload_out[0]) {
      case 'u':   // u: PWM up
         pwm = OCR1B;
         pwm += -1;
         OCR1B = pwm;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'd':   // d: PWM down
         pwm = OCR1B;
         pwm += 1;
         OCR1B = pwm;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'n':   // n: PWM on
         OCR1B = 0;
         pwmh = 0;
         pwml = 0;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'f':   // f: PWM off
         OCR1B = 1023;
         pwmh = 3;
         pwml = 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'r':   // r: PWM read
         pwm = OCR1B;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'w':   // w: PWM write
         pwm = apa_hex_int(&port->payload_out[1]);
         OCR1B = pwm;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'p': // save path to host for future usage
         for (i = 0; i < port->path_out_length; ++i)
            path_to_host[i] = port->path_out[i];
            //path_to_host[i] = '0';
         path_to_host_length = port->path_out_length;
         port->payload_out[0] = port->id;
         port->payload_out_length = 1;
         path_to_host_stored = true;
         blink3();
         break;

      case ' ':   // do nothing
         port->path_out_length = 0;
         port->payload_out_length = 0;
      default:    // unknown command
         port->payload_out[0] = '?';
         port->payload_out_length = 1;
         break;
      }
   }

//
// Configure interrupt pins
//
void configInt(void){
   GIMSK = _BV(PCIE0);    // turns on pin change interrupts
   PCMSK0 = _BV(PCINT2) | _BV(PCINT3);    // turn on interrupts on pins PCINT2 PCINT3
   sei();                 // enables interrupts
}

//
// ISR - interrupt service routine
//
ISR (PCINT0_vect){
   updateEncoder();
}

//
// increment/decrement position variable
//
void updateEncoder(void){

   //                           _______         _______       
   //               Pin1 ______|       |_______|       |______ Pin1
   // negative <---         _______         _______         __      --> positive
   //               Pin2 __|       |_______|       |_______|   Pin2

   // new   new   old   old
   // pin2  pin1  pin2  pin1  Result
   // ----  ----  ----  ----  ------
   // 0  0  0  0  no movement
   // 0  0  0  1  +1
   // 0  0  1  0  -1
   // 0  0  1  1  +2  (assume pin1 edges only)
   // 0  1  0  0  -1
   // 0  1  0  1  no movement
   // 0  1  1  0  -2  (assume pin1 edges only)
   // 0  1  1  1  +1
   // 1  0  0  0  +1
   // 1  0  0  1  -2  (assume pin1 edges only)
   // 1  0  1  0  no movement
   // 1  0  1  1  -1
   // 1  1  0  0  +2  (assume pin1 edges only)
   // 1  1  0  1  -1
   // 1  1  1  0  +1
   // 1  1  1  1  no movement

   uint8_t s = state & 3;
   uint8_t reading = PINA;
   if (reading & _BV(PA2)) s |= 4; // put '1' into the 3rd bit - 01**
   if (reading & _BV(PA3)) s |= 8; // put '1' into the 4th bit - 10**

   switch (s) {
      case 0: case 5: case 10: case 15:
         break;
      case 1: case 7: case 8: case 14:
         position = 1; position_changed = true; break;
      case 2: case 4: case 11: case 13:
         position = 2; position_changed = true; break;
      case 3: case 12:
         position = 3; position_changed = true; break;
      default:
         position = 4; position_changed = true; break;
   }
   state = (s >> 2); // shift two bits left to keep the old ones XXXX => **XX
}
//
// send out encoder position
//

void apa_encoder_check(struct apa_port_type *port) {

   unsigned char i;

   if (position_changed && path_to_host_stored){
      
      position_changed = false;

      for (i = 0; i < path_to_host_length; ++i)
         port->path_out[i] = path_to_host[i];
      port->path_out_length = path_to_host_length;

      port->payload_out[0] = position;
      port->payload_out_length = 1;

      //BLINK;

   } else {
      //PORTA &= ~(_BV(PA5)); // turn LED OFF
   }
}

//
// main
//

int main(void) {
   static struct apa_port_type port_0, port_1; //, port_2;
   //
   // set clock divider to /1
   //
   CLKPR = (1 << CLKPCE);
   CLKPR = (0 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);

   //
   // using blue led to debug encoder readouts
   // 
   DDRA |= _BV(PA5); // set pin PA5 as an output
   PORTA |= _BV(PA5); // turn LED ON
   _delay_ms(1000);  // wait 1 sec
   PORTA &= ~(_BV(PA5)); // turn LED OFF

   //
   // set up PWM pins
   //
   // TCCR1A = ((1 << COM1B1) | (1 << COM1B0) | (1 << WGM11)
   //    | (1 << WGM10)); // clear OC1B on compare match, fast 8-bit PWM
   // TCCR1B = ((0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11)
   //    | (1 << CS10)); // /1 clock divider
   // OCR1B = 1023;
   // clear(PWM_port, PWM_pin);
   // output(PWM_direction, PWM_pin);

   //
   // initialize ports and pins
   //
   port_0.pins_in = &PINA;
   port_0.port_out = &PORTB;
   port_0.direction_out = &DDRB;
   port_0.pin_in = (1 << PA7);
   port_0.pin_out = (1 << PB2);
   port_0.path_in_length = 0;
   port_0.path_out_length = 0;
   port_0.id = '0';
   port_0.next_port = &port_1;
   clear(*port_0.port_out, port_0.pin_out);
   output(*port_0.direction_out, port_0.pin_out);
   //
   port_1.pins_in = &PINA;
   port_1.port_out = &PORTA;
   port_1.direction_out = &DDRA;
   port_1.pin_in = (1 << PA0);
   port_1.pin_out = (1 << PA1);
   port_1.path_in_length = 0;
   port_1.path_out_length = 0;
   port_1.id = '1';
   port_1.next_port = &port_0; /// !!!!!!!!!!!!!!!!!!!!!!!!!
   clear(*port_1.port_out, port_1.pin_out);
   output(*port_1.direction_out, port_1.pin_out);
   //
   // port_2.pins_in = &PINA;
   // port_2.port_out = &PORTA;
   // port_2.direction_out = &DDRA;
   // port_2.pin_in = (1 << PA4);
   // port_2.pin_out = (1 << PA6);
   // port_2.path_in_length = 0;
   // port_2.path_out_length = 0;
   // port_2.id = '2';
   // port_2.next_port = &port_0;
   // clear(*port_2.port_out, port_2.pin_out);
   // output(*port_2.direction_out, port_2.pin_out);
   
   //
   // power on delay
   //
   power_on_delay();

   blink3();

   //
   // configure interrupt pins
   //
   configInt();

   //
   // main loop
   //
   while (1) {
      apa_port_scan(&port_0);
      apa_port_scan(&port_1);
      //apa_port_scan(&port_2);
      apa_encoder_check(&port_0);

      //
      // check for sync start
      //
      // if (0 != pin_test(*port_1.pins_in, port_1.pin_in)) {
      //    apa_get_packet(&port_1);
      //    if (port_1.path_in_length != 0) {
      //       apa_move_packet(&port_1,&port_0);
      //       apa_put_packet(&port_0);
      //       }
      // }
      
   }
}
