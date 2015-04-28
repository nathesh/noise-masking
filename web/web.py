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

@app.route('/graph')
def graph():
    return Super.test()
    

if __name__=='__main__':
    app.run(host='0.0.0.0')
