set autoscale
unset log
unset label
set xtic auto
set ytic 5

#set title "Connectivity Index with STEM-Mesh"
#set xlabel "TVWS bitrate"
#set ylabel "Repairing units"
set zlabel "Bitrate" rotate left

set title font ",22"
set xlabel font ",22"
set ylabel font ",22"
set zlabel font ",22"

#set xrange [20 : 100]
#set yrange [5 : 25]
#set zrange [0 : 100]

#Legend
set key top left
#set ket at 9,44
set key font ",225
set key spacing 1.75

#set terminal postscript eps enhanced color font "Times"
#set output "3dgraph.eps"

set ticslevel 0.0

#set cntrparam levels auto 1
#set cntrparam levels disc 40,50,60,70,80,90
#set contour case
#set hidden3d back offset 1 trianglepattern 3 undefined 1 altdiagonal bentover

set border 31+32+64+256+512 lw .4
#set grid

set pm3d
set hidden3d

#set dgrid3d 1000,1000 qnorm 2
#set cntrparam bspline

splot "data.dat" using 1:2:3 title '' with lines
