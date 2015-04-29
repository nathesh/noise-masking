import subprocess
import os
import time
cmd = ["../exe/mask"]
# The following object represents our masker thread
child = subprocess.Popen(cmd)
num = 0
while(True):
    try:
        psd = open("../streams/powerspec.txt", 'r')
        num = +1
        for r in psd:
            #print 'TEST'
            try:
                data = r.split(",")
                #print 'data:', data
                x = float(data[0])
                y = float(data[1])
                print "Freq:", x, "Power:",y
            except:
                continue
    except:
        print "waiting the bat.."
        time.sleep(.1) # Modify (seconds) to se sufficient with your bat to be ready
        continue
    if num == 5:
        break
    #print 'NUM:',num
child.terminate()