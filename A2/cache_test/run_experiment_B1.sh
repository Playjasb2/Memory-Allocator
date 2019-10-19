
make clear

# loop through kb
for ((i = 2 ; i <= 1024 ; i += 2 )); do
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i -f cache_bandwidth_kb.csv
done

# loop through mb
for ((i = 1024 ; i <= 32 * 1024 ; i += 512 )); do
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i  -f cache_bandwidth_mb.csv
done

echo "
    set terminal png
    set datafile separator ','
    set output 'cache_bandwidth_kb.png'
    set title 'Cache Bandwidth (KB)'
    set xlabel 'Blocksize (KB)'
    set ylabel 'Bandwidth (GB/s)'
    plot 'cache_bandwidth_kb.csv' using 1:2 w lp title 'Bandwidth'
" | gnuplot

echo "
    set terminal png
    set datafile separator ','
    set output 'cache_bandwidth_mb.png'
    set title 'Cache Bandwidth (MB)'
    set xlabel 'Blocksize (KB)'
    set ylabel 'Bandwidth (GB/s)'
    plot 'cache_bandwidth_mb.csv' using 1:2 w lp title 'Bandwidth'
" | gnuplot
