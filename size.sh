files=("32KB")
#files=("8KB" "16KB" "64KB" "100KB" "250KB" "500KB" "1MB" "2MB" "4MB" "10MB")

for name in ${files[@]}; do
  hyperfine "./build/tools/sutf -f UTF-8 -t UTF-16LE $name -o /dev/null" --warmup 100 -N >> sutfsize.txt 2> /dev/null
  hyperfine "iconv -f UTF-8 -t UTF-16LE $name -o /dev/null" --warmup 100 -N >> iconvsize.txt 2> /dev/null
done