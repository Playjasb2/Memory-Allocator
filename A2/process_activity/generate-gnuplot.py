def get_data():
    
    data = []

    with open("results.csv", 'r') as file:
        for line in file:
            if "Start" in line or "Finish" in line:
                continue

            dataLine = line.rstrip("\n")
            dataLine = dataLine.split(",")
            data.append((float(dataLine[0]),float(dataLine[1])))
    
    activeData = []
    inactiveData = []

    for i in range(len(data)):
        if i % 2 == 0:
            activeData.append(data[i])
        else:
            inactiveData.append(data[i])

    return activeData, inactiveData




if __name__ == "__main__":

    activeData, inactiveData = get_data()

    maxDuration = inactiveData[-1][1]

    activeData = activeData[1:]
    inactiveData = inactiveData[1:]
    

    file = open("test.gnuplot", 'w+')
    
    file.write("set terminal png\n")
    file.write("set output \"test.png\"\n")
    file.write("set title \"Active and Inactive Periods\"\n")
    file.write("set xlabel \"Time (ms)\"\n")
    file.write("unset ytics\n")

    file.write("set key box\n")

    for i in range(len(activeData)):
        command = "set object " + str(2*i+1) + " rect from "
        command += str(activeData[i][0]) + ", " + "1 "
        command += " to " + str(activeData[i][1]) + ", " + "2 "
        command += "fc rgb \"blue\" fs solid\n"
        file.write(command)

        command = "set object " + str(2*i+2) + " rect from "
        command += str(inactiveData[i][0]) + ", " + "1 "
        command += "to " + str(inactiveData[i][1]) + ", " + "2 "
        command += "fc rgb \"red\" fs solid\n"
        file.write(command)

    file.write("plot [0:" + str(maxDuration) + "] [0:3] 0 notitle, NaN with points pt 5 lc rgb \"blue\" title \"Active\", NaN with points pt 5 lc rgb \"red\" title \"Inactive\"")

    file.close()




    






    