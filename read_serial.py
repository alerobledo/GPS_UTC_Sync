#example call this script:  python read_serial.py /dev/ttyACM0

import serial
import datetime
import sys

ser = serial.Serial(sys.argv[1], 115200)

while True:
	line = ser.readline()
	t = datetime.datetime.now()
	print 'Value received: ' + repr(line) +'   - at time: ' + repr(t.isoformat())
