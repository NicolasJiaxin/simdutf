#include "simdutf.h"

#include <array>
#include <algorithm>
#include <string.h>

#include "helpers/random_utf8.h"
#include <tests/helpers/test.h>
#include <fstream>
#include <iostream>
#include <memory>

std::vector<size_t> sizes = {16, 128, 512, 1024, 4096, 16384, 102400, 512000, 2097152};
std::vector<std::string> names = {"16B", "128B", "512B", "1KB", "4KB", "16KB", "100KB", "500KB", "2MB"};


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