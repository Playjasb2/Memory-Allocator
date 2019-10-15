
all:
	gcc invalid_periods.c tsc.c -o invalid_periods

run:
	numactl -l --physcpubind=1 ./invalid_periods 50
