set terminal png
set datafile separator ","
set output 'results.png'
plot 'data.txt' using 1:2 w lp title 'base', \
'' using 1:3 w lp title 'cmp_0.csv', \
'' using 1:4 w lp title 'cmp_1.csv', \
'' using 1:5 w lp title 'cmp_3.csv', \
'' using 1:6 w lp title 'cmp_4.csv', \
'' using 1:7 w lp title 'cmp_5.csv', \
