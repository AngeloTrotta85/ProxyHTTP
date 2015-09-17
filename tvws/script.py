#!/usr/bin/python
import math
import numpy as np

def pl2(f,d):
    return 69.55 + 26.16 * math.log10(f) - 13.82 * math.log10(30.) - ((1.1 * math.log10(f) - 0.7) - (1.56 * math.log10(f) - 0.8)) + (44.9 - 6.55 * math.log10(30.)) * math.log10(d) - 5.4 + 2 * ((math.log10(f/28.))**2)

def plnew(f,d):
#    CH = 3.2*(np.log10(11.75)**2)-4.97
    CH = (1.1 * np.log10(f) - 0.7) - (1.56 * np.log10(f) - 0.8)
    a = 69.55 + (26.16 * np.log10(f)) - (13.82 * np.log10(30)) - CH
    b = 44.9 - 6.55 * np.log10(30)
    c = 5.4 + (2 * ((np.log10(f/28.0))**2))
    dc = 40.94 + (4.78 * ((np.log10(f))**2)) - (18.33 * np.log10(f))
    L = 69.55 + 26.16 * np.log10(f) - 13.82 * np.log10(30) + (44.9 - 6.55 * np.log10(30)) * np.log10(d) - CH - 5.4 + 2 *((np.log10(f/28.0))**2)
    L = a + (b * np.log10(d)) - dc
    return (L)

def pllog(f,d):
    return 20 * np.log10(f) + 20 * np.log10(d)

for i in range(1,20):
    print str(i) + " - " + str(plnew(470.0,i)) + " - " + str(plnew(850,i)) + " - " + str(pllog(850,i)) + " - " + str(pllog(1900,i))

def distance(x1,y1,x2,y2):
    return np.sqrt(np.absolute(x1 - x2)**2 + np.absolute(y1 - y2)**2)


bsx = 5000
bsy = 5000
ux = np.random.random_integers(0,8000)
uy = np.random.random_integers(0,8000)
nextx = ux
nexty = uy
speed = 18
prob = 0.6
counter = 5
shadow = 0
ptx = 27
fw = open("trace.sh","w+")
fw.write("./tc.sh set eth0 100000\n")
fw.write("./tc.sh set wlan0 100000\n")
fw.write("sleep 10\n")
for time in range(200):
    # Get the distance
    d = distance(bsx,bsy,ux,uy)

    # Check if inside building
    r = np.random.random()
    counter = counter + 1
    counter = min(counter,5)
    if counter == 5:
        counter = 0
        if r <= prob:
            shadow = 20
        else:
            shadow = 0

    # Move
    if nextx > ux:
        ux = min(nextx, ux + speed)
    elif nextx < ux:
        ux = max(nextx, ux - speed)
    if nexty > uy:
        uy = min(nexty, uy + speed)
    elif nexty < uy:
        uy = max(nexty, uy - speed)

    if nextx == ux and nexty == uy:
        nextx = np.random.random_integers(0,8000)
        nexty = np.random.random_integers(0,8000)

    # Determine pathloss and datarate
    plc = plnew(900,d/1000.)
    plt = plnew(470,d/1000.)
    if shadow > 0:
        plc += 19
        plt += 13
    prxc = ptx - plc
    prxt = ptx - plt

    drc = 33
    drt = 19

    if prxc < -90:
        drc = 0
    elif prxc < -88:
        drc = ((prxc + 90.0) / 2) * (7.5 - 1) + 1
    elif prxc < -82 and prxc > -88:
        drc = ((prxc + 88.0) / 6) * (33 - 7.5) + 7.5
    
    if prxt < -95:
        drt = 0
    elif prxt < -75 and prxt > -95:
        drt = ((prxt + 95.0) / 20) * (19 - 1) + 1

#    print str(ux) + ":" + str(uy) + "(" + str(nextx) + ":" + str(nexty) + ") - " + str(prxc) + " - " + str(prxt) + " - " + str(drc) + " - " + str(drt)

    # Write to file
    tceth0 = 100
    if drc >= 1:
        tceth0 = ((drc - 1) * 13000) + 16000

    tcwlan0 = 100
    if drt >= 1:
        tcwlan0 = ((drt - 1) * 13000) + 16000

    fw.write("./tc.sh set eth0 " + str(tceth0) + "\n")
    fw.write("./tc.sh set wlan0 " + str(tcwlan0) + "\n")
    fw.write("sleep 4\n")

fw.close()
