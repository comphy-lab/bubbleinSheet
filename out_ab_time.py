# To out major axis a, and minor axis b, over time

import numpy as np
import os
import subprocess as sp

def getting_ab(filename):
    exe = ["./getab", filename]
    p = sp.Popen(exe, stdout=sp.PIPE, stderr=sp.PIPE)
    stdout, stderr = p.communicate()
    temp1 = stderr.decode("utf-8")
    temp2 = temp1.split("\n")
    temp3 = temp2[0].split(" ")
    return float(temp3[0]), float(temp3[1]), float(temp3[2]), float(temp3[3]), float(temp3[4])
# ----------------------------------------------------------------------------------------------------------------------


nGFS = 10000
tsnap = 0.1
    
for ti in range(nGFS):
    t = tsnap*ti
    place = "intermediate/snapshot-%5.4f" % t

    if not os.path.exists(place):
        print("%s File not found!" % place)
    else:
            tp, xmax, y_xmax, x_ymax, ymax = getting_ab(place) # time, number od drops, volume, x, y
            print(tp)
            print("Time: %4.3f, xmax: %4.3f, y_xmax: %4.3f, x_ymax: %4.3f, ymax: %4.3f" % (tp, xmax, y_xmax, x_ymax, ymax))
            
            f = open("out_ab_time.txt", "a")
            f.write("%4.6f"  % (tp))
            f.write("\t")
            f.write("%4.6f"  % (xmax))
            f.write("\t")
            f.write("%4.6f"  % (y_xmax))
            f.write("\t")
            f.write("%4.6f"  % (x_ymax))
            f.write("\t")
            f.write("%4.6f"  % (ymax))
            f.write("\t")
            f.write("\n")

f.close()
