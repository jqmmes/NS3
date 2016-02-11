from sys import argv, stdout, stdin
from math import sqrt, pow

data = stdin.read().replace(",", ".").split("\n")
header = data[0].split(";")

if len(argv) < 2:
	param = "time"
else:
	param = str(argv[1])
    
show_sd = False
if len(argv) >= 3:
    if argv[2] == "sd":
        show_sd = True

colors = ["blue", "black", "purple", "green", "orange", "grey", "pink", "yellow"]
marks = ["square", "triangle", "*", "diamond", "square*", "grey", "pink", "yellow"]
c = 0

print "\\begin{figure}[ht]"
print "\minipage{0.5\textwidth}"
print "    \\begin{tikzpicture}[scale=0.7]"
print "      \\begin{axis}["
print "          title={%s}," % header[0] # Download Time / Traffic
print "          xlabel={Number of devices},"
print "          ylabel={%s}," % ("MegaBytes [MB]" if param == "load" else "Time [ms]") # Time [ms] /  MegaBytes [MB]
print "          xmin=0, xmax=17,"
print "          ymin=0, ymax=%d," % (1250 if param == "load" else 12500) # 12500 / 1250
print "          xtick={1,2,4,6,8},"
print "          ytick={0,%s}," % ("250,500,750,1000,1250" if param == "load" else "2500,5000,7500,10000,12500") # 2500,5000,7500,10000,12500 / 250,500,750,1000,1250
print "          ymajorgrids=true,"
print "          grid style=dashed,"
print "          legend pos=north west"
print "        ]"
for row in data[1:]:
    row = row.split(";")
    if len(row) < len(header):
        continue
    print "        \\addplot[color=%s, mark=square]" % (colors[c%len(colors)])
    print "        plot[error bars/.cd, y dir=both, y explicit]"
    print "        coordinates {" 
    i = 0
    for s in header[1:]:
        if row[min((i*2 if show_sd else i)+1, len(row)-1)].strip() == "":
            print "           (%s, 0.0)   +-(1, 0.0)" % s
        else:
            print "           (%s, %0.2f)   +-(1, %0.2f)" % (s, float(row[(i*2 if show_sd else i)+1]), (float(row[i*2+2]) if show_sd else 0.0))
        i += 1
    print "         };"
    print "         \\addlegendentry{%s%%}" % row[0]
    print ""
    c += 1
print "      \end{axis}"
print "    \end{tikzpicture}"
print "  \caption{%s}" % ("Average download time per file." if param != "load" else "Traffic handled by AP.")
print "  \label{fig:simulation_%s}" % ("download_time" if param != "load" else "load")
print "\endminipage"