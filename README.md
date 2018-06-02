# IpadTimer
IpadTimer - Control ipad usage time and allow for bonus system

## Project Description
Make a simple device that will allow a user (kids) to use an iPad for a certain amount of time during a day. When the ipad is in the box, the time is stopped. When the ipad is taken out, the timer is counting down. The timer device has an RFID reading buildin, so that bonus cards can be given out to get extra time. 

The project is is based on a Raspberry PI zero W, an MFRC-522 RFID reader and an Adafruit 0.56" 4-Digit 7-Segment Display w/I2C Backpack. 

The software will use the Telegram API to send messages to the Ipad for every hour that has passed and when there is no more time to play for that day.

![Inner works of the inital hardware](https://github.com/AnykeyNL/IpadTimer/blob/master/ipad_1.jpg)
![Sample of first hardware model](https://github.com/AnykeyNL/IpadTimer/blob/master/ipad_2.jpg)
