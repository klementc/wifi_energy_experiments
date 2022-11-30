import random
import sys
import numpy

# Small script to generate a csv file with communication information for the experiment
# each record in the file contains the communicating nodes, the size of the communication and its start timestamp
# Specify a seed to reproduce the same file each time, otherwise timestamps and sizes are randomly generated

def genComm(src, size, start):
    return str(str(src)+",OUT,"+str(size)+","+str(start))


if(len(sys.argv) < 11):
    print("usage rand.rand: "+sys.argv[0]+" rand <nbSTA> <minSize> <maxSize> <startTime> <startTimeEnd> <endTime> <minNbMsg> <maxNbMsg> <minIntervalBetweenMsg> <maxIntervalBetweenMsg> <seed>")
    print("usage numpy.norm: "+sys.argv[0]+" norm <nbSTA> <meanSize> <deviation> <startTime> <startTimeEnd> <endTime> <avgMsg> <devNbMsg> <meanIntervalBetweenMsg> <devIntervalBetweenMsg> <seed>")
    exit(1)


# reproducibility
randomL = True
if(sys.argv[1] == "norm"):
  randomL = False

nbSTA = int(sys.argv[2]) # total number of stations
startTime = int(sys.argv[5])
startTimeEnd = int(sys.argv[6])
endTime = int(sys.argv[7])

if(randomL):
    minSize = int(sys.argv[3])
    maxSize = int(sys.argv[4])
    minNbMsg = int(sys.argv[8])
    maxNbMsg = int(sys.argv[9])
    minIntervalBetweenMsg = int(sys.argv[10])
    maxIntervalBetweenMsg = int(sys.argv[11])

    random.seed(sys.argv[12])
else:
    meanSize = int(sys.argv[3])
    deviationSize = int(sys.argv[4])
    meanNbMsg = int(sys.argv[8])
    devNbMsg = int(sys.argv[9])
    meanIntervalBetweenMsg = int(sys.argv[10])
    devIntervalBetweenMsg = int(sys.argv[11])

    numpy.random.seed(int(sys.argv[12]))
    random.seed(sys.argv[12])

for i in range(nbSTA):
    if (randomL):
        nbMsg = random.randint(minNbMsg, maxNbMsg)
    else:
        nbMsg = int(numpy.random.normal(meanNbMsg, devNbMsg, 1)[0])

    timeLastMsg = random.randint(startTime, startTimeEnd)
    for j in range(nbMsg):
        if (randomL):
            comSize = random.randint(minSize, maxSize)
            timeLastMsg = timeLastMsg + random.randint(minIntervalBetweenMsg, maxIntervalBetweenMsg)
        else:
            comSize = int(numpy.random.normal(meanSize, deviationSize, 1)[0])
            timeLastMsg = timeLastMsg + numpy.random.normal(meanIntervalBetweenMsg, devIntervalBetweenMsg, 1)[0]

        if(timeLastMsg < endTime):
            print(genComm(str(i), comSize, timeLastMsg))
