#example call this script:  python read_serial.py /dev/ttyACM0

import serial
import datetime
import sys

ser = serial.Serial(sys.argv[1], 115200)
fileName = sys.argv[2]+'.csv'

while True:
	line = ser.readline()
	t = datetime.datetime.now()
	f = open(sys.argv[2]+'.csv', 'a')
	print repr(t.isoformat()) + '  -  '+repr(line) 
	f.write(t.isoformat())
	f.write(',')
	f.write(line)
	f.close()
