#!/usr/bin/env python3

import cv2 # pip install opencv-python
import serial
import time

fname = 'bad_apple_96x16.mp4'
#fname = 'C:/Downloads/Doom 2 1.9 Demo Loop - Attract Mode - HD.mp4'
#fname = 'C:/Videos/bad_apple/bad_apple_hd_widescreen.mp4'

cap = cv2.VideoCapture(fname)

fps = cap.get(cv2.CAP_PROP_FPS)
frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

try:
    s = serial.Serial('COM18')
except Exception as e:
    s = None
    print(e)

def convert(im):
    im = cv2.resize(im, (96, 16))
    grey_im = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
    im = cv2.threshold(im, 127, 255, cv2.THRESH_BINARY)[1]
    h = im.shape[0]
    w = im.shape[1]
    byte = 0
    buf = []
    shift = 0
    lines = h//8
    for i in range(lines):
        for y in range(i*8, i*8+8):
            for x in range(w):
                bit = 1 if any(im[y,x]) else 0
                byte |= bit << (7-shift)
                shift += 1
                if shift==8:
                    shift = 0
                    buf.append(byte)
                    byte = 0
            buf.extend([0]*((128-w)//8))
    return buf

count = 0
prev = 0

while True:
    time_elapsed = time.time() - prev
    if time_elapsed > 1./fps:
        prev = time.time()
        res, im = cap.read()
        if not res: break
        buf = convert(im)
        if s: s.write(buf)
        print ("frame", count, "of", frames, "at", fps, "fps", end='\r')
        count += 1





