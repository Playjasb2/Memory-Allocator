
if [ "$#" -ne 1 ]; then
    echo "correct usage: ./run_experiment_A.sh <number of inactive periods>"
    exit 1
fi

make clear
make all

./inactive_periods $1
python generate-gnuplot.py
gnuplot results.gnuplot
