#!/usr/bin/env python3
"""Reset the SX1302 concentrator on the Elecrow LR1302 HAT.

The HAT's reset line is on GPIO 17 (BCM numbering). The stock
reset_lgw.sh does this with gpioset, which hung on my Pi (libgpiod v2 on
Raspberry Pi OS trixie). This does the same pulse with RPi.GPIO.

Run before starting the packet forwarder:

    sudo python3 reset_gpio.py

Needs: sudo apt install python3-rpi.gpio
"""

import time

import RPi.GPIO as GPIO

RESET_PIN = 17  # BCM numbering

GPIO.setwarnings(False)   # the pin may still be claimed from a previous run
GPIO.setmode(GPIO.BCM)
GPIO.setup(RESET_PIN, GPIO.OUT)

# Pulse: low, high for 100 ms (chip held in reset), low again (chip runs).
GPIO.output(RESET_PIN, GPIO.LOW)
time.sleep(0.1)
GPIO.output(RESET_PIN, GPIO.HIGH)
time.sleep(0.1)
GPIO.output(RESET_PIN, GPIO.LOW)

GPIO.cleanup()
