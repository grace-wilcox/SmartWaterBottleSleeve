import serial
import time
import csv

if __name__ == '__main__':
    serialport = serial.Serial('/dev/cu.usbmodem2101', 115200)
    serialport.timeout = 2  # set read timeout

    if serialport.is_open:
        with open('movement_check.csv', 'w', newline='') as csvfile: # 'w' write over, 'a' append
            csvwriter = csv.writer(csvfile)
            csvwriter.writerow('1')
            while True:
                timenow = time.time()
                raw = serialport.readline()
                data = raw.decode(encoding='UTF-8')
                print(data)
                csvwriter.writerow([timenow,data])
