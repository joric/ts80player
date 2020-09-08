#!/usr/bin/env python3

import cv2 # pip install opencv-python
import serial
import time
from PIL import Image
import itertools

fname = 'bad_apple_96x16.mp4'
fname = 'doom_128x64.mp4'
#fname = 'C:/Videos/bad_apple/bad_apple_hd_widescreen.mp4'

cap = cv2.VideoCapture(fname)

fps = cap.get(cv2.CAP_PROP_FPS)
frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
frame = 0

try:
    s = serial.Serial('COM18')
except Exception as e:
    s = None
    print(e)

def collect(bits):
    return [sum([(1 if byte[i] else 0)<<(7-i) for i in range(0,8)]) for byte in zip(*(iter(bits),) * 8)]

def convert(im):
    im = cv2.cvtColor(im, cv2.COLOR_BGR2RGB)
    im = Image.fromarray(im)
    im = im.resize((96,16), Image.ANTIALIAS)
    im = im.convert(mode='1', dither=Image.FLOYDSTEINBERG)
    #im.save("images/%d.png" % frame, "png")
    return collect(im.getdata())

start = time.time()
while True:
    if time.time() >= start + frame * 1./fps:
        ret, im = cap.read()
        if not ret:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            continue

        buf = convert(im)
        if s: s.write(buf)
        print ("frame", frame, "of", frames, "at", fps, "fps", end='\r')
        frame += 1
