
import glob

# find all the comparison files
cmp_file_names = glob.glob("cmp_*.csv")

if cmp_file_names is not None:
    cmp_file_names.sort()

# open the base file
base_file = open("base.csv", "r")

# open the comparison files
cmp_files = []
for cmp_file_name in cmp_file_names:
    cmp_files.append(open(cmp_file_name, "r"))

# merge all the data into a single data file
data_out = open("data.txt", "w")
for base_line in base_file:
    str = base_line.replace('\n', '')
    for cmp_file in cmp_files:
        x, y = cmp_file.readline().replace('\n', '').split(',')
        str += "," + y
    print(str)
    data_out.write(str + "\n")

data_out.close()
base_file.close()
for cmp_file in cmp_files:
    cmp_file.close()

# generate the plot file
gp_out = open("plot.gp", "w")
gp_out.write("set terminal png\n")
gp_out.write("set datafile separator \",\"\n")
gp_out.write("set output 'results.png'\n")
gp_out.write("plot 'data.txt' using 1:2 w lp title 'base', \\\n")

for i in range(0, len(cmp_files)):
    gp_out.write("'' using 1:{} w lp title '{}', \\\n".format(3 + i, cmp_file_names[i]))

gp_out.close()
