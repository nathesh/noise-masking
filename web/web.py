from jinja2 import TemplateNotFound

from Super import *
import threading
import subprocess
import os
import sys
from celery import Celery
from flask import Flask, render_template,  make_response
from flask import abort
from flask import request, Response
app = Flask(__name__)
app.debug = True


@app.route('/', methods=['POST', 'GET'])
def index():
    return render_template('index.html')

@app.route('/in_signal')
def create_graph1():
    return graph1()  # return input signal


@app.route('/in_fft')
def create_graph2():
    return graph2()  # return input FFT


@app.route('/out_spl')
def create_graph3():
    return graph3()  # return output SPL


if __name__ == '__main__':
    app.run(host='0.0.0.0', threaded=True)
    celery = Celery(app.name)
