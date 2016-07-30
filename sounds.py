#!/usr/bin/env python

import RPi.GPIO as GPIO
import sys, os, time, random, subprocess

try:
	(hit_pin, win_pin, lose_pin) = (int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]))
except:
	(hit_pin, win_pin, lose_pin) = [4, 17, 27]

print "Starting sounds.py at ", time.asctime()
print "pins %d %d %d" % (hit_pin, win_pin, lose_pin)

# are we referring to pin(s) by "BOARD" (ie., printed on board)
# or "BCM" (broadcom SOC channel) 
# For example, on pi board 2, model b, it is 4(BCM) == 7(BOARD) and
# 17(BCM) == 11(BOARD)
GPIO.setmode(GPIO.BCM)

GPIO.setup(hit_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(win_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(lose_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

sound_dir = "/home/pi/shooting-gallery"
hit_files = ["ricochet1.wav", "ricochet2.wav"]
win_files = ["win1.wav", "win2.wav"]
lose_files = ["lose1.wav", "lose2.wav", "lose3.wav", "lose4.wav"]

def callback_impl(sound_files):
	wav = random.choice(sound_files)
	wav_path = os.path.join(sound_dir, wav)
	subprocess.Popen(["aplay", wav_path], stdin=None, stdout=None, stderr=None,close_fds=True)

# our callback runs in a separate thread, but (I think) there is just one
# such event handling thread. so if we want to have a 1 second sound get
# trigger twice in a short time interval (and overplay each other) I think
# need to spawn an aplay system process and not wait around for the 
# os.system() call to return.
def hit_cb(channel):
	print "HIT: rising edge on channel %d" % channel
	callback_impl(hit_files)

def win_cb(channel):
	print "WIN: rising edge on channel %d" % channel
	callback_impl(win_files)

def lose_cb(channel):
	print "LOSE: rising edge on channel %d" % channel
	callback_impl(lose_files)

GPIO.add_event_detect(hit_pin, GPIO.FALLING, callback=hit_cb, bouncetime=1)
GPIO.add_event_detect(win_pin, GPIO.FALLING, callback=win_cb, bouncetime=1)
GPIO.add_event_detect(lose_pin, GPIO.FALLING, callback=lose_cb, bouncetime=1)

try:
	time.sleep(10000000000)
finally:
	GPIO.cleanup()

