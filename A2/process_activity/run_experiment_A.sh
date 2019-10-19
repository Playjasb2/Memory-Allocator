
make clear
make all

./inactive_periods $1
python generate-gnuplot.py
gnuplot results.gnuplot
