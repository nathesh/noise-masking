import subprocess
import os
import time
cmd = ["../exe/mask"]
# The following object represents our masker thread
child = subprocess.Popen(cmd)
num = 0
print "ENTER the HAO"
while True:
    try:
        f = open("../streams/inputsignal.txt", 'r')
        print 'HERE'
        for r in f:
            try:
                data = float(r)
                print data
            except:

                continue
    except:
        print "waiting the bat.."
        # Modify (seconds) to se sufficient with your bat to be ready
        time.sleep(.1)
        continue
    num +=1
    if num == 5:
        break
    # print 'NUM:',num
child.terminate()
