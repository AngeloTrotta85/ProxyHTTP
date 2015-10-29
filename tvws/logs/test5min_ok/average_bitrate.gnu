# Gnuplot script file for plotting data in file "force.dat"
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
#set title "Link Budget"
#set xlabel "Number of repairing units"
set ylabel "Average bitrate"
#set xlabel font ",22"
set ylabel font ",22"

#LEGEND PARAMETERS
set key top left
#set key at 1.2,96
#set key box
#set key font ",24"
#set key spacing 3.5
set key font ",20"
set key spacing 2.5

set terminal postscript eps enhanced color font "Times"
set output "average_bitrate.eps"

set style fill solid border -1
set style histogram cluster gap 1
set style data histogram

#set xrange [0 : 50]
#set yrange [0 : 100]
#set yrange [0 : 100]
set xtics ("TVWS only" 0, "LTE only" 1, "RANDOM" 2, "CABA" 3)

plot \
"data/average_bitrate.dat" using 1 t "Dynamic scenario" fs pattern 7

#plot \
#"data/TheoricalSCD.dat" using 1 t "Theoretical MCC" fs pattern 7, \
#"data/Stem.dat" using 1 t "Distributed MCC" fs pattern 4, \
#"data/Chain.dat" using 1 t "Chain-based" fs pattern 1, \
#"data/Dummy.dat" using 1 t "Greedy" fs pattern 5

