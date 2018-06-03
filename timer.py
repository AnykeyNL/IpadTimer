#!/usr/bin/python
# 
# Tablet Timer project
# Written by Levi Schutte and Richard Garsthagen
# https://github.com/AnykeyNL/IpadTimer

import RPi.GPIO as GPIO
import time
import datetime
import MFRC522
import signal
from Adafruit_LED_Backpack import SevenSegment

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD) # Mandatory as the MFRC522 libary is based on this

###################################################
# Main configuration

DailyTotalSeconds = 10 * 60 * 1    # 5 hours
Debug = False

##################################################


class RFIDCards:
	carddata = []
	MIFAREReader = MFRC522.MFRC522()

	def __init__(self):
		print ("Reading RFID Card data")
		RFIDCards.carddata = []
		cardfile = open("cards.txt","r")
		for line in cardfile:
			try:
				if line.find("#") == -1:
					id1,id2,id3,id4,add_time = line.strip("\n").split(",")
					RFIDCards.carddata.append([int(id1),int(id2),int(id3),int(id4),int(add_time),0])
			except:
				a = 1
	def show(self):
		for card in RFIDCards.carddata:
			print (card)

	def setUsed(self,id1,id2,id3,id4):
		idx = 0
		for card in RFIDCards.carddata:
			if card[0] == id1 and card[1] == id2 and card[2] == id3 and card[3] == id4:
				RFIDCards.carddata[idx][5] = 1
			idx = idx + 1

	def cardValue(self,id1,id2,id3,id4):
		 for card in RFIDCards.carddata:
                        if card[0] == id1 and card[1] == id2 and card[2] == id3 and card[3] == id4:
                                return card[4]

	def reset(self):
		for card in RFIDCards.carddata:
			card[5] = 0
		

	def checkRFID(self):
		global LeftSeconds, DailyTotalSeconds
		(status,TagType) = RFIDCards.MIFAREReader.MFRC522_Request(RFIDCards.MIFAREReader.PICC_REQIDL)
		if status == RFIDCards.MIFAREReader.MI_OK:
			try:		
				(status,uid) = RFIDCards.MIFAREReader.MFRC522_Anticoll()
				if Debug:
					print "Card read UID: %s,%s,%s,%s" % (uid[0], uid[1], uid[2], uid[3])	
				idx = 0
				for card in RFIDCards.carddata:
					if card[0] == uid[0] and card[1] == uid[1] and card[2] == uid[2] and card[3] == uid[3] and card[4] == 99:
						if Debug:
							print ("Resetting time and used cards")
						self.reset()
						LeftSeconds = DailyTotalSeconds
						display.setTime(LeftSeconds)
						time.sleep(5)

		                        elif card[0] == uid[0] and card[1] == uid[1] and card[2] == uid[2] and card[3] == uid[3] and card[5] == 0:
						if Debug: 
							print ("adding time: {}".format(card[4]))
						LeftSeconds = LeftSeconds + (card[4] * 60)
						self.setUsed(uid[0], uid[1], uid[2], uid[3])
				idx = idx + 1

			except:
				if Debug:
					print "Error reading card"


class TimerDisplay:
	segment = SevenSegment.SevenSegment(address=0x70)

	def __init__(self, startseconds):
		TimerDisplay.segment.begin()
		self.setTime(startseconds)

	def setTime(self,seconds):
		hour,minute,second = self.HourMinuteSeconds(seconds)
		TimerDisplay.segment.clear()
		TimerDisplay.segment.set_digit(0, int(hour / 10))     # Hour Tens
		TimerDisplay.segment.set_digit(1, hour % 10)          # Hour Ones
		TimerDisplay.segment.set_digit(2, int(minute / 10))   # Minute Tens
		TimerDisplay.segment.set_digit(3, minute % 10)        # Minute Ones
		TimerDisplay.segment.set_colon((second+1) % 2 ) 	
		TimerDisplay.segment.write_display()

	def HourMinuteSeconds(self,seconds):
		m, s = divmod(seconds, 60)
		h, m = divmod(m, 60)
		return h,m,s

	def setEND(self):
                TimerDisplay.segment.clear()	
		TimerDisplay.segment.set_digit_raw(0,0)
                TimerDisplay.segment.set_digit_raw(1,121)  # E
                TimerDisplay.segment.set_digit_raw(2,84)   # n
                TimerDisplay.segment.set_digit_raw(3,94)   # d
                TimerDisplay.segment.write_display()


class LEDS:
	redpin = 11   
	greenpin = 13
	
	def __init__(self):
		GPIO.setup(LEDS.redpin,GPIO.OUT)
		GPIO.setup(LEDS.greenpin,GPIO.OUT)

	def red(self, state):
		GPIO.output(LEDS.redpin, state)

	def green(self,state):
		GPIO.output(LEDS.greenpin, state)


class TabletDetect:
	switchpin = 18
	
	def __init__(self):
		GPIO.setup(TabletDetect.switchpin, GPIO.IN,pull_up_down = GPIO.PUD_DOWN)

	def state(self):
		 return GPIO.input(TabletDetect.switchpin)




LeftSeconds = DailyTotalSeconds

cards = RFIDCards()
display = TimerDisplay(LeftSeconds)

LED = LEDS()
switch = TabletDetect()

print ("Tablet timer is running...")

while(True):
	cards.checkRFID()
	if LeftSeconds > 0:
		display.setTime(LeftSeconds)
		if switch.state() == 0:
                	LeftSeconds = LeftSeconds -1
	                LED.red(0)
        	        LED.green(1)
	        else:
			if LeftSeconds % 2:  # Always stop timer on even second to show colon in time
				LeftSeconds = LeftSeconds + 1
        	        LED.red(1)
                	LED.green(0)
			
	else:
		display.setEND()
		LED.red(0)
		LED.green(0)
	
	time.sleep(1)







