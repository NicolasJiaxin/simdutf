#include "simdutf.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <random>
#include <string>

#include <tests/reference/encode_utf8.h>
#include <tests/helpers/test.h>

uint32_t seed = 123;

std::string message;

extern "C" {
void __asan_on_error() {
  std::cout << "You caused an error: " << message << std::endl;
}
}

/**
 * We should be able to receive random data without any problem
 * when using the validating transcoder. It is difficult to test
 * extensively, but it is easy to try many thousands of random test
 * cases.
 */

namespace {
std::vector<size_t> input_size{7, 16, 12, 64, 67, 128, 129, 256, 1024, 1025};
}

//  Possible states.
//  Format: xxx_yyy where xxx is the number of bytes (in UTF-8) and yyy is the error encoded (if any).
enum state {
  ONE_VALID = 0,
  ONE_TOO_LONG,
  ONE_TOO_LARGE,
  TWO_VALID,  // 3
  TWO_HEADER,
  TWO_TOO_SHORT,
  TWO_TOO_LONG,
  TWO_OVERLONG,
  THREE_VALID,  // 8
  THREE_HEADER,
  THREE_TOO_SHORT,
  THREE_TOO_LONG,
  THREE_OVERLONG,
  THREE_SURROGATE,
  FOUR_VALID, // 14
  FOUR_HEADER,
  FOUR_TOO_SHORT,
  FOUR_TOO_LONG,
  FOUR_OVERLONG,
  FOUR_TOO_LARGE
};

struct state_tracker {
  private:
  enum state current_state;
  enum state first_error;
  double state_weights[20];
  std::mt19937 gen;

  public:
  state_tracker(uint64_t seed, double onevalid, double onetoolong, double onetoolarge, double twovalid, double twoheader, double twotooshort,
            double twotoolong, double twooverlong, double threevalid, double threeheader, double threetooshort, double threetoolong, double threeoverlong,
            double threesurrogate, double fourvalid, double fourheader, double fourtooshort, double fourtoolong, double fouroverlong, double fourtoolarge) noexcept
      : state_weights{onevalid, onetoolong, onetoolarge, twovalid, twoheader, twotooshort, twotoolong, twooverlong, threevalid, threeheader, threetooshort, threetoolong,
        threeoverlong, threesurrogate, fourvalid, fourheader, fourtooshort, fourtoolong, fouroverlong, fourtoolarge}, gen(std::mt19937::result_type(seed)) {
    current_state = next_state();
    first_error = current_state;
  }

  // valid_weight is the weight of having a valid state (4 valid states) and invalid_weight is the weight of having a invalid state (16 invalid states)
  state_tracker(uint64_t seed, double valid_weight, double invalid_weight) :
                state_tracker(seed, valid_weight/4, invalid_weight/16, invalid_weight/16, valid_weight/4, invalid_weight/16, invalid_weight/16, invalid_weight/16, invalid_weight/16,
                valid_weight/4, invalid_weight/16, invalid_weight/16, invalid_weight/16, invalid_weight/16, invalid_weight/16, valid_weight/4, invalid_weight/16, invalid_weight/16,
                invalid_weight/16, invalid_weight/16, invalid_weight/16) {}

