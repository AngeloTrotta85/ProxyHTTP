#!/usr/bin/python3
import sys
ff = open(sys.argv[1],'r')
buckets = 1
time = buckets
summa = 0
counter = 0
for line in ff.readlines():
    ll = line.split(" ")
    if float(ll[0]) < time:
        summa += float(ll[1])
        counter += 1
    else:
        print(str(time) + " " + str((summa*1.0)/counter))
        time += buckets
        summa = float(ll[1])
        counter = 1
    
