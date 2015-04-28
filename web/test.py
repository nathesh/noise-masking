import subprocess
cmd = ["../exe/mask"]
# The following object represents our masker thread
child = subprocess.Popen(cmd, stdout=subprocess.PIPE, bufsize=0)
while(True):
    with open("../streams/powerspec.txt", 'r') as psd:
        for r in psd:
            try:
                data = r.split(",")
                print 'data:', data
                x = float(data[0])
                y = float(data[1])
            except IndexError,ValueError:
                continue
