#example call this script:  python read_serial.py /dev/ttyACM0

import serial
import time
import sys

ser = serial.Serial(sys.argv[1], 115200)
while True:
	print time.time() 
	print ser.readline()
