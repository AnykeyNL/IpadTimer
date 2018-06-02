#!/usr/bin/python

import RPi.GPIO as GPIO
import time
import datetime
import MFRC522
import signal
from Adafruit_LED_Backpack import SevenSegment

GPIO.setwarnings(False)

segment = SevenSegment.SevenSegment(address=0x70)
segment.begin()

MIFAREReader = MFRC522.MFRC522()


print "Press CTRL+Z to exit"

debug = 1

teller = 0

hour = 3
minute = 0 
second = 0

switch = 18
red = 11
green = 13

GPIO.setup(switch, GPIO.IN,pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(red,GPIO.OUT)
GPIO.setup(green,GPIO.OUT)

kaarten = []
kaarten.append([112,129,45,164,15,0])
kaarten.append([57,151,117,99,99,0])
kaarten.append([116,53,219,95,15,0])
kaarten.append([208,178,112,163,15,0])
kaarten.append([236,192,47,7,15,0])
kaarten.append([166,248,131,99,99,0])

def reset():
  global kaarten

while(True):

 (status,TagType) = MIFAREReader.MFRC522_Request(MIFAREReader.PICC_REQIDL)
 if status == MIFAREReader.MI_OK:
   try:
     (status,uid) = MIFAREReader.MFRC522_Anticoll()
     print "Card read UID: %s,%s,%s,%s" % (uid[0], uid[1], uid[2], uid[3])
   
     for kaart in kaarten:
       if (kaart[0]==uid[0] and kaart[1] == uid[1] and kaart[2] == uid[2] and kaart[3] == uid[3]):
         print (kaart[4])
         if kaart[5] == 0:
           print ("Geldig")
           kaart[5] = 1
         else: 
           print ("al gebruikt!")
   except:
     print ("fout met lezen")



 ipad = GPIO.input(switch)
 if(ipad==0):
  GPIO.output(red,False)  
  GPIO.output(green,True)
  segment.clear()
  # Set hours
  segment.set_digit(0, int(hour / 10))     # Tens
  segment.set_digit(1, hour % 10)          # Ones
  # Set minutes
  segment.set_digit(2, int(minute / 10))   # Tens
  segment.set_digit(3, minute % 10)        # Ones
  # Toggle colon
  segment.set_colon(second % 2)              # Toggle colon at 1Hz

  # Write the display buffer to the hardware.  This must be called to
  # update the actual display LEDs.
  segment.write_display()

  # Wait a quarter second (less than 1 second to prevent colon blinking getting$
  if (debug == 0):
    time.sleep(0.25)
    teller = teller+1
  else:
    teller = 4
  if (teller==4):
    teller = 0
    if (second == 0):
      if (minute == 0):
        if (hour == 0):
          break
        else:
          hour = hour-1
        minute=59  
      else:
        minute = minute-1
      second=59
    else:
      second = second-1
    
 else:
  GPIO.output(green,False)
  GPIO.output(red,True)



