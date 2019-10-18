
rm -f *.csv

num_cores=$(nproc)

for ((c = 0 ; c < $num_cores ; c++ )); do
    if [[ $c -eq $1 ]]; then
        continue
    fi

    echo "compare to ${c}"

    ./cache_test -c $1 -k 1024 -f "base.csv"

    # loop through kb
    for ((i = 1 ; i <= 1024 ; i *= 2 )); do
        echo "core ${c} size ${i}"
        ./cache_test -c $c -k $i &
        P1=$!
        ./cache_test -c $1 -k $i -f "cmp_${c}.csv" &
        P2=$!
        wait $P1 $P2
    done

    # loop through mb
    for ((i = 1024 ; i <= 16 * 1024 ; i += 1024 )); do
        echo "core ${c} size ${i}"
        ./cache_test -c $c -k $i &
        P1=$!
        ./cache_test -c $1 -k $i -f "cmp_${c}.csv" &
        P2=$!
        wait $P1 $P2
    done

done
