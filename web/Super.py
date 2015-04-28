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


def random_gen():
    while True:
        val = random.random()
        yield val
        time.sleep(0.1)


def run():
    cmd = ["../exe/mask"]
    # The following object represents our masker thread
    child = subprocess.Popen(cmd, stdout=subprocess.PIPE, bufsize=0)

    N = 200
    MAX_DATA = N  # plot only last 200 samples
    x = np.linspace(0, N, N)
    y = np.sin(x)

    output_server("line_animate")

    p = figure(title='Input Signal')

    p.line(x, y, color="#3333ee", name="plot")

    show(p)

    # get renderer from object by tag name
    renderer = p.select(dict(name="plot"))
    # data from object
    ds = renderer[0].data_source

    while True:
        for i in np.hstack((np.linspace(1, -1, 300), np.linspace(-1, 1, 300))):
            # update the value of the object
            ds.data["y"] = np.sin(x * i)
            cursession().store_objects(ds)
            time.sleep(0.05)

    # Poll for data in buffer
    # i = 0
    # x = 0
    # while True:
    #     neg = child.stdout.read(1)
    #     sec = 0.0
    #     if str(neg) == '-':
    #         sec = float(neg + child.stdout.read(6))
    #     else:
    #         sec = (neg + child.stdout.read(5))
    #         sec = float(sec)
    #     xn = sec
    # print xn
    # plt.ylim([.005-xn,.005+xn])
    #     time.sleep(0.1)
    #     plt.pause(0.0001)
    # yn = child.stdout.read(1)
    # Buffer is empty and child is finished
    # This will print '%c'b'' This is pythons notation for
    # byte strings but the extra formatting is not really there
    # byte = 'a'
    # send data to plotly

    #     i +=1

    # close the stream when done plotting
    # s.close()


def test():
    from bokeh.plotting import *
    import random
    import time
    from bokeh.models.renderers import GlyphRenderer
    cmd = ["../exe/mask"]
    # The following object represents our masker thread
    child = subprocess.Popen(cmd, bufsize=0)

    MAX_DATA = 60

    # We will store the data here
    x_inputdata = []
    y_inputdata = []

    x_fftdata = []
    y_fftdata = []

    x_psddata = []
    y_psddata = []

    output_server("multiple_updateable_plots")

    # Set up first plot
    p1 = figure(title="Input Signal")
    p1.line(x_inputdata, y_inputdata,
            color="#0000FF",
            tools="pan,resize,wheel_zoom", width=1200, height=300)

    # Set up second plot
    p2 = figure(title="FFT of Input")
    p2.line(x_fftdata, y_fftdata,
            color="#0000FF",
            tools="pan,resize,wheel_zoom", width=1200, height=300)

    # Set up third plot
    p3 = figure(title='SPL of Output')
    p3.line(x_psddata, y_psddata,
            color="#0000FF",
            tools="pan,resize,wheel_zoom", width=1200, height=300)

    # Show plots!
    show(p1)
    # Set up the dynamic plotting
    renderer_input = p1.select(dict(type=GlyphRenderer))
    ds_input = renderer_input[0].data_source

    renderer_fft = p2.select(dict(type=GlyphRenderer))
    ds_fft = renderer_fft[0].data_source

    renderer_psd = p3.select(dict(type=GlyphRenderer))
    ds_psd = renderer_psd[0].data_source
    #child.terminate()
    while True:
        #neg = child.stdout.read(1)
        # graph the first thing
        '''
        with open("../streams/inputsignal.txt", 'r') as f:
            for r in f:
                try:
                    data = float(r)
                except ValueError:
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

        with open("../streams/fftw.txt", 'r') as fft:
            for r in fft:
                try:
                    data = float(r)
                except ValueError:
                    continue
                time_ref = time.time() % 60
                ds_fft.data["x"] = x_fftdata
                ds_fft.data["y"] = y_fftdata
                ds_fft._dirty = True
                cursession().store_objects(ds_input)

                # Plotting only the last MAX_DATA samples
                if len(x_fftdata) > MAX_DATA:
                    x_fftdata.pop(0)
                    y_fftdatata.pop(0)
                x_fftdata.append(time_ref)

                y_fftdata.append(data)

                time.sleep(0.1)
        '''
        with open("../streams/powerspec.txt", 'r') as psd:
            for r in psd:
                try:
                    data = r.split(",")
                    x = float(data[0])
                    y = floa(data[1])
                except ValueError,IndexError:
                    continue
                time_ref = time.time() % 60
                ds_psd.data["x"] = x_psddata
                ds_psd.data["y"] = y_psddata
                ds_psd._dirty = True
                cursession().store_objects(ds_input)

                # Plotting only the last MAX_DATA samples
                if len(x_psddata) > MAX_DATA:
                    x_psddata.pop(0)
                    y_psddata.pop(0)
                x_psddata.append(x)

                y_fftdata.append(y)

                time.sleep(0.1)
