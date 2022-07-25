#include <string>

namespace simdutf {

enum encoding_type {
        UTF16_LE,   // BOM 0xff 0xfe
        UTF16_BE,   // BOM 0xfe 0xff
        UTF32_LE,   // BOM 0xff 0xfe 0x00 0x00
        UTF32_BE,   // BOM 0x00 0x00 0xfe 0xff
        UTF8,       // BOM 0xef 0xbb 0xbf
        unspecified
};

enum endianness {
        LITTLE,
        BIG
};

std::string to_string(encoding_type bom);

// Note that BOM for UTF8 is discouraged.
namespace BOM {

/**
 * Checks for a BOM. If not, returns unspecified
 * @param input         the string to process
 * @param length        the length of the string in words
 * @return the corresponding encoding
 */

encoding_type check_bom(const uint8_t* byte, size_t length);
encoding_type check_bom(const char* byte, size_t length);
/**
 * Returns the size, in bytes, of the BOM for a given encoding type.
 * Note that UTF8 BOM are discouraged.
 * @param bom         the encoding type
 * @return the size in bytes of the corresponding BOM
 */
size_t bom_byte_size(encoding_type bom);

} // BOM namespace
} // simdutf namespace
