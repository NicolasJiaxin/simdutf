#include "simdutf.h"

#include <array>
#include <algorithm>
#include <string.h>

#include "helpers/random_utf8.h"
#include <tests/helpers/test.h>
#include <fstream>
#include <iostream>
#include <memory>

std::vector<size_t> sizes = {32768};//{8192, 16384, 65536, 102400, 256000, 512000, 1048576, 2097152, 4194304, 10485760};
std::vector<std::string> names = {"32KB"};//{"8KB", "16KB", "64KB", "100KB", "250KB", "500KB", "1MB", "2MB", "4MB", "10MB"};


bool write_to_file_descriptor(std::FILE *fp, const char * data, size_t length) {
  if(fp == NULL) { return false; }
  size_t bytes_written = std::fwrite(data, 1, length, fp);
  if (bytes_written != length) { return false; }
  return true;
}

int main() {
  ASSERT_TRUE(sizes.size() == names.size());
  simdutf::tests::helpers::random_utf8 generator{1234, 1, 1, 1, 1};
  for(int i = 0; i < sizes.size(); i++) {
    std::FILE *fp = std::fopen(names[i].c_str(), "wb");
    auto data = generator.generate(sizes[i]);
    write_to_file_descriptor(fp, reinterpret_cast<const char *>(data.data()), data.size());
  }
}