#!/usr/bin/env python
#
# apa.py
#    APA serial interface
#
# Neil Gershenfeld
# CBA MIT 12/4/11
#
# (c) Massachusetts Institute of Technology 2010
# Permission granted for experimental and personal use;
# license for commercial sale available from MIT.
#
import serial, sys, time
#
# define special chars
#
packet_start = '{'
packet_pointer = '^'
packet_divider = '|'
packet_end = '}'
packet_escape = '\\'
timeout_count = 1000
char_delay = 0.001
#
# test for waiting chars
#
def test_waiting():
   check_count = 0
   while 1:
      if (0 != ser.inWaiting()):
         return 1
      check_count += 1
      if (check_count == timeout_count):
         print "test_waiting: timeout"
         return 0
      time.sleep(char_delay)
#
# check for waiting chars and exit on timeout
#
def check_waiting():
   check_count = 0
   while 1:
      if (0 != ser.inWaiting()):
         break
      check_count += 1
      if (check_count == timeout_count):
         print "check_waiting: timeout"
         sys.exit()
      time.sleep(char_delay)
   return
#
# get input
#
if (len(sys.argv) != 5):
   print "command line: apa.py serial_port port_speed path payload"
   sys.exit()
serial_port = sys.argv[1]
port_speed = int(sys.argv[2])
path = sys.argv[3]
payload = sys.argv[4]
#
# open port
#
#ser = serial.Serial(port=serial_port,baudrate=port_speed,rtscts=True)
ser = serial.Serial(port=serial_port,baudrate=port_speed)
ser.flushInput()
ser.flushOutput()
#
# send packet
#
packet = packet_start + packet_pointer + path + packet_divider + payload + packet_end
print "send packet: "+packet
for i in range(len(packet)):
   ser.write(packet[i])
   time.sleep(char_delay)
#
# get packet
#
packet = ""
chr0 = ''
count = 0
while (chr0 != packet_start): # start
   check_waiting()
   chr0 = ser.read()
   count += 1
   if (count == timeout_count):
      print "apa.serial.test.py: timeout"
      sys.exit()
packet += chr0
chr0 = ''
count = 0
while (chr0 != packet_pointer): # pointer
   check_waiting()
   chr0 = ser.read()
   count += 1
   if (count == timeout_count):
      print "apa.serial.test.py: timeout"
      sys.exit()
   packet += chr0
chr0 = ''
count = 0
while (chr0 != packet_divider): # divider
   check_waiting()
   chr0 = ser.read()
   count += 1
   if (count == timeout_count):
      print "apa.serial.test.py: timeout"
      sys.exit()
   packet += chr0
chr0 = ''
count = 0
while (chr0 != packet_end): # end
   check_waiting()
   chr0 = ser.read()
   count += 1
   if (count == timeout_count):
      print "apa.serial.test.py: timeout"
      sys.exit()
   if (chr0 != packet_escape):
      packet += chr0
   else:
      check_waiting()
      chr0 = ser.read() # read escaped char
      packet += chr0
      check_waiting()
      chr0 = ser.read() # read next char
      packet += chr0
print "receive packet: "+packet
for i in range(len(packet)):
   print packet[i],ord(packet[i])
