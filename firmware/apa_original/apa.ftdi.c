//
// apa.ftdi.c
//
// APA FTDI asynchronous bridge
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

//
// includes
//

#include <apa.h>

//
// defines
//

#define output(directions,pin) (directions |= pin) // set port direction for output
#define set(port,pin) (port |= pin) // set port pin
#define clear(port,pin) (port &= (~pin)) // clear port pin
#define serial_bit_delay_time 8.5 // bit delay for 115200 with overhead
//define serial_bit_delay_time 17 // bit delay for 57600 with overhead
#define serial_bit_delay() _delay_us(serial_bit_delay_time) // RS232 bit delay
#define serial_half_bit_delay() _delay_us(serial_bit_delay_time/2) // RS232 half bit delay
#define serial_char_delay() _delay_ms(10) // char delay

#define serial_port PORTA
#define serial_direction DDRA
#define serial_pins PINA
#define serial_pin_in (1 << PA1)
#define serial_pin_out (1 << PA2)

void serial_get_char(volatile unsigned char *pins, unsigned char pin, char *rxbyte) {
   //
   // read character into rxbyte on pins pin with timeout
   //    assumes line driver (inverts bits)
   //
   *rxbyte = 0;
   //
   // wait for start bit
   //
   while (1) {
      if (0 == pin_test(*pins,pin))
         break;
      }
   //
   // delay to middle of first data bit
   //
   serial_half_bit_delay();
   serial_bit_delay();
   //
   // unrolled loop to read data bits
   //
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 0);
   else
      *rxbyte |= (0 << 0);
   serial_bit_delay();
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 1);
   else
      *rxbyte |= (0 << 1);
   serial_bit_delay();
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 2);
   else
      *rxbyte |= (0 << 2);
   serial_bit_delay();
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 3);
   else
      *rxbyte |= (0 << 3);
   serial_bit_delay();
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 4);
   else
      *rxbyte |= (0 << 4);
   serial_bit_delay();
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 5);
   else
      *rxbyte |= (0 << 5);
   serial_bit_delay();
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 6);
   else
      *rxbyte |= (0 << 6);
   serial_bit_delay();
   if pin_test(*pins,pin)
      *rxbyte |= (1 << 7);
   else
      *rxbyte |= (0 << 7);
   //
   // wait for stop bit
   //
   serial_bit_delay();
   serial_half_bit_delay();
   }

void serial_put_char(volatile unsigned char *port, unsigned char pin, char txchar) {
   //
   // send character in txchar on port pin
   //    assumes line driver (inverts bits)
   //
   // start bit
   //
   clear(*port,pin);
   serial_bit_delay();
   //
   // unrolled loop to write data bits
   //
   if bit_test(txchar,0)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   if bit_test(txchar,1)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   if bit_test(txchar,2)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   if bit_test(txchar,3)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   if bit_test(txchar,4)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   if bit_test(txchar,5)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   if bit_test(txchar,6)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   if bit_test(txchar,7)
      set(*port,pin);
   else
      clear(*port,pin);
   serial_bit_delay();
   //
   // stop bit
   //
   set(*port,pin);
   serial_bit_delay();
   //
   // char delay
   //
   serial_bit_delay();
   }

