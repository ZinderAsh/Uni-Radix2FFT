# Helper script to plot FFT timing using gnuplot
# See `run.sh` for usage and `make run` for automatically
# testing and plotting your results
set terminal pngcairo size 640, 480
set output "time_diff.png"
set autoscale
set title "Time difference between Naive and FFTW"
set xlabel "Number of elements"
set ylabel "Time (usec)"
# set logscale y
set logscale x 2

set style data linespoints

plot "200.dat" using 1:2 title "200", \
     "210.dat" using 1:3 title "210", \
     "220.dat" using 1:3 title "220", \
     "230.dat" using 1:3 title "230", \
     "240.dat" using 1:3 title "240", \
     "250.dat" using 1:3 title "250", \
     "256.dat" using 1:3 title "256"
