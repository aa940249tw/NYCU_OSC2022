#!/usr/bin/python3

import numpy as np
from serial import Serial
from tqdm import tqdm
from time import sleep

def reset_serial(s):
    s.flush()
    s.flushInput()
    s.flushOutput()

def send_img():
    with open("/home/aa940249tw/nctu/OSDI/LAB_1/kernel8.img", "rb") as imgfile:
        with Serial('/dev/ttyUSB0', 115200) as ser:
            data = imgfile.read()
            data_size = len(data)
            print("Start sending data...")
            reset_serial(ser)
            ser.write(b'load_img\n')
            reset_serial(ser)
            sleep(3)
            ser.write(data_size.to_bytes(4, 'big'))
            reset_serial(ser)

            print("Image_size: {0}".format(data_size))
            
            chunk_size = 128
            for i in tqdm(range(len(data)//chunk_size + 1)):
                sleep(0.1)
                if ((chunk_size * (i+1)) < len(data)):
                    ser.write(data[i*chunk_size : (i+1)*chunk_size])
                else:
                    ser.write(data[i*chunk_size: ])   
            
            reset_serial(ser)
if __name__ == "__main__":
    send_img()
    