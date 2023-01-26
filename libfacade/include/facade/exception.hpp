#ifndef __FACADE_EXCEPTION_HPP
#define __FACADE_EXCEPTION_HPP

#include <cstdint>
#include <exception>
#include <sstream>
#include <string>

namespace facade
{
namespace exception
{
   class Exception : public std::exception
   {
   public:
      std::string error;

      Exception() : std::exception() {}
      Exception(const std::string &error) : error(error), std::exception() {}

      const char *what() const noexcept {
         return this->error.c_str();
      }
   };

   class InvalidChunkTag : public Exception
   {
   public:
      InvalidChunkTag() : Exception("Invalid chunk tag: the string provided was not a valid chunk tag. "
                                    "Chunk tags can only be 4 characters long.") {}
   };

   class OutOfBounds : public Exception
   {
   public:
      std::size_t given;
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

   class NullPointer : public Exception
   {
   public:
      NullPointer() : Exception("Null pointer: encountered a null pointer where it wasn't expected.") {}
   };

   class BadPNGSignature : public Exception
   {
   public:
      BadPNGSignature() : Exception("Bad PNG signature: the signature header of the PNG file was not valid.") {}
   };

   class BadCRC : public Exception
   {
   public:
      std::uint32_t given;
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

   class OpenFileFailure : public Exception
   {
   public:
      std::string filename;

      OpenFileFailure(const std::string &filename) : filename(filename), Exception() {
         std::stringstream stream;

         stream << "Open file failure: failed to open file with name \""
                << filename
                << "\"";

         this->error = stream.str();
      }
   };

   class InsufficientSize : public Exception
   {
   public:
      std::size_t given;
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

   class NoHeaderChunk : public Exception
   {
   public:
      NoHeaderChunk() : Exception("No header chunk: the header chunk for the PNG image was not found.") {}
   };

   class ZLibError : public Exception
   {
   public:
      int code;

      ZLibError(int code) : code(code), Exception() {
         std::stringstream stream;

         stream << "ZLib error: ZLib encountered an error while compressing/decompressiong, error code "
                << code;

         this->error = stream.str();
      }
   };

   class NoImageDataChunks : public Exception
   {
   public:
      NoImageDataChunks() : Exception("No image data chunks: there were no image data (IDAT) chunks found in the PNG image.") {}
   };

   class NoImageData : public Exception
   {
   public:
      NoImageData() : Exception("No image data: either the image data was not decompressed from the IDAT chunks "
                                "or the canvas was not initialized.") {}
   };

   class PixelMismatch : public Exception
   {
   public:
      PixelMismatch() : Exception("Pixel mismatch: the given pixel type does not match with either the header type or the "
                                  "raw pixel data.") {}
   };

   class NoPixels : public Exception
   {
   public:
      NoPixels() : Exception("No pixels: there are no pixels in the given scanline.") {}
   };

   class InvalidColorType : public Exception
   {
   public:
      std::uint8_t color_type;

      InvalidColorType(std::uint8_t color_type) : color_type(color_type), Exception() {
         std::stringstream stream;

         stream << "Invalid color type: the encountered color type value (" << static_cast<int>(color_type) << ") is invalid.";

         this->error = stream.str();
      }
   };

   class InvalidBitDepth : public Exception
   {
   public:
      std::uint8_t bit_depth;

      InvalidBitDepth(std::uint8_t bit_depth) : bit_depth(bit_depth), Exception() {
         std::stringstream stream;

         stream << "Invalid bit depth: the encountered bit depth (" << bit_depth << ") is invalid. ";
         stream << "Valid values can be 1, 2, 4, 8 and 16.";

         this->error = stream.str();
      }
   };

   class ScanlineMismatch : public Exception
   {
   public:
      ScanlineMismatch() : Exception("Scanline mismatch: the given scanline does not match the target scanline.") {}
   };

   class InvalidFilterType : public Exception
   {
   public:
      std::uint8_t filter_type;

      InvalidFilterType(std::uint8_t filter_type) : filter_type(filter_type), Exception() {
         std::stringstream stream;

         stream << "Invalid filter type: the given filter type " << static_cast<int>(filter_type) << " was not a valid value.";

         this->error = stream.str();
      }
   };

   class AlreadyFiltered : public Exception
   {
   public:
      AlreadyFiltered() : Exception("Already filtered: the given scanline has already had a filter applied to it.") {}
   };

   class IntegerOverflow : public Exception
   {
   public:
      std::size_t given, max;

      IntegerOverflow(std::size_t given, std::size_t max) : given(given), max(max), Exception() {
         std::stringstream stream;

         stream << "Integer overflow: the given number was " << given
                << ", but the maximum value is " << max;

         this->error = stream.str();
      }
   };

   class NoData : public Exception
   {
   public:
      NoData() : Exception("No data: no data was provided.") {}
   };

   class InvalidPixelType : public Exception
   {
   public:
      std::size_t pixel_type;

      InvalidPixelType(std::size_t pixel_type) : pixel_type(pixel_type), Exception() {
         std::stringstream stream;

         stream << "Invalid pixel type: the given pixel type, " << pixel_type << ", is invalid.";

         this->error = stream.str();
      }
   };

   class NoKeyword : public Exception
   {
   public:
      NoKeyword() : Exception("No keyword: there is no keyword present in the text chunk.") {}
   };

   class KeywordTooLong : public Exception
   {
   public:
      KeywordTooLong() : Exception("Keyword too long: the keyword in a text chunk can only be 79 characters long.") {}
   };

   class TextNotFound : public Exception
   {
   public:
      TextNotFound() : Exception("Text not found: the given text was not found in the PNG image.") {}
   };
}}
#endif
