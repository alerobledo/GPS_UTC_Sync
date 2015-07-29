#example call this script:  python read_serial.py /dev/ttyACM0

import multiprocessing
import serial
import time
import sys

def readSerial(serialPort, baudRate):
        ser = serial.Serial(serialPort, baudRate)
        while True:
                print 'Serial port: ' + repr(serialPort) +'  - Value received: ' + repr(ser.readline()) +'   - at time: ' + repr(time.time())

#sys.argv[1]
                
if __name__ == "__main__":
    hilo1 = multiprocessing.Process(target=readSerial, args=(sys.argv[1], 115200))
    hilo2 = multiprocessing.Process(target=readSerial, args=(sys.argv[2], 115200))

    print "Launching threads"
    hilo1.start()
    hilo2.start()
    hilo1.join()
    hilo2.join()
    print "Done"