void apa_get_serial_packet(struct apa_port_type *port) {
   //
   // get packet on port
   //
   char c;
   unsigned char cnt;
   //
   // find start
   //
   cnt = apa_timeout;
   do {
      serial_get_char(&serial_pins, serial_pin_in, &c);
      cnt -= 1;
      if (cnt == 0) {
         port->path_in_length = 0;
         return;
         }
      } while (c != apa_start);
   //
   // get path
   //
   port->path_in_length = 0;
   cnt = apa_max_packet;
   do {
      serial_get_char(&serial_pins, serial_pin_in, &c);
      cnt -= 1;
      if (cnt == 0) {
         port->path_in_length = 0;
         return;
         }
      port->path_in[port->path_in_length] = c;
      port->path_in_length += 1;
      } while (c != apa_divider);
   port->path_in_length -= 1;
   //
   // get payload
   //
   port->payload_in_length = 0;
   cnt = apa_max_packet;
   do {
      //
      // get next char
      //
      serial_get_char(&serial_pins, serial_pin_in, &c);
      cnt -= 1;
      if (cnt == 0) {
         port->path_in_length = 0;
         return;
         }
      //
      // check for escape
      //
      if (c == apa_escape) {
         //
         // get escaped char
         //
         serial_get_char(&serial_pins, serial_pin_in, &c);
         cnt -= 1;
         if (cnt == 0) {
            port->path_in_length = 0;
            return;
            }
         port->payload_in[port->payload_in_length] = c;
         port->payload_in_length += 1;
         //
         // get next char
         //
         serial_get_char(&serial_pins, serial_pin_in, &c);
         cnt -= 1;
         if (cnt == 0) {
            port->path_in_length = 0;
            return;
            }
         }
      //
      // save char
      //
      port->payload_in[port->payload_in_length] = c;
      port->payload_in_length += 1;
      } while (c != apa_end);
   port->payload_in_length -= 1;
   }

void apa_put_serial_packet(struct apa_port_type *port) {
   //
   // put packet on port
   //
   unsigned char i;
   char c;
   //
   // put start
   //
   serial_put_char(&serial_port, serial_pin_out, apa_start);
   //
   // put path
   //
   for (i = 0; i < port->path_out_length; ++i)
      serial_put_char(&serial_port, serial_pin_out, port->path_out[i]);
   port->path_out_length = 0;
   //
   // put divider
   //
   serial_put_char(&serial_port, serial_pin_out, apa_divider);
   //
   // put payload
   //
   for (i = 0; i < port->payload_out_length; ++i) {
      //
      // get next char
      //
      c = port->payload_out[i];
      //
      // check if escape needed
      //
      if ((c == apa_end) || (c == apa_escape))
         //
         // yes, put escape
         //
         serial_put_char(&serial_port, serial_pin_out, apa_escape);
      //
      // put char
      //
      serial_put_char(&serial_port, serial_pin_out, c);
      }
   port->payload_out_length = 0;
   //
   // put end
   //
   serial_put_char(&serial_port, serial_pin_out, apa_end);
   }

//
// process the packet
//
void apa_process_packet(struct apa_port_type *port) {
   ;
   }

//
// main
//

int main(void) {
   static struct apa_port_type port_0, port_1;
   //
   // set clock divider to /1
   //
   CLKPR = (1 << CLKPCE);
   CLKPR = (0 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);
   //
   // initialize asynchronous APA port 0
   //
   port_0.id = '0';
   port_0.path_in_length = 0;
   port_0.path_out_length = 0;
   port_0.next_port = &port_1;
   set(serial_port, serial_pin_out);
   output(serial_direction, serial_pin_out);
   //
   // initialize synchronous APA port 1
   //
   port_1.pins_in = &PINA;
   port_1.port_out = &PORTB;
   port_1.direction_out = &DDRB;
   port_1.pin_in = (1 << PA7);
   port_1.pin_out = (1 << PB2);
   port_1.id = '1';
   port_1.path_in_length = 0;
   port_1.path_out_length = 0;
   port_1.next_port = &port_0;
   clear(*port_1.port_out, port_1.pin_out);
   output(*port_1.direction_out, port_1.pin_out);
   //
   // power on delay
   //
   power_on_delay();
   //
   // main loop
   //
   while (1) {
      //
      // check for async start
      //
      if (0 == pin_test(serial_pins, serial_pin_in)) {
         apa_get_serial_packet(&port_0);
         if (port_0.path_in_length != 0) {
            apa_move_packet(&port_0,&port_1);
            apa_put_packet(&port_1);
            }
         }
      //
      // check for sync start
      //
      if (0 != pin_test(*port_1.pins_in, port_1.pin_in)) {
         apa_get_packet(&port_1);
         if (port_1.path_in_length != 0) {
            apa_move_packet(&port_1,&port_0);
            apa_put_serial_packet(&port_0);
            }
         }
      }
   }
