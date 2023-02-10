#ifndef __FACADE_UTILITY_HPP
#define __FACADE_UTILITY_HPP

//! @file utility.hpp
//! @brief Functions relating to the needs of images in the program.
//!
//! This file features code related to various functionality of the program, such as PNG's needs for compression
//! utilities and CRC32 calculation.
//!

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cctype>
#include <string>
#include <vector>

#include <zlib.h>

#include <facade/platform.hpp>

namespace facade
{
   /// @brief The table used for calculating a CRC32 value.
   const std::uint32_t CRC32_TABLE[256] = { 
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
      0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
      0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
      0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
      0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
      0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
      0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
      0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
      0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
      0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
      0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
      0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
      0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
      0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
      0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
      0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
      0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
      0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
      0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
      0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
      0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
      0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
      0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
      0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
      0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
      0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
      0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
      0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
      0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
      0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
      0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
      0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
      0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
      0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
      0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
      0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
      0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
      0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
      0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
      0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
      0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
      0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
      0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
      0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
      0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
      0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
      0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
      0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
      0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
      0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
      0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
      0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
   };

   /// @brief The alphabet of base64 data.
   const std::string BASE64_ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

   /// @brief Swap the endianness of a 16-bit value.
   ///
   /// This converts big endian to little endian, or little endian to big endian.
   ///
   /// @param value The value whose endianness needs to be swapped.
   /// @return The 16-bit endian-swapped value.
   ///
   EXPORT std::uint16_t endian_swap_16(std::uint16_t value);

   /// @brief Swap the endianness of a 32-bit value.
   ///
   /// This converts big endian to little endian, or little endian to big endian.
   ///
   /// @param value The value whose endianness needs to be swapped.
   /// @return The 32-bit endian-swapped value.
   ///
   EXPORT std::uint32_t endian_swap_32(std::uint32_t value);

   /// @brief Calculate the CRC32 value of a given buffer.
   ///
   /// @param ptr A pointer to the data whose CRC you wish to calculate.
   /// @param size The size, in bytes, contained by the data pointer.
   /// @param init_crc The initial CRC value to use. The default is 0 for an initial value.
   /// @return The calculated CRC32.
   ///
   EXPORT std::uint32_t crc32(const void *ptr, std::size_t size, std::uint32_t init_crc=0);

   /// @brief Compress the given data buffer with the given compression level.
   ///
   /// This utilizes [zlib](https://zlib.net)'s deflate algorithm. For more on that, read *zlib*'s documentation.
   ///
   /// @param ptr The given data pointer to compress.
   /// @param size The size, in bytes, of the given data pointer.
   /// @param level The compression level to pass to the deflate algorithm,
   ///              see [the zlib manual](https://www.zlib.net/manual.html#Basic) for possible values.
   /// @return The compressed buffer.
   /// @throws facade::exception::ZLibError
   ///
   EXPORT std::vector<std::uint8_t> compress(const void *ptr, std::size_t size, int level);
   /// @brief Compress the given byte vector with the given compression level.
   ///
   /// @param vec The byte vector to compress.
   /// @param level The compression level to give to *deflate*,
   ///              see [the zlib manual](https://www.zlib.net/manual.html#Basic) for possible values.  
   /// @sa The root compression function: compress(const void *, std::size_t, int)
   ///
   EXPORT std::vector<std::uint8_t> compress(const std::vector<std::uint8_t> &vec, int level);
   /// @brief Decompress the given data buffer with [zlib](https://zlib.net)'s inflate algorithm.
   /// @param ptr The compressed data pointer to decompress.
   /// @param size The size, in bytes, of the data pointer.
   /// @return The decompressed buffer.
   /// @throws facade::exception::ZLibError
   ///
   EXPORT std::vector<std::uint8_t> decompress(const void *ptr, std::size_t size);
   /// @brief Decompress the given data vector with [zlib](https://zlib.net)'s inflate algorithm.
   /// @param vec The compressed data vector.
   /// @return The decompressed buffer.
   /// @throws facade::exception::ZLibError
   /// @sa The root decompress function: decompress(const void *, std::size_t)
   ///
   EXPORT std::vector<std::uint8_t> decompress(const std::vector<std::uint8_t> &vec);

   /// @brief Determine if the string is a base64 string.
   /// @param base64 The string of (alleged) base64 data.
   /// @return Whether or not this string is base64 data.
   ///
   EXPORT bool is_base64_string(const std::string &base64);
   /// @brief Base64 encode the given buffer data.
   /// @param ptr The data buffer to encode.
   /// @param size The size, in bytes, of the given pointer.
   /// @return A base64-encoded string.
   ///
   EXPORT std::string base64_encode(const void *ptr, std::size_t size);
   /// @brief Base64 encode the given data vector.
   /// @param data The vector of bytes to encode.
   /// @return A base64-encoded string.
   ///
   EXPORT std::string base64_encode(const std::vector<std::uint8_t> &data);
   /// @brief Base64-decode the given string into a byte vector.
   /// @param data The base64-encoded string.
   /// @return The decoded byte buffer.
   /// @throws facade::exception::InvalidBase64Character
   ///
   EXPORT std::vector<std::uint8_t> base64_decode(const std::string &data);

   /// @brief Read a file into a byte vector.
   /// @param filename The given filename to read.
   /// @return The byte vector representing the file data.
   /// @throws facade::exception::OpenFileFailure
   ///
   EXPORT std::vector<std::uint8_t> read_file(const std::string &filename);
   /// @brief Write a given buffer to the given filename.
   /// @param filename The filename to save to.
   /// @param ptr The data buffer to write.
   /// @param size The size, in bytes, of the pointed data buffer.
   /// @throws facade::exception::OpenFileFailure
   ///
   EXPORT void write_file(const std::string &filename, const void *ptr, std::size_t size);
   /// @brief Write a given byte vector to the given filename.
   /// @param filename The filename to save to.
   /// @param vec The byte vector to write.
   /// @throws facade::exception::OpenFileFailure
   ///
   EXPORT void write_file(const std::string &filename, const std::vector<std::uint8_t> &vec);
}

#endif
