
make clear

num_cores=$(nproc)

# perform the base calculations
# loop through kb
for ((i = 1 ; i < 1024 ; i *= 2 )); do
    echo -n "$i," >> "base.csv"
    perf stat -e cache-misses,cache-references ./cache_test -c $1 -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' >> "base.csv"
done

# then compare to each other core
# loop through mb
for ((i = 1024 ; i <= 16 * 1024 ; i += 1024 )); do
    echo -n "$i," >> "base.csv"
    perf stat -e cache-misses,cache-references ./cache_test -c $1 -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' >> "base.csv"
done

for ((c = 0 ; c < $num_cores ; c++ )); do
    if [[ $c -eq $1 ]]; then
        continue
    fi

    echo "compare to ${c}"

    # loop through kb
    for ((i = 1 ; i <= 1024 ; i *= 2 )); do
        echo "core ${c} size ${i}"
        echo -n "$i," >> "cmp_${c}.csv"
        perf stat -e cache-misses,cache-references ./cache_test -c $1 -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' >> "cmp_${c}.csv" &
        P1=$!
        perf stat -e cache-misses,cache-references ./cache_test -c $c -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' &
        P2=$!
        wait $P1 $P2
    done

    # loop through mb
    for ((i = 1024 ; i <= 16 * 1024 ; i += 1024 )); do
        echo "core ${c} size ${i}"
        echo -n "$i," >> "cmp_${c}.csv"
        perf stat -e cache-misses,cache-references ./cache_test -c $1 -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' >> "cmp_${c}.csv" &
        P1=$!
        perf stat -e cache-misses,cache-references ./cache_test -c $c -k $i 2>&1 | grep 'cache-misses' | awk '{print $4;}' &
        P2=$!
        wait $P1 $P2
    done
done

python merge_results.py
gnuplot graph_kb.gp
gnuplot graph_mb.gp
