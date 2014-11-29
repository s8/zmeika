# //
# // zmeika_gui.py
# //
# // python/Tkinter gui for zmeika digital puzzle
# //
# // Konstantin Leonenko 2014
# //


from Tkinter import *
import serial, sys, time
import Queue
import threading
import os

num_modules = 2

serial_monitor_flag = True

ser = serial.Serial("/dev/tty.usbserial-A603OPB4", 57600)
ser.flushInput()
ser.flushOutput()

serial_queue = Queue.Queue()

#----------------------------------------------------------
# define special APA chars
#----------------------------------------------------------
packet_start = '{'
packet_pointer = '^'
packet_divider = '|'
packet_end = '}'
packet_escape = '\\'
timeout_count = 1000
char_delay = 0.001

#packet divider hex-string representation
pd = str(packet_divider.encode('hex'))


#--------------------------------------------------------------------------------------------------------------------
# single module class
#--------------------------------------------------------------------------------------------------------------------
class module():

    module_id = ''
    led_value = 0
    button = ''
    angle = 0

    def __init__(self, module_id,angle,led_value,button): #, queue):
            self.module_id  = module_id
            self.led_value  = led_value
            self.button     = button
            #self.queue      = queue

    #----------------------------------------------------------
    # check for waiting chars and exit on timeout
    #----------------------------------------------------------
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

    def store_path(self):
        print 'poking module ' + str(self.module_id)
        packet = packet_start + packet_pointer + self.module_id + packet_divider + 'p' + packet_end
        self.send_packet(packet)

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

#--------------------------------------------------------------------------------------------------------------------
# zmeika window class
#--------------------------------------------------------------------------------------------------------------------
class Zmeika(Frame):

   # global serial_queue

    module_list = list()

    enc_entries = list()

    led_buttons = list()
    led_sliders = list()

    enc_buttons = list()
    enc_sliders = list()

    


    #-----------------------------
    # initialization
    #-----------------------------
    def __init__(self, master=None):

        Frame.__init__(self, master)
        self.grid(row = 0, column  = 0)
        self.createWidgets()
        
        #running serial monitor in a separate thread to keep GUI responsive
        serial_monitor = threading.Thread(target=self.watch_serial)
        serial_monitor.daemon = True
        serial_monitor.start()
        
        #wanted to run this in a separate thread as well, but then the enc_entries in not acessible
        self.update_GUI()

        time.sleep(1.0)
        self.module_list[0].store_path()
        time.sleep(2.0)
        self.module_list[1].store_path()
        

    #-----------------------------
    # GUI updater function
    #-----------------------------
    def update_GUI(self):
        global serial_queue
        global pd
        if not serial_queue.empty():
            p = ''
            for j in serial_queue.get().split(':'):
                 p+= j

            d = p.find(pd) # packet divider location
            #select the origin module
            a = (d - 2 )/ 2 - 1

            s = 'node: (' + str(p[d-2-2*a:d-2].decode('hex'))+ ') | ' + str(int(p[-4:-2],16))
            
            self.enc_entries[a].delete(0,END)
            self.enc_entries[a].insert(0, s)

        #emulating an infinite loop this way, as otherwise it blocks the GUI thread
        self.after(10,self.update_GUI)

    #-----------------------------
    # serial monitor function
    #-----------------------------
    def watch_serial(self):
        incoming_packet  = ''

        print "serial monitor started!"

        while True:
            char = ser.read().encode('hex')
            incoming_packet += char
            incoming_packet += ':'
            if (char == packet_end.encode('hex')):
                serial_queue.put(incoming_packet[:-1])
                incoming_packet = ""

    #-----------------------------
    # make the GUI elements function
    #-----------------------------

    def createWidgets(self):

        a = ''

        #-----------------------------
        # creating GUI elements
        #-----------------------------
        for i in range(num_modules):
  
            self.led_buttons.insert(i, Button(self, text = 'OFF'))
            self.led_buttons[i].grid(row = 1, column = i * 2)

            self.led_sliders.insert(i, Scale(self, from_=255, to=0))
            self.led_sliders[i].grid(row = 0, column = i * 2)

            self.enc_entries.insert(i, Entry(self))
            self.enc_entries[i].grid(row=2,column=(i*2),columnspan=1)

            self.module_list.insert(i, module(a, 0, 0, self.led_buttons[i]))

            self.led_buttons[i]["command"] = self.module_list[i].toggle_led
            self.led_sliders[i]["command"] = self.module_list[i].set_led

            a += '1'

        # for i in range(num_modules):
        #     self.module_list[i].store_path()
        #     time.sleep(0.2)


        self.QUIT = Button(self, text = "QUIT", fg = "red",
          command = quit).grid(row = 3, column = 0)

#--------------------------------------------------------------------------------------------------------------------

root = Tk()
app = Zmeika(master=root)


app.mainloop()

#-----------------------------
# do on quit
#-----------------------------

def quit():  
  ser.close()
  global root
  root.destroy()




