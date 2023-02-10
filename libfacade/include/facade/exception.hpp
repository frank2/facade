#ifndef __FACADE_EXCEPTION_HPP
#define __FACADE_EXCEPTION_HPP

//! @file exception.hpp
//! @brief A collection of exceptions thrown by the library.
//!

#include <cstdint>
#include <exception>
#include <sstream>
#include <string>

namespace facade
{
namespace exception
{
   /// @brief The base exception that all exceptions in this library are built upon.
   ///
   class Exception : public std::exception
   {
   public:
      /// @brief The resulting error string provided by the exception.
      std::string error;

      Exception() : std::exception() {}
      Exception(const std::string &error) : error(error), std::exception() {}

      /// @brief Get a C-string representation of the error. Adds compatibility with std::exception.
      ///
      const char *what() const noexcept {
         return this->error.c_str();
      }
   };

   /// @brief An exception thrown when a provided chunk tag is not valid.
   ///
   /// Typically thrown when a chunk tag is too long in size. Chunk tags can only be 4 ASCII characters long.
   ///
   class InvalidChunkTag : public Exception
   {
   public:
      InvalidChunkTag() : Exception("Invalid chunk tag: the string provided was not a valid chunk tag. "
                                    "Chunk tags can only be 4 characters long.") {}
   };

   /// @brief An exception thrown when the operation goes out of bounds.
   class OutOfBounds : public Exception
   {
   public:
      /// @brief The offending boundary.
      std::size_t given;
      /// @brief The boundary the offense broke.
      std::size_t boundary;

