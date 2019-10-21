
make clean
make

# loop through kb
for ((i = 256 ; i <= 1024 ; i += 16 )); do
    echo "block size = ${i} kb"
    echo -n "$i," >> miss_rate_kb.csv
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' >> miss_rate_kb.csv
done

# loop through mb
for ((i = 1024 ; i <= 32 * 1024 ; i += 512 )); do
    echo "block size = ${i} mb"
    echo -n "$i," >> miss_rate_mb.csv
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' >> miss_rate_mb.csv
done

echo "
    set terminal png
    set datafile separator ','
    set output 'cache_miss_rate_kb.png'
    set xrange [256:1024]
    set xtics 256,64
    set title 'Miss Rate (KB)'
    set xlabel 'Blocksize (KB)'
    set ylabel 'Miss Rate (%)'
    plot 'miss_rate_kb.csv' using 1:2 w lp title 'Miss Rate'
" | gnuplot

echo "
    set terminal png
    set datafile separator ','
    set output 'cache_miss_rate_mb.png'
    set xrange [1024:32768]
    set xtics 2048,2048
    set xtics rotate
    set title 'Miss Rate (MB)'
    set xlabel 'Blocksize (KB)'
    set ylabel 'Miss Rate (%)'
    plot 'miss_rate_mb.csv' using 1:2 w lp title 'Miss Rate'
" | gnuplot
