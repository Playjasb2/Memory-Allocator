set terminal postscript eps enhanced color solid colortext 9
set output 'results.png'
plot 'data.csv' using 1:2 w lp title 'base', \
    using 1:3 w lp title 'cmp_0.csv', \
    using 1:3 w lp title 'cmp_2.csv', \
    using 1:3 w lp title 'cmp_3.csv', \
    using 1:3 w lp title 'cmp_4.csv', \
    using 1:3 w lp title 'cmp_5.csv'
