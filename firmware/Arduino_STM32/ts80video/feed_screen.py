#!/usr/bin/env python3

import cv2 # pip install opencv-python
import serial
import time
from PIL import Image
from PIL import ImageGrab
from PIL import ImageFilter
import itertools

frame = 0
frames = 0
fps = 30

try:
    s = serial.Serial('COM18')
except Exception as e:
    s = None
    print(e)

def collect(bits):
    return [sum([(1 if byte[i] else 0)<<(7-i) for i in range(0,8)]) for byte in zip(*(iter(bits),) * 8)]

def convert(im):
    im = im.resize((96,16), Image.ANTIALIAS)
    im = im.filter(ImageFilter.FIND_EDGES)
    im = im.convert(mode='1', dither=Image.FLOYDSTEINBERG)
    #im.save("images/%d.png" % frame, "png")
    return collect(im.getdata())

start = time.time()
while True:
    if time.time() >= start + frame * 1./fps:
        im = ImageGrab.grab(bbox=(0,0,1920,1080))
        buf = convert(im)
        if s: s.write(buf)
        print ("frame", frame, "at", fps, "fps", end='\r')
        frame += 1
