#!/usr/bin/python3
import sys

ff = open(sys.argv[1])
counter1 = 0
counter2 = 0
if1 = -1
if2 = -1
first = True
for line in ff.readlines():
    if first:
        first = False
        continue
    ll = line.split()
#    print("IF1 = " + str(if1) + ", IF2 = " + str(if2))
    if if1 == -1 and if2 == -1:
        if ll[6].startswith('130'):
            if1 = ll[6]
        else:
            if2 = ll[6]
    if ll[6] != if1 and ll[6] != if2:
        if if1 == -1:
            if1 = ll[6]
        else:
            if2 = ll[6]
#    if (if2 == -1 or if1 == -1) and (if1 != ll[6] or if2 != ll[6]):
    if ll[6] == if1:
        counter1 += 1
    else:
        counter2 += 1
    print(ll[0] + " " + str(counter1 / (counter1 + counter2)))

ff.close()
