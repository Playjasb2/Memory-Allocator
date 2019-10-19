
make clear

# loop through kb
for ((i = 1 ; i <= 1024 ; i *= 2 )); do
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i
done

# loop through mb
for ((i = 1024 ; i <= 32 * 1024 ; i += 1024 )); do
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i
done

echo "
    set terminal png
    set datafile separator ','
    set output 'cache_size.png'
    set title 'cache bandwidth'
    set xlabel 'blocksize (KB)'
    set ylabel 'bandwidth (GB/s)'
    plot 'results.csv' using 1:2 w lp title 'bandwidth'
" | gnuplot
