from flask import Flask, make_response
import StringIO
import subprocess
import sys
import time
import random
from collections import deque
import numpy as np
from bokeh.plotting import *
from bokeh.models.renderers import GlyphRenderer
import atexit
    




def graph1():
    MAX_DATA = 60
    cmd = ["../exe/mask"]
    child = subprocess.Popen(cmd)
    # We will store the data here
    x_inputdata = []
    y_inputdata = []

    output_server("multiple_updateable_plots")

    # Set up first plot
    p1 = figure(title="Input Signal")
    p1.line(x_inputdata, y_inputdata,
            color="#0000FF",
            tools="pan,resize,wheel_zoom", width=1200, height=300)
    show(p1)
    # Set up the dynamic plotting
    renderer_input = p1.select(dict(type=GlyphRenderer))
    ds_input = renderer_input[0].data_source

    # child.terminate()
    while True:
        #neg = child.stdout.read(1)
        # graph the first thing

        try:
            f = open("../streams/inputsignal.txt", 'r')
            for r in f:
                try:
                    data = float(r)
                except:
                    continue
                time_ref = time.time() % 60
                ds_input.data["x"] = x_inputdata
                ds_input.data["y"] = y_inputdata
                ds_input._dirty = True
                cursession().store_objects(ds_input)

                # Plotting only the last MAX_DATA samples
                if len(x_inputdata) > MAX_DATA:
                    x_inputdata.pop(0)
                    y_inputdata.pop(0)
                x_inputdata.append(time_ref)
                y_inputdata.append(data)

                time.sleep(0.1)
        except:
            # print "waiting the bat.."
            # Modify (seconds) to se sufficient with your bat to be ready
            time.sleep(.1)
            continue


# plots input fft
def graph2():
    MAX_DATA = 60

    # We will store the data here
    x_fftdata = []
    y_fftdata = []

    output_server("multiple_updateable_plots")

    # Set up second plot
    p2 = figure(title="FFT of Input")
    p2.line(x_fftdata, y_fftdata,
            color="#0000FF",
            tools="pan,resize,wheel_zoom", width=1200, height=300)

    # Show plots!
    show(p2)
    # Set up the dynamic plotting

    renderer_fft = p2.select(dict(type=GlyphRenderer))
    ds_fft = renderer_fft[0].data_source
    # child.terminate()
    while True:
        #neg = child.stdout.read(1)
        # graph the first thing
        with open("../streams/fft.txt", 'r') as fft:
            for r in fft:
                try:
                    data = float(r)
                except:
                    continue
                time_ref = time.time() % 60
                ds_fft.data["x"] = x_fftdata
                ds_fft.data["y"] = y_fftdata
                ds_fft._dirty = True
                cursession().store_objects(ds_fft)

                # Plotting only the last MAX_DATA samples
                if len(x_fftdata) > MAX_DATA:
                    x_fftdata.pop(0)
                    y_fftdata.pop(0)
                x_fftdata.append(time_ref)

                y_fftdata.append(data)

 
                time.sleep(0.1)


# plots output spl
def graph3():

    MAX_DATA = 40

    x_psddata = []
    y_psddata = []

    output_server("multiple_updateable_plots")

    # Set up third plot
    p3 = figure(title='SPL of Output', x_range=[0, 4000])
    p3.line(x_psddata, y_psddata,
            color="#0000FF",
            tools="pan,resize,wheel_zoom", width=1200, height=300)

    # Show plots!
    show(p3)
    # Set up the dynamic plotting
    renderer_psd = p3.select(dict(type=GlyphRenderer))
    ds_psd = renderer_psd[0].data_source
    # child.terminate()
    while True:
        #time.sleep(10)
        #neg = child.stdout.read(1)
        # graph the first thing
        try:
            psd = open("../streams/powerspec.txt", 'r')
            for r in psd:
                try:
                    data = r.split(",")
                    x = float(data[0])
                    y = float(data[1])
                except:
                    continue
                ds_psd.data["x"] = x_psddata
                ds_psd.data["y"] = y_psddata
                ds_psd._dirty = True
                cursession().store_objects(ds_psd)

                # Plotting only the last MAX_DATA samples
                if len(x_psddata) > MAX_DATA:
                    x_psddata.pop(0)
                    y_psddata.pop(0)
                x_psddata.append(x)

                y_psddata.append(y)

                time.sleep(0.1)
        except:
            # print "waiting the bat.."
            # Modify (seconds) to se sufficient with your bat to be ready
            
            continue
