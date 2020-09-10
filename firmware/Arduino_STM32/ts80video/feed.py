#!/usr/bin/env python3

import cv2 # pip install opencv-python
import serial
import time
import sys
import itertools
from PIL import Image, ImageOps, ImageGrab, ImageFilter, ImageEnhance

def find_serial():
    import serial.tools.list_ports
    for port, desc, hwid in serial.tools.list_ports.comports():
        if 'VID:PID=1EAF:0029' in hwid:
            return port
    return None

def collect(bits):
    return [sum([(1 if byte[i] else 0)<<(7-i) for i in range(0,8)]) for byte in zip(*(iter(bits),) * 8)]

def convert(im, frame):
    im = im.resize((96,16), Image.ANTIALIAS)
    im = im.convert('L')
    im = ImageOps.posterize(im, bits=3)

    enhancer = ImageEnhance.Brightness(im)
    im = enhancer.enhance(1.5)

    enhancer = ImageEnhance.Contrast(im)
    im = enhancer.enhance(1.75)

    im.save("images/%04d.png" % frame, "png")

    #im = im.filter(ImageFilter.FIND_EDGES)

    #im = im.convert(mode='1', dither=Image.NONE)
    im = im.convert(mode='1', dither=Image.FLOYDSTEINBERG)

    return collect(im.getdata())

def main(video):

    port = find_serial()
    print("Opening port", port)
    s = serial.Serial(port)

    cap = None
    frames = None
    frame = 0
    fps = None

    if video:
        print("Reading", video)
        try:
            cap = cv2.VideoCapture(video)  # no exception here?
            fps = cap.get(cv2.CAP_PROP_FPS) # reads 0 if no file
            frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
        except Exception as e:
            print(e)

    if not fps:
        cap = None
        fps = 30
        print("No video given, grabbing from screen at", fps, "fps")


    start = time.time()
    while True:
        if time.time() >= start + frame * 1./fps:

            if cap:
                ret, im = cap.read()
                if not ret:
                    cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                    continue
                im = Image.fromarray(cv2.cvtColor(im, cv2.COLOR_BGR2RGB))
            else:
                im = ImageGrab.grab() # grab from screen

            buf = convert(im, frame)

            if s: s.write(buf)
            print ("frame", frame, "of", frames, "at", fps, "fps", end='\r')
            frame += 1

if __name__ == '__main__':
    video = None if len(sys.argv)<2 else sys.argv[1]
    #video = 'bad_apple_96x16.mp4'
    #video = 'doom_128x64.mp4'
    main(video)

