files=("16B" "128B" "512B" "1KB" "4KB" "16KB" "100KB" "500KB" "2MB")

for name in ${files[@]}; do
  hyperfine "./build/tools/sutf -f UTF-8 -t UTF-16LE $name -o /dev/null" --warmup 100 -N >> sutfsize.txt 2> /dev/null
  hyperfine "iconv -f UTF-8 -t UTF-16LE $name -o /dev/null" --warmup 100 -N >> iconvsize.txt 2> /dev/null
done