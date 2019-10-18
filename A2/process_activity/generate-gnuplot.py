import sys

def parse_data(data):
    dataLines = data.split("\n")

    activeData = []
    inactiveData = []

    for dataLine in dataLines:
        currentLineArray = dataLine.split()

        if currentLineArray == [] or currentLineArray[0] not in ["Active", "Inactive"]:
            continue

        term = float(currentLineArray[-2][1:])
        
        if currentLineArray[0] == "Active":
            activeData.append(term)
        else:
            inactiveData.append(term)
    
    return activeData, inactiveData



if __name__ == "__main__":

    data = sys.stdin.read()

    activeData, inactiveData = parse_data(data)

    maxDuration = sum(activeData) + sum(inactiveData)

    activeDataTimeStamps = []
    inactiveDataTimeStamps = []

    startingTime = activeData[0] + inactiveData[0]

    for i in range(1,len(activeData)):
        activeDataTimeStamps.append((startingTime, startingTime + activeData[i]))

        startingTime += activeData[i]

        inactiveDataTimeStamps.append((startingTime, startingTime + inactiveData[i]))

        startingTime += inactiveData[i]
    

    file = open("test.gnuplot", 'w+')
    
    file.write("set terminal png\n")
    file.write("set output \"test.png\"\n")
    file.write("set title \"Active and Inactive Periods\"\n")
    file.write("set xlabel \"Time (ms)\"\n")
    file.write("unset ytics\n")

    file.write("set key box\n")

    for i in range(len(activeDataTimeStamps)):
        command = "set object " + str(2*i+1) + " rect from "
        command += str(activeDataTimeStamps[i][0]) + ", " + "1 "
        command += " to " + str(activeDataTimeStamps[i][1]) + ", " + "2 "
        command += "fc rgb \"blue\" fs solid\n"
        file.write(command)

        command = "set object " + str(2*i+2) + " rect from "
        command += str(inactiveDataTimeStamps[i][0]) + ", " + "1 "
        command += "to " + str(inactiveDataTimeStamps[i][1]) + ", " + "2 "
        command += "fc rgb \"red\" fs solid\n"
        file.write(command)

    file.write("plot [0:" + str(maxDuration) + "] [0:3] 0 notitle, NaN with points pt 5 lc rgb \"blue\" title \"Active\", NaN with points pt 5 lc rgb \"red\" title \"Inactive\"")

    file.close()




    






    