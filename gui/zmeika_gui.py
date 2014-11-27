
# TO DO:
# on quit the serial port leaves hanging open
#
#


from Tkinter import *
import serial, sys, time
import Queue
import threading
import os

packets = list()
packets.append ('abc')

enc_entries = list()

serial_queue = Queue.Queue()

module_list = list()

num_modules = 2

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

ser = serial.Serial("/dev/tty.usbserial-A603OPB4", 57600)
ser.flushInput()
ser.flushOutput()

#
# Globabl serial watching function
#
def watch_serial():
    global packets
    global enc_entries

    incoming_packet  = list()
    print "started!"
    while (1):
        char = ser.read().encode('hex')
        incoming_packet += char
        incoming_packet += ':'
        if (char == packet_end.encode('hex')):
            print incoming_packet[:-1]
            packets[0] = incoming_packet
            # serial_queue.put(incoming_packet)
            incoming_packet = ""

            # functional, but slow way to do this
            # enc_entries[0].config(state = NORMAL)
            # enc_entries[0].delete(0,END)
            # enc_entries[0].insert(0, str(packets[0][12 :-4]))
            # enc_entries[0].config(state = DISABLED)

def update_GUI():
    while True:
        enc_entries[0].delete(0,END)
        enc_entries[0].insert(0, str(packets[0][12:-4]))
        # enc_entries[0].insert(0, "yo")
        # if not serial_queue.empty():
        #     enc_entries[0].delete(0,END)
        #     enc_entries[0].insert(0, str(serial_queue.get()[12 :-4]))
        #     # enc_entries[0].insert(0, str(packets[0][12 :-4]))
        #     # enc_entries[0].insert(0, "yo")
        time.sleep(0.01)


class module():

    module_id = ''
    led_value = 0
    button = ''
    angle = 0

    def __init__(self, module_id,angle,led_value,button, queue):
            self.module_id  = module_id
            self.led_value  = led_value
            self.button     = button
            self.queue      = queue

    def send_packet(self, packet):
        for i in packet:
            ser.write(i)
            time.sleep(char_delay)

    def read_packet(self):
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
      #print 'chr 0 = ' +chr0
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
      #print 'chr 1 = ' +chr0
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
      #print 'chr 2 = ' +chr0
      chr0 = ''
      count = 0
      while (chr0 != packet_end): # end
         check_waiting()
         chr0 = ser.read()
         #print 'chr a = ' +chr0
         count += 1
         if (count == timeout_count):
            print "apa.serial.test.py: timeout"
            sys.exit()
         if (chr0 != packet_escape):
            #print 'chr b = ' +chr0
            packet += chr0
         else:
            check_waiting()
            chr0 = ser.read() # read escaped char
            #print 'chr c = ' +chr0
            packet += chr0
            check_waiting()
            chr0 = ser.read() # read next char
            #print 'chr d = ' +chr0
            packet += chr0
      print "received packet: "+packet

      print ':'.join(x.encode('hex') for x in packet)


    def read_enc(self):
      print '------------------------'
      packet = packet_start + packet_pointer + self.module_id + packet_divider + '0' + packet_end
      self.send_packet(packet)
      self.read_packet()
      # packet = packet_start + packet_pointer + self.module_id + packet_divider + '2' + packet_end
      # self.send_packet(packet)
      # self.read_packet()

    def set_led(self, led_value):
        self.led_value = led_value

        packet = packet_start + packet_pointer + self.module_id + packet_divider + 'w' + str(hex(255))[2:] + str(hex(int(led_value)))[2:] + packet_end
        self.send_packet(packet)

        if led_value == 0:
            self.button["text"] = 'OFF'
        elif led_value == 255:
            self.button["text"] = 'ON'
        else:
            self.button["text"] = str(led_value)

    def read_led(self):
        print 'readLed method prototype'

    def watch_serial():
        global packets
        global enc_entries

        incoming_packet  = list()
        print "started!"
        while (1):
            char = ser.read().encode('hex')
            incoming_packet += char
            incoming_packet += ':'
            if (char == packet_end.encode('hex')):
                print incoming_packet[:-1]
                packets[0] = incoming_packet
                incoming_packet = ""

                # functional, but slow way to do this
                # enc_entries[0].config(state = NORMAL)
                # enc_entries[0].delete(0,END)
                # enc_entries[0].insert(0, str(packets[0][12 :-4]))
                # enc_entries[0].config(state = DISABLED)

    def toggle_led(self):
        if self.led_value >= 128:
            self.led_value = 0
            packet = packet_start + packet_pointer + self.module_id + packet_divider + 'n' + packet_end
            self.button["text"] = 'OFF'
            print 'toggle LED: ' + str(self.module_id) + ' OFF'
        else:
            self.led_value = 255
            packet = packet_start + packet_pointer + self.module_id + packet_divider + "f" + packet_end
            self.button["text"] = 'ON'
            print 'toggle LED: ' + str(self.module_id) + ' ON'
        self.send_packet(packet)

class Zmeika(Frame):

    global serial_queue

    global module_list


    #===================#===================#===================
    # initialization
    #===================#===================#===================

    def __init__(self, master=None):

        Frame.__init__(self, master)
        self.grid(row = 0, column  = 0)
        self.createWidgets()

    def createWidgets(self):

        global enc_entries

        led_buttons = list()
        led_sliders = list()

        enc_buttons = list()
        enc_sliders = list()


        a = ''

        # initializing buttons and sliders
        for i in range(num_modules):
            l_s = Scale(self, from_=255, to=0)
            l_b = Button(self, text = 'OFF')
            l_s.grid(row = 0, column = i * 2)
            l_b.grid(row = 1, column = i * 2)
            led_buttons.append(l_b)
            led_sliders.append(l_s)

            e_s = Scale(self, from_=255, to=0)
            e_b = Button(self, text = 'read \n enc')
            e_e = Entry(self, text = 'packet here')#, state = DISABLED)

            e_s.grid(row = 0, column = i*2 + 1)
            e_b.grid(row = 1, column = i*2 + 1)
            e_e.grid(row = 2, column = (i * 2), columnspan = 2)

            # e_e.insert(0,'abc')

            enc_buttons.append(e_b)
            enc_sliders.append(e_s)
            enc_entries.append(e_e)

        # initializing modules
        for i in range(num_modules):
            m = module(a, 0, 0, led_buttons[i], serial_queue)
            led_buttons[i]["command"] = m.toggle_led
            led_sliders[i]["command"] = m.set_led
            #enc_buttons[i]["command"] = m.read_enc
            module_list.append(m)
            a += '1'

        self.QUIT = Button(self, text = "QUIT", fg = "red",
          command = self.quit).grid(row = 3, column = 0)

        self.led_buttons = led_buttons
        self.led_sliders = led_sliders


root = Tk()
app = Zmeika(master=root)

serial_monitor = threading.Thread(target=watch_serial)
serial_monitor.start()

gui_updater = threading.Thread(target = update_GUI)
gui_updater.start()


app.mainloop()

root.destroy()
