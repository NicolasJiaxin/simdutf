#!/bin/bash


for i in `ls ~/unicode_lipsum/lipsum/* | grep utf32`
do
    hyperfine "./build/tools/sutf -f UTF-32LE -t UTF-16LE $i -o /dev/null" --warmup 100 -N
done > sutf.txt 2> /dev/null

for i in `ls ~/unicode_lipsum/lipsum/* | grep utf32`
do
    hyperfine "iconv -f UTF-32LE -t UTF-16LE $i -o /dev/null" --warmup 100 -N
done > iconv.txt 2> /dev/null

python3 avg.py sutf.txt
python3 avg.py iconv.txt