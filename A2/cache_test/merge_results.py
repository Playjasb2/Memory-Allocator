
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
data_kb = open("data_kb.csv", "w")
data_mb = open("data_mb.csv", "w")
for base_line in base_file:
    str = base_line.replace('\n', '')
    for cmp_file in cmp_files:
        x, y = cmp_file.readline().replace('\n', '').split(',')
        str += "," + y
    str += "\n"

    block_size = int(str.split(',')[0])
    if block_size <= 1024:
        data_kb.write(str)
    if block_size >= 1024:
        data_mb.write(str)

data_kb.close()
data_mb.close()
base_file.close()
for cmp_file in cmp_files:
    cmp_file.close()

# generate the plot file
def write_gp_file(f_name, f_input, f_output, title, x_label, y_label):
    gp_out = open(f_name, "w")
    gp_out.write("set terminal png\n")
    gp_out.write("set datafile separator \",\"\n")
    gp_out.write("set output '{}'\n".format(f_output))
    gp_out.write("set title '{}'\n".format(title))
    gp_out.write("set xlabel '{}'\n".format(x_label))
    gp_out.write("set ylabel '{}'\n".format(y_label))
    gp_out.write("plot '{}' using 1:2 w lp title 'base', \\\n".format(f_input))

    for i in range(0, len(cmp_files)):
        gp_out.write("'' using 1:{} w lp title '{}', \\\n".format(3 + i, cmp_file_names[i]))

    gp_out.close()

write_gp_file("graph_kb.gp", "data_kb.csv", "results_kb.png", "shared bandwidth (KB)", "blocksize (KB)", "bandwidth (GB/s)")
write_gp_file("graph_mb.gp", "data_mb.csv", "results_mb.png", "shared bandwidth (MB)", "blocksize (MB)", "bandwidth (GB/s)")
