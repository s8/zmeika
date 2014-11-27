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

#define PWM_pin (1 << PA5)
#define PWM_port PORTA
#define PWM_direction  DDRA

//
// process the packet
//
void apa_process_packet(struct apa_port_type *port) {
   uint16_t ad, pwm;
   unsigned char pwml, pwmh, adl, adh;
   //
   // execute command
   //
   switch (port->payload_out[0]) {
      case 'u':
         //
         // u: PWM up
         //
         pwm = OCR1B;
         pwm += -1;
         OCR1B = pwm;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'd':
         //
         // d: PWM down
         //
         pwm = OCR1B;
         pwm += 1;
         OCR1B = pwm;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'n':
         //
         // n: PWM on
         //
         OCR1B = 0;
         pwmh = 0;
         pwml = 0;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'f':
         //
         // f: PWM off
         //
         OCR1B = 1023;
         pwmh = 3;
         pwml = 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'r':
         //
         // r: PWM read
         //
         pwm = OCR1B;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case 'w':
         //
         // w: PWM write
         //   wxxxx writes 16-bit hex value
         //
         pwm = apa_hex_int(&port->payload_out[1]);
         OCR1B = pwm;
         pwmh = pwm >> 8;
         pwml = pwm & 255;
         port->payload_out[0] = pwmh;
         port->payload_out[1] = pwml;
         port->payload_out_length = 2;
         break;
      case '0':
         //
         // 0: read A/D input pin 0
         //
         ADMUX = (1 << REFS1) | (0 << REFS0) | (0 << MUX5) | (0 << MUX4) | (0 << MUX3) | (0 << MUX2) | (1 << MUX1) | (0 << MUX0); // PA2, Vcc ref

         ADCSRA |= (1<<ADSC); // read to settle
            while (!(ADCSRA&(1<<ADSC)));
         ad = ADCW;
         ADCSRA |= (1<<ADSC); // read and save
            while (!(ADCSRA&(1<<ADSC)));
         ad = ADCW;
         adh = ad >> 8;
         adl = ad & 255;
         port->payload_out[0] = adh;
         port->payload_out[1] = adl;
         port->payload_out_length = 2;
         break;
      case '2':
         //
         // 2: read A/D input pin 2
         //
         ADMUX = (1 << REFS1) | (0 << REFS0) | (0 << MUX5) | (0 << MUX4) | (0 << MUX3) | (0 << MUX2) | (1 << MUX1) | (1 << MUX0); // PA3, Vcc ref

         ADCSRA |= (1<<ADSC); // read to settle
            while (!(ADCSRA&(1<<ADSC)));
         ad = ADCW;
         ADCSRA |= (1<<ADSC); // read and save
            while (!(ADCSRA&(1<<ADSC)));
         
         ad = ADCW;
         adh = ad >> 8;
         adl = ad & 255;
         port->payload_out[0] = adh;
         port->payload_out[1] = adl;
         port->payload_out_length = 2;
         break;
      case ' ':
         //
         // do nothing
         //
         port->path_out_length = 0;
         port->payload_out_length = 0;
      default:
         //
         // unknown command
         //
         port->payload_out[0] = '?';
         port->payload_out_length = 1;
         break;
      }
   }

//
// main
//

int main(void) {
   static struct apa_port_type port_0, port_1, port_2;
   //
   // set clock divider to /1
   //
   CLKPR = (1 << CLKPCE);
   CLKPR = (0 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);
   //
   // initialize ADC
   //
   ADCSRB &= ~(1<<ADLAR); // right-adjust result
   ADCSRA |= (1<<ADEN); // enable ADC
   ADCSRA |= (1<<ADPS2); // set ADC prescalers to /128
   ADCSRA |= (1<<ADPS1); // "
   ADCSRA |= (1<<ADPS0); // "
   //
   // initialize PWM counter and pin
   //
   TCCR1A = ((1 << COM1B1) | (1 << COM1B0) | (1 << WGM11)
      | (1 << WGM10)); // clear OC1B on compare match, fast 8-bit PWM
   TCCR1B = ((0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11)
      | (1 << CS10)); // /1 clock divider
   OCR1B = 1023;
   clear(PWM_port, PWM_pin);
   output(PWM_direction, PWM_pin);
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
   port_1.next_port = &port_2;
   clear(*port_1.port_out, port_1.pin_out);
   output(*port_1.direction_out, port_1.pin_out);
   //
   port_2.pins_in = &PINA;
   port_2.port_out = &PORTA;
   port_2.direction_out = &DDRA;
   port_2.pin_in = (1 << PA4);
   port_2.pin_out = (1 << PA6);
   port_2.path_in_length = 0;
   port_2.path_out_length = 0;
   port_2.id = '2';
   port_2.next_port = &port_0;
   clear(*port_2.port_out, port_2.pin_out);
   output(*port_2.direction_out, port_2.pin_out);
   //
   // power on delay
   //
   power_on_delay();
   //
   // main loop
   //
   while (1) {
      apa_port_scan(&port_0);
      apa_port_scan(&port_1);
      apa_port_scan(&port_2);
      }
   }
