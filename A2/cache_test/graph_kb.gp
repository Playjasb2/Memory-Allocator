set terminal png
set datafile separator ","
set output 'shared_cache_kb.png'
set title 'Shared Cache Measurement (KB)'
set xlabel 'Blocksize (KB)'
set ylabel 'Miss Rate (%)'
plot 'data_kb.csv' using 1:2 w lp title 'base', \
'' using 1:3 w lp title 'cmp_0.csv', \
'' using 1:4 w lp title 'cmp_2.csv', \
'' using 1:5 w lp title 'cmp_3.csv', \
'' using 1:6 w lp title 'cmp_4.csv', \
'' using 1:7 w lp title 'cmp_5.csv', \