  size_t next(std::vector<char>& output) {
    // Write current state to output
    auto consume = [&output](uint8_t byte) {
      output.push_back(byte);
    };
    size_t count{0};
    switch(current_state)
    {
      case ONE_VALID: {
        simdutf::tests::reference::utf8::encode(generate(0x0, 0x7f), consume);
        count = 1;
        break;
      }
      case ONE_TOO_LONG: {
        simdutf::tests::reference::utf8::encode(generate(0x0, 0x7f), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        count = 2;
        break;
      }
      case ONE_TOO_LARGE: {
        output.push_back(generate(0x80, 0xff));
        count = 1;
        break;
      }
      case TWO_VALID: {
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        count = 2;
        break;
      }
      case TWO_HEADER: {
        uint32_t codepoint = generate(0x80, 0x7ff);
        output.push_back(0xf8 | (codepoint >> 6));  // Corrupt leading byte
        output.push_back(0x80 | (codepoint & 0x3f));
        count = 2;
        break;
      }
      case TWO_TOO_SHORT: {
        output.push_back(generate(0xc1, 0xdf)); // Only produce normal leading byte
        count = 1;
        break;
      }
      case TWO_TOO_LONG: {
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        count = 3;
        break;
      }
      case TWO_OVERLONG: {
        output.push_back(char(0xc0)); // Add "empty" leading byte
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        count = 2;
        break;
      }
      case THREE_VALID: {
        uint32_t codepoint = generate(0x800, 0xffff);
        // Hacky, but there is only a ~3.2% to generate a codepoint in the forbidden range each time
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        count = 3;
        break;
      }
      case THREE_HEADER: {
        uint32_t codepoint = generate(0x800, 0xffff);
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        output.push_back(0xf8 | (codepoint >> 12)); // Corrupt leading byte
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        output.push_back(0x80 | (codepoint & 0x3f));
        count = 3;
        break;
      }
      case THREE_TOO_SHORT: {
        uint32_t codepoint = generate(0x800, 0xffff);
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        output.push_back(0xe0 | (codepoint >> 12)); // Corrupt leading byte
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        count = 2;
        break;
      }
      case THREE_TOO_LONG: {
        uint32_t codepoint = generate(0x800, 0xffff);
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        count = 4;
        break;
      }
      case THREE_OVERLONG: {
        output.push_back(char(0xe0)); // Add "empty" leading byte
        output.push_back(generate(0x80, 0x9f)); // First continuation byte must start by 0x8_ or 0x9_
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        count = 3;
        break;
      }
      case THREE_SURROGATE: {
        simdutf::tests::reference::utf8::encode(generate(0xd800, 0xdfff), consume);
        count = 3;
        break;
      }
      case FOUR_VALID: {
        simdutf::tests::reference::utf8::encode(generate(0x10000, 0x10ffff), consume);
        count = 4;
        break;
      }
      case FOUR_HEADER: {
        uint32_t codepoint = generate(0x10000, 0x10ffff);
        output.push_back(0xf8 | (codepoint >> 18));   // Corrupt leading byte
        output.push_back(0x80 | ((codepoint >> 12) & 0x3f));
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        output.push_back(0x80 | (codepoint & 0x3f));
        count = 4;
        break;
      }
      case FOUR_TOO_SHORT: {
        uint32_t codepoint = generate(0x10000, 0x10ffff);
        output.push_back(0xf0 | (codepoint >> 18));
        output.push_back(0x80 | ((codepoint >> 12) & 0x3f));
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        count = 3;
        break;
      }
      case FOUR_TOO_LONG: {
        simdutf::tests::reference::utf8::encode(generate(0x10000, 0x10ffff), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        count = 5;
        break;
      }
      case FOUR_OVERLONG: {
        output.push_back(char(0xf0)); // Add "empty" leading byte
        output.push_back(generate(0x80, 0x8f)); // First continuation byte must have start by 0x8_
        output.push_back(generate(0x80, 0xbf)); // Add two random continuation bytes
        output.push_back(generate(0x80, 0xbf));
        count = 4;
        break;
      }
      case FOUR_TOO_LARGE: {
        simdutf::tests::reference::utf8::encode(generate(0x110000, 0x1fffff), consume);
        count = 4;
        break;
      }
    }
    if (first_error == ONE_VALID || first_error == TWO_VALID || first_error == THREE_VALID || first_error == FOUR_VALID) {
      if (current_state != ONE_VALID && current_state != TWO_VALID && current_state != THREE_VALID && current_state != FOUR_VALID) {
        first_error = current_state;
      }
    }
    // Move to next state
    next_state();

    return count;
  }

  private:
  uint32_t generate(uint32_t lo, uint32_t hi) {
    return std::uniform_int_distribution<uint32_t>{lo, hi}(gen);
  }

  enum state next_state() {
    return static_cast<state>(std::discrete_distribution<>{state_weights[0], state_weights[1], state_weights[2], state_weights[3], state_weights[4], state_weights[5], state_weights[6], state_weights[7], state_weights[8], state_weights[9],
              state_weights[10], state_weights[11], state_weights[12], state_weights[13], state_weights[14], state_weights[15], state_weights[16], state_weights[17], state_weights[18], state_weights[19]}(gen));
  }
};

TEST(basic_fuzz) {
  size_t counter{0};
  state_tracker tracker(seed, 1, 1);
  while (counter < 100000) {
    for (size_t size : input_size) {
      std::vector<char> input(size);
      std::vector<char> output(4*size);
      while (input.size() < size) {
        tracker.next(input);
      }
      counter++;
      if ((counter % 10000) == 0) {
        printf("-");
        fflush(NULL);
      }
      bool is_ok_utf8 =
          implementation.validate_utf8(input.data(), input.size());
      bool is_ok_utf16 = implementation.validate_utf16le(
          reinterpret_cast<char16_t *>(input.data()),
          input.size() / sizeof(char16_t));
      bool is_ok_utf32 = implementation.validate_utf32(
          reinterpret_cast<char32_t *>(input.data()),
          input.size() / sizeof(char32_t));
      size_t utf8_to_utf16 = implementation.convert_utf8_to_utf16le(
          input.data(), input.size(),
          reinterpret_cast<char16_t *>(output.data()));
      size_t utf8_to_utf32 = implementation.convert_utf8_to_utf32(
          input.data(), input.size(),
          reinterpret_cast<char32_t *>(output.data()));
      size_t utf16_to_utf8 = implementation.convert_utf16le_to_utf8(
          reinterpret_cast<char16_t *>(input.data()),
          input.size() / sizeof(char16_t), output.data());
      size_t utf16_to_utf32 = implementation.convert_utf16le_to_utf32(
          reinterpret_cast<char16_t *>(input.data()),
          input.size() / sizeof(char16_t), reinterpret_cast<char32_t *>(output.data()));
      size_t utf32_to_utf8 = implementation.convert_utf32_to_utf8(
          reinterpret_cast<char32_t *>(input.data()),
          input.size() / sizeof(char32_t), output.data());
      size_t utf32_to_utf16 = implementation.convert_utf32_to_utf16le(
          reinterpret_cast<char32_t *>(input.data()),
          input.size() / sizeof(char32_t), reinterpret_cast<char16_t *>(output.data()));
      if(is_ok_utf8 ? (utf8_to_utf16 == 0 || utf8_to_utf32 == 0) : (utf8_to_utf16 > 0 || utf8_to_utf32 > 0)) {
        std::cout << (is_ok_utf8 ? "UTF-8 is ok" : "UTF-8 is not ok") << std::endl;
        std::cout << " size = " << input.size() << std::endl;
        std::cout << "  implementation.convert_utf8_to_utf16 return " << utf8_to_utf16 << std::endl;
        std::cout << "  implementation.convert_utf8_to_utf32 return " << utf8_to_utf32 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf8 ? (utf8_to_utf16 > 0 && utf8_to_utf32 > 0) : (utf8_to_utf16 == 0 && utf8_to_utf32 == 0));
      if(is_ok_utf16 ? (utf16_to_utf8 == 0 || utf16_to_utf32 == 0) : (utf16_to_utf8 > 0 || utf16_to_utf32 > 0)) {
        std::cout << (is_ok_utf16 ? "UTF-16 is ok" : "UTF-16 is not ok") << std::endl;
        std::cout << " size = " << input.size() / sizeof(char16_t) << std::endl;
        std::cout << "  implementation.convert_utf16_to_utf8 return " << utf16_to_utf8 << std::endl;
        std::cout << "  implementation.convert_utf16_to_utf32 return " << utf16_to_utf32 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf16 ? (utf16_to_utf8 > 0 && utf16_to_utf32 > 0) : (utf16_to_utf8 == 0 && utf16_to_utf32 == 0));
      if(is_ok_utf32 ? (utf32_to_utf8 == 0 || utf32_to_utf16 == 0) : (utf32_to_utf8 > 0 || utf32_to_utf16 > 0)) {
        std::cout << (is_ok_utf32 ? "UTF-32 is ok" : "UTF-32 is not ok") << std::endl;
        std::cout << " size = " << input.size() / sizeof(char32_t) << std::endl;
        std::cout << "  implementation.convert_utf32_to_utf8 return " << utf32_to_utf8 << std::endl;
        std::cout << "  implementation.convert_utf32_to_utf16 return " << utf32_to_utf16 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf32 ? (utf32_to_utf8 > 0 && utf32_to_utf16 > 0) : (utf32_to_utf8 == 0 && utf32_to_utf16 == 0));
    }
  }
}

TEST(overflow_fuzz) {
  size_t counter{0};
  state_tracker tracker(seed, 1, 1);
  while (counter < 100000) {
    for (size_t size : input_size) {
      std::vector<char> input(size);
      std::vector<char> output(4*size);
      while (input.size() < size) {
        tracker.next(input);
      }
      counter++;
      if ((counter % 10000) == 0) {
        printf("-");
        fflush(NULL);
      }
      bool is_ok_utf8 =
          implementation.validate_utf8(input.data(), input.size());
      bool is_ok_utf16 = implementation.validate_utf16le(
          reinterpret_cast<char16_t *>(input.data()),
          input.size() / sizeof(char16_t));
      bool is_ok_utf32 = implementation.validate_utf32(
          reinterpret_cast<char32_t *>(input.data()),
          input.size() / sizeof(char32_t));
      if (is_ok_utf8) {
        size_t expected_length = implementation.utf16_length_from_utf8(input.data(), input.size());
        output.resize(expected_length);
        size_t utf8_to_utf16 = implementation.convert_utf8_to_utf16le(
            input.data(), input.size(),
            reinterpret_cast<char16_t *>(output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == output.size() && expected_length == utf8_to_utf16);

        expected_length = implementation.utf32_length_from_utf8(input.data(), input.size());
        output.resize(expected_length);
        size_t utf8_to_utf32 = implementation.convert_utf8_to_utf32(
            input.data(), input.size(),
            reinterpret_cast<char32_t *>(output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == output.size() && expected_length == utf8_to_utf32);
      }
      if (is_ok_utf16) {
        size_t expected_length = implementation.utf8_length_from_utf16le(reinterpret_cast<char16_t *>(input.data()), input.size() / sizeof(char16_t));
        output.resize(expected_length);
        size_t utf16_to_utf8 = implementation.convert_utf16le_to_utf8(
            reinterpret_cast<char16_t *>(input.data()),
            input.size() / sizeof(char16_t), output.data());
        ASSERT_TRUE(expected_length > 0 && expected_length == output.size() && expected_length == utf16_to_utf8);

        expected_length = implementation.utf32_length_from_utf16le(reinterpret_cast<char16_t *>(input.data()), input.size() / sizeof(char16_t));
        output.resize(expected_length);
        size_t utf16_to_utf32 = implementation.convert_utf16le_to_utf32(
            reinterpret_cast<char16_t *>(input.data()),
            input.size() / sizeof(char16_t), reinterpret_cast<char32_t *>(output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == output.size() && expected_length == utf16_to_utf32);
      }
      if (is_ok_utf32) {
        size_t expected_length = implementation.utf8_length_from_utf32(reinterpret_cast<char32_t *>(input.data()), input.size() / sizeof(char32_t));
        output.resize(expected_length);
        size_t utf32_to_utf8 = implementation.convert_utf32_to_utf8(
            reinterpret_cast<char32_t *>(input.data()),
            input.size() / sizeof(char32_t), output.data());
        ASSERT_TRUE(expected_length > 0 && expected_length == output.size() && expected_length == utf32_to_utf8);

        expected_length = implementation.utf16_length_from_utf32(reinterpret_cast<char32_t *>(input.data()), input.size() / sizeof(char32_t));
        output.resize(expected_length);
        size_t utf32_to_utf16 = implementation.convert_utf32_to_utf16le(
            reinterpret_cast<char32_t *>(input.data()),
            input.size() / sizeof(char32_t), reinterpret_cast<char16_t *>(output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == output.size() && expected_length == utf32_to_utf16);
      }
    }
  }
}


int main(int argc, char *argv[]) {
  if (argc == 2) {
    try {
      seed = std::stoi(argv[1]);
    } catch (const std::exception& e) {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }
  }
  std::mt19937 gen{seed};
  for (int i = 0; i < 20; i++) {
    input_size.push_back(std::uniform_int_distribution<uint32_t>{50, 800}(gen));
  }
  return simdutf::test::main((argc==2) ? 1 : argc, argv);
}