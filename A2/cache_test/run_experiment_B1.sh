
for ((i = 1 ; i <= 1024 ; i *= 2 )); do
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i
done

for ((i = 1024 ; i <= 64 * 1024 ; i += 1024 )); do
    perf stat -e cache-misses,cache-references ./cache_test -c 1 -k $i
done

