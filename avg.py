import sys
total = 0
count = 0

data = open(sys.argv[1])
next(data)
for line in data:
    line = line.strip()
    a = line.split(" ")
    if(a[0] == 'Time'):
        total += float(a[10])
        count += 1
    data.readline()
    data.readline()
    data.readline()

print("Average " + sys.argv[1] + ": " + str(total / count))

#for i in `ls ~/unicode_lipsum/lipsum/* | grep utf8`; do hyperfine "iconv -f UTF-8 -t UTF-16LE $i -o /dev/null" --warmup 100 -N; done > iconv.txt
#python3 avg.py