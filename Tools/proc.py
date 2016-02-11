from sys import argv, stdout
from re import match
from math import sqrt, pow

if len(argv) < 2:
	param = "time"
else:
	param = str(argv[1])
    
show_sd = False
if len(argv) >= 3:
    if argv[2] == "sd":
        show_sd = True

def sdeviation(data):
    avg = sum(data)/len(data)
    tot = 0
    for e in data:
        tot += pow((e-avg),2)
    return sqrt(tot/len(data))
    
def iconfidence(data):
    if param != "load":
        return (1.96*sdeviation(data))/sqrt(len(data))
    return 0.0
    
def calcLoad(data, limit):
    tot = 0.0
    for e in data:
        if e < limit:
            tot += 3.0
    return tot

def readFile(t_file):
    try:
        f = open(t_file, "r").read().split("\n")
        tot = []
        for line in f:
            if match("\d+\.\d+\.\d+\.\d+\t\d+.\d+$", line):
                tot.append(float(line.split("\t")[1]))
        return tot
    except:
        return []

stdout.write("Connection Percentage; 1; 2; 4; 8\n")
for p in [5,10,15,20,25,30,35,40,45,50,55,60,75]:
    stdout.write("%d; " % p)
    for s in [1,2,4,8]:
        tot = []
        succ_open = 0
        for e in range(1,11):
            tot_tmp = readFile("12_%d_%d_%d_single.out" % (s, p, e))
            if len(tot_tmp) > 0: 
                tot += tot_tmp
                succ_open += 1
        if (len(tot) > 0):
            if param == "load":
                if succ_open > 0:
                    stdout.write(str(720.0-(calcLoad(tot, 1.0)/succ_open)).replace(".", ","))
            else:
                stdout.write(str(sum(tot)/len(tot)).replace(".", ","))
        if show_sd:
            stdout.write("; ")
            stdout.write(str(iconfidence(tot)).replace(".", ","))
        if s != 8:
            stdout.write("; ")
        else:
            stdout.write("\n")