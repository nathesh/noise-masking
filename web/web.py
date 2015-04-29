from jinja2 import TemplateNotFound
import Super
from Super import test,run
import threading
import subprocess
import os
import sys
from flask import Flask, render_template,  make_response
from flask import abort 
from flask import request, Response
app = Flask(__name__)
app.debug = True


@app.route('/',methods=['POST','GET'])
def index():
    return render_template('index.html')

@app.route('/in_signal')
def graph1():
    return Super.graph1() # return input signal

@app.route('/in_fft')
def graph2():
	return Super.graph2() # return input FFT

@app.route('/out_spl')
def graph3():
	return Super.graph3() # return output SPL
    

if __name__=='__main__':
    app.run(host='0.0.0.0')