      OutOfBounds(std::size_t given, std::size_t boundary) : given(given), boundary(boundary), Exception() {
         std::stringstream stream;

         stream << "Out of bounds: the given index is "
                << given
                << ", but the boundary is "
                << boundary;

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when encountering a null pointer.
   class NullPointer : public Exception
   {
   public:
      NullPointer() : Exception("Null pointer: encountered a null pointer where it wasn't expected.") {}
   };

   /// @brief An exception thrown when the signature header of a given binary stream is not a PNG header.
   class BadPNGSignature : public Exception
   {
   public:
      BadPNGSignature() : Exception("Bad PNG signature: the signature header of the PNG file was not valid.") {}
   };

   /// @brief An exception thrown when the CRC didn't match expectations.
   class BadCRC : public Exception
   {
   public:
      /// @brief The bad CRC.
      std::uint32_t given;
      /// @brief The expected CRC.
      std::uint32_t expected;

      BadCRC(std::uint32_t given, std::uint32_t expected) : given(given), expected(expected), Exception() {
         std::stringstream stream;

         stream << "Bad CRC: the given CRC was "
                << std::hex << std::showbase << std::uppercase
                << given
                << ", but the expected CRC was "
                << expected;

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when a file couldn't be opened, either for reading or writing.
   class OpenFileFailure : public Exception
   {
   public:
      /// @brief The file which could not be opened.
      std::string filename;

      OpenFileFailure(const std::string &filename) : filename(filename), Exception() {
         std::stringstream stream;

         stream << "Open file failure: failed to open file with name \""
                << filename
                << "\"";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when there was not enough data to complete the operation.
   class InsufficientSize : public Exception
   {
   public:
      /// @brief The offending size.
      std::size_t given;
      /// @brief The minimum size needed to complete the operation.
      std::size_t minimum;

      InsufficientSize(std::size_t given, std::size_t minimum) : given(given), minimum(minimum), Exception() {
         std::stringstream stream;

         stream << "Insufficient size: given a size of "
                << given
                << ", but needed at least "
                << minimum;

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when no header chunk is present in a PNG file.
   class NoHeaderChunk : public Exception
   {
   public:
      NoHeaderChunk() : Exception("No header chunk: the header chunk for the PNG image was not found.") {}
   };

   /// @brief An exception thrown when encountering a ZLib error.
   class ZLibError : public Exception
   {
   public:
      /// @brief The code returned by zlib.
      /// @sa [ZLib documentation](https://www.zlib.net/manual.html#Constants) for error code meanings.
      int code;

      ZLibError(int code) : code(code), Exception() {
         std::stringstream stream;

         stream << "ZLib error: ZLib encountered an error while compressing/decompressiong, error code "
                << code;

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when no image data chunks are present in the PNG file.
   class NoImageDataChunks : public Exception
   {
   public:
      NoImageDataChunks() : Exception("No image data chunks: there were no image data (IDAT) chunks found in the PNG image.") {}
   };

   /// @brief An exception thrown when image data is not currently loaded in the PNG object.
   class NoImageData : public Exception
   {
   public:
      NoImageData() : Exception("No image data: either the image data was not decompressed from the IDAT chunks "
                                "or the canvas was not initialized.") {}
   };

   /// @brief An exception thrown when pixel types do not match up.
   class PixelMismatch : public Exception
   {
   public:
      PixelMismatch() : Exception("Pixel mismatch: the given pixel type does not match with either the header type or the "
                                  "raw pixel data.") {}
   };

   /// @brief An exception thrown when no pixels appear in a given scanline.
   class NoPixels : public Exception
   {
   public:
      NoPixels() : Exception("No pixels: there are no pixels in the given scanline.") {}
   };

   /// @brief An exception thrown when the color type does not match the facade::png::ColorType enum.
   class InvalidColorType : public Exception
   {
   public:
      /// @brief The offending color type value.
      std::uint8_t color_type;

      InvalidColorType(std::uint8_t color_type) : color_type(color_type), Exception() {
         std::stringstream stream;

         stream << "Invalid color type: the encountered color type value (" << static_cast<int>(color_type) << ") is invalid.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when the given bit depth isn't a valid value.
   ///
   /// Valid bit-depth values are 1, 2, 4, 8 and 16.
   class InvalidBitDepth : public Exception
   {
   public:
      /// @brief The offending bit depth value.
      std::uint8_t bit_depth;

      InvalidBitDepth(std::uint8_t bit_depth) : bit_depth(bit_depth), Exception() {
         std::stringstream stream;

         stream << "Invalid bit depth: the encountered bit depth (" << bit_depth << ") is invalid. ";
         stream << "Valid values can be 1, 2, 4, 8 and 16.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when the scanline pixel types do not match up.
   class ScanlineMismatch : public Exception
   {
   public:
      ScanlineMismatch() : Exception("Scanline mismatch: the given scanline does not match the target scanline.") {}
   };

   /// @brief An exception thrown when the given filter type does not match the facade::png::FilterType enum.
   class InvalidFilterType : public Exception
   {
   public:
      /// @brief The offending filter type value.
      std::uint8_t filter_type;

      InvalidFilterType(std::uint8_t filter_type) : filter_type(filter_type), Exception() {
         std::stringstream stream;

         stream << "Invalid filter type: the given filter type " << static_cast<int>(filter_type) << " was not a valid value.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when the PNG image has already been filtered.
   class AlreadyFiltered : public Exception
   {
   public:
      AlreadyFiltered() : Exception("Already filtered: the given scanline has already had a filter applied to it.") {}
   };

   /// @brief An exception thrown when a given integer overflows.
   class IntegerOverflow : public Exception
   {
   public:
      /// @brief The given value which caused the overflow.
      std::size_t given;

      /// @brief The maximum value that can be given.
      std::size_t max;

      IntegerOverflow(std::size_t given, std::size_t max) : given(given), max(max), Exception() {
         std::stringstream stream;

         stream << "Integer overflow: the given number was " << given
                << ", but the maximum value is " << max;

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when no data is provided.
   class NoData : public Exception
   {
   public:
      NoData() : Exception("No data: no data was provided.") {}
   };

   /// @brief An exception thrown when the pixel type does not match the facade::png::PixelEnum enum.
   class InvalidPixelType : public Exception
   {
   public:
      /// @brief The offending pixel type value.
      std::size_t pixel_type;

      InvalidPixelType(std::size_t pixel_type) : pixel_type(pixel_type), Exception() {
         std::stringstream stream;

         stream << "Invalid pixel type: the given pixel type, " << pixel_type << ", is invalid.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when no keyword is present in the given `tEXt` or `zTXt` chunk of a PNG image.
   class NoKeyword : public Exception
   {
   public:
      NoKeyword() : Exception("No keyword: there is no keyword present in the text chunk.") {}
   };

   /// @brief An exception thrown when the given keyword is too long for the `tEXt` or `zTXt` chunk.
   ///
   /// The length limit of keywords is 79 characters.
   class KeywordTooLong : public Exception
   {
   public:
      KeywordTooLong() : Exception("Keyword too long: the keyword in a text chunk can only be 79 characters long.") {}
   };

   /// @brief An exception returned when the given `tEXt` or `zTXt` section parameters (typically the keyword) is not found in the PNG data.
   class TextNotFound : public Exception
   {
   public:
      TextNotFound() : Exception("Text not found: the given text was not found in the PNG image.") {}
   };

   /// @brief An exception thrown when encountering an invalid Base64 character.
   /// @sa facade::BASE64_ALPHA
   class InvalidBase64Character : public Exception
   {
   public:
      /// @brief The offending character which was encountered.
      char c;

      InvalidBase64Character(char c) : c(c), Exception() {
         std::stringstream stream;

         stream << "Invalid Base64 character: the given character, '" << c << "', is not a valid Base64 character.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when encountering an invalid base64 string.
   class InvalidBase64String : public Exception
   {
   public:
      /// @brief The offending string.
      std::string string;

      InvalidBase64String(const std::string &string) : string(string), Exception() {
         std::stringstream stream;

         stream << "Invalid Base64 string: the given string is not a valid Base64 string: " << string;

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when the given pixel enum type is not supported.
   class UnsupportedPixelType : public Exception
   {
   public:
      /// @brief The offending pixel type.
      std::size_t pixel_type;

      UnsupportedPixelType(std::size_t pixel_type) : pixel_type(pixel_type), Exception() {
         std::stringstream stream;

         stream << "Unsupported pixel type: the given pixel-type value, " << pixel_type << ", is unsupported for the given operation.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when the possible space provided by the image data is too small for the given operation.
   class ImageTooSmall : public Exception
   {
   public:
      /// @brief The amount of data the given image can hold, in bytes.
      std::size_t given;
      /// @brief The amount of space needed by the operation.
      std::size_t needed;

      ImageTooSmall(std::size_t given, std::size_t needed) : given(given), needed(needed), Exception() {
         std::stringstream stream;

         stream << "Image too small: the image wasn't large enough to support the operation. The image can hold "
                << given << " bytes of data, but needed at least " << needed;

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when no steganographic data is present.
   class NoStegoData : public Exception
   {
   public:
      NoStegoData() : Exception("No stego data: there is no steganographic data within the image.") {}
   };

   /// @brief An exception thrown when the chunk is not found in the PNG data.
   class ChunkNotFound : public Exception
   {
   public:
      /// @brief The chunk tag which could not be found.
      std::string tag;

      ChunkNotFound(std::string tag) : tag(tag), Exception() {
         std::stringstream stream;

         stream << "Chunk not found: chunks with the tag \"" << tag << "\" could not be found in the image.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when the given bit offset is not a multiple of 4.
   class InvalidBitOffset : public Exception
   {
   public:
      /// @brief The offending offset.
      std::size_t offset;

      InvalidBitOffset(std::size_t offset) : offset(offset), Exception() {
         std::stringstream stream;

         stream << "Invalid bit offset: the given bit offset, " << offset << ", was not divisble by 4.";

         this->error = stream.str();
      }
   };

   /// @brief An exception thrown when no trailing data is present in the PNG image.
   class NoTrailingData : public Exception
   {
   public:
      NoTrailingData() : Exception("No trailing data: the image has no trailing data.") {}
   };
}}
#endif
