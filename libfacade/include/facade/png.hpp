#ifndef __FACADE_PNG_HPP
#define __FACADE_PNG_HPP

//! @file png.hpp
//! @brief Code functionality for dealing with PNG images.
//!
//! This file contains everything you should need for interacting with a PNG file where it relates to arbitrary payloads.
//!
//! @section design Design Explanation
//! 
//! The library is specifically designed in mind to make things as easy as possible without drowning the user in
//! repetitive typing of objects. There are many design challenges when attempting to create a library for PNG images.
//!
//! It starts with five pixels types:
//! * **Grayscale**: literally just grayscale pixels.
//! * **TrueColor**: traditional colored RGB pixels.
//! * **Palette**: a fixed, indexed palette of pixel colors.
//! * **AlphaGrayscale**: a grayscale pixel, but with an added transparency channel.
//! * **AlphaTrueColor**: an RGB pixel with an added transparency channel.
//!
//! Already, we have five types to consider. Individual pixel values are composed of what are called *samples*. For example,
//! a *sample* of a grayscale pixel value can be the following sizes:
//! * 1-bit
//! * 2-bit
//! * 4-bit
//! * 8-bit
//! * 16-bit
//!
//! This already provides one problem in particular with C++: we can't have references to pixels because their *sample size* could go under the
//! byte-boundary threshhold. That means we can't directly bind an object to a location in memory because we would need access to bit boundaries,
//! which is only really exposed through a byte interface of interacting with bits via masking and assignment. In other words, the present of
//! sub 8-bit samples means we need to create an *interface* to accessing and assigning pixels.
//!
//! In addition to creating this bit-access problem, the bit depth of provided pixels means we now have many, many more types to deal with:
//! * **Grayscale**: Can be 1-bit, 2-bit, 4-bit, 8-bit or 16-bit, resulting in 5 pixel types.
//! * **TrueColor**: Can be 8-bit RGB or 16-bit RGB, resulting in 2 pixel types.
//! * **Palette**: Can be 1-bit, 2-bit, 4-bit or 8-bit, resulting in 4 pixel types.
//! * **AlphaGrayscale**: Can be 8-bit or 16-bit, resulting in 2 pixel types.
//! * **AlphaTrueColor**: Can be 8-bit or 16-bit, resulting in 2 pixel types.
//!
//! In total, there are then **15** pixel types to deal with. This cascades down to what are called *scanlines*, which in turn, there are
//! 15 types of scanlines as well (e.g., an AlphaTrueColor 8-bit scanline).
//!
//! On top of the typing issue, there is the fact that we don't know what pixel type we're dealing with until we read the file. This means we
//! can't just declare our image data as a certain type before the image is parsed. We can, however, set the type of pixel for our image data
//! through the use of [C++ *variants*](https://en.cppreference.com/w/cpp/utility/variant). In short, a *variant* is a C++ version of a
//! *union* in C. It intelligently keeps track of which type is active within the variant.
//!
//! So to solve our problems from the *pixel* side of things, we have one variant type for pixels: **facade::png::Pixel**. This is a variant
//! of all pixel types within a PNG image. To store our pixel data into scanlines, we also have a scanline variant, **facade::png::Scanline**.
//! This way, a PNG image can be instantiated and loaded without the user needing to clarify ahead of time-- and possibly incorrectly--
//! what type of pixel the PNG contains.
//!
//! To solve the problem of pixels with less than 8-bit samples, within a scanline, pixels are reduced to an object that is called a
//! **facade::png::PixelSpan**. This is the raw interface to accessing pixels in memory. It is capable of both accessing pixel values at the
//! bit level as well as pixel values that go well beyond a byte in size.
//!
//! @section usage Usage
//!
//! For the sake of saving memory when it isn't needed, only the PNG chunks are loaded in the image at first, not the image data. Not every operation
//! on a given PNG image needs the image data, after all. This means a given PNG image can be in various states.
//!
//! There are basically three steps to getting PNG image data:
//! * **Parsing**: this is when the PNG image is reduced to its chunks, for example, the `IDAT` chunk which contains image data and the
//!                `IHDR` chunk, which contains PNG header information for this image.
//! * **Decompressing**: this is when the PNG image data is being decompressed into its raw (but not processed) form.
//! * **Reconstructing**: this is the process of undoing the filtering on the image that helps it compress better.
//!
//! You can opt to do this manually by calling facade::png::Image::decompress followed by facade::png::Image::reconstruct, or you could simply call
//! facade::png::Image::load, which does those functions for you. The same process is important to do backwards when saving a file: before saving
//! the file, you should call facade::png::Image::filter, followed by facade::png::Image::compress, followed by facade::png::Image::save.
//!
//! This example demonstrates how to load an image, access its pixels, modify its pixels, and save the pixels to a new image.
//! @include png_manipulation.cpp
//!

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <facade/platform.hpp>
#include <facade/exception.hpp>
#include <facade/utility.hpp>

namespace facade
{
namespace png
{
   /// @brief The string tag identifying a given facade::png::ChunkVec or facade::png::ChunkPtr.
   ///
   class
   PACK(1)
   EXPORT
      ChunkTag
   {
      std::uint8_t _tag[4];

   public:
      ChunkTag() { std::memset(this->_tag, 0, sizeof(std::uint8_t) * 4); }
      ChunkTag(const std::uint8_t *tag, std::size_t size) { this->set_tag(tag, size); }
      ChunkTag(const char *tag) { this->set_tag(std::string(tag)); }
      ChunkTag(const std::string tag) { this->set_tag(tag); }
      ChunkTag(const ChunkTag &other) { this->set_tag(&other._tag[0], 4); }

      bool operator==(const ChunkTag &other) const;

      /// @brief Set the tag value for this chunk tag with a std::string value.
      ///
      /// A chunk tag that is not 4 bytes in size throws facade::exception::InvalidChunkTag.
      ///
      /// @param tag The std::string representation of the tag.
      /// @throws facade::exception::InvalidChunkTag
      ///
      void set_tag(const std::string tag);

      /// @brief Set the tag value for this chunk tag with a uint8_t pointer value.
      ///
      /// A chunk tag that is not 4 bytes in size throws facade::exception::InvalidChunkTag.
      ///
      /// @param tag The tag data pointer.
      /// @param size The size of the tag data pointer.
      /// @throws facade::exception::InvalidChunkTag
      ///
      void set_tag(const std::uint8_t *tag, std::size_t size);

      /// @brief Return a pointer to the underlying chunk tag data.
      ///
      std::uint8_t *tag();

      /// @brief Return a const pointer to the underlying chunk tag data.
      ///
      const std::uint8_t *tag() const;

      /// @brief Convert this chunk tag to a std::string value.
      ///
      std::string to_string() const;
   };
   UNPACK()

   class ChunkPtr;

   /// @brief A vector-based version of a given PNG chunk.
   ///
   /// This is the base class of many different types of PNG chunks, such as facade::png::Header and facade::png::Text.
   ///
   class
   EXPORT
   ChunkVec
   {
      ChunkTag _tag;
      std::vector<std::uint8_t> _data;

   public:
      ChunkVec(const ChunkTag tag) : _tag(tag) {}
      ChunkVec(const ChunkTag tag, const void *ptr, std::size_t size)
         : _tag(tag),
           _data(&reinterpret_cast<const std::uint8_t *>(ptr)[0],
                 &reinterpret_cast<const std::uint8_t *>(ptr)[size]) {}
      ChunkVec(const ChunkTag tag, const std::vector<std::uint8_t> &data) : _tag(tag), _data(data) {}
      ChunkVec(const ChunkVec &other) : _tag(other._tag), _data(other._data) {}

      bool operator==(const ChunkVec &other) const;

      /// @brief Return the length of this chunk's data.
      ///
      std::size_t length() const;

      /// @brief Return the chunk tag reference associated with this chunk.
      ///
      ChunkTag &tag();
      /// @brief Return a const chunk tag reference associated with this chunk.
      ///
      const ChunkTag &tag() const;

      /// @brief Return the chunk data reference associated with this chunk.
      ///
      std::vector<std::uint8_t> &data();
      /// @brief Return const chunk data reference associated with this chunk.
      ///
      const std::vector<std::uint8_t> &data() const;
      /// @brief Set the chunk data for this chunk.
      /// @param data The data vector to set for the chunk's data.
      ///
      void set_data(std::vector<std::uint8_t> &data);

      /// @brief Calculate the CRC value of this chunk.
      ///
      std::uint32_t crc() const;

      /// @brief Convert this ChunkVec to a facade::png::ChunkPtr.
      /// @warning If the vector returned by this function goes out of scope, the returned ChunkPtr will point
      ///          at deleted data. Make sure the vector and the ChunkPtr object retain the same scope.
      ///
      std::pair<std::vector<std::uint8_t>, ChunkPtr> to_chunk_ptr() const;

      /// @brief Create a reference to a higher-level ChunkVec object, such as facade::png::Header.
      ///
      template <typename T>
      T& upcast() {
         static_assert(std::is_base_of<ChunkVec, T>::value,
                       "Upcast type must derive ChunkVec");

         return *static_cast<T*>(this);
      }

      /// @brief Create a const reference to a higher-level ChunkVec object, such as facade::png::Header.
      ///
      template <typename T>
      const T& upcast() const {
         static_assert(std::is_base_of<ChunkVec, T>::value,
                       "Upcast type must derive ChunkVec");
         
         return *static_cast<const T*>(this);
      }

      /// @brief Interpret this ChunkVec derivative as a ChunkVec object.
      ///
      /// Really only useful when turning a derived ChunkVec object into its base.
      ///
      ChunkVec &as_chunk_vec();

      /// @brief Interpret this ChunkVec derivative as a const ChunkVec object.
      ///
      /// Really only useful when turning a derived ChunkVec object into its base.
      ///
      const ChunkVec &as_chunk_vec() const;
   };

   /// @brief The chunk class responsible for parsing the raw data of a PNG file.
   ///
   /// This should only be used when either parsing chunk data directly from the PNG file or writing
   /// said data back into the PNG file. It relies on data outside of itself to function, and it is
   /// therefore possible to crash your program if you're not careful with it.
   ///
   class
   EXPORT
   ChunkPtr
   {
      const std::uint32_t *_length;
      const ChunkTag *_tag;
      const std::uint8_t *_data;
      const std::uint32_t *_crc;

      ChunkPtr(const std::uint32_t *length, const ChunkTag *tag, const std::uint8_t *data, const std::uint32_t *crc)
         : _length(length),
           _tag(tag),
           _data(data),
           _crc(crc) {}

   public:
      ChunkPtr() : _length(nullptr), _tag(nullptr), _data(nullptr), _crc(nullptr) {}
      ChunkPtr(const ChunkPtr &other)
         : _length(other._length),
           _tag(other._tag),
           _data(other._data),
           _crc(other._crc) {}

      /// @brief Parse the given buffer for chunk data.
      /// @param ptr The pointer to the raw PNG chunk data.
      /// @param size The size of the buffer to look at.
      /// @param offset The offset in the buffer to begin looking for a chunk.
      /// @return A parsed chunk.
      /// @throws facade::exception::NullPointer
      /// @throws facade::exception::NoData
      /// @throws facade::exception::OutOfBounds
      ///
      static ChunkPtr parse(const void *ptr, std::size_t size, std::size_t offset);
      /// @brief Parse the given vector for chunk data.
      /// @param vec The vector to check for chunk data.
      /// @param offset The offset to begin parsing.
      /// @return A parsed chunk.
      /// @sa facade::png::ChunkPtr::parse(const void *, std::size_t, std::size_t)
      ///
      static ChunkPtr parse(const std::vector<std::uint8_t> &vec, std::size_t offset);

      /// @brief Return the length of this parsed chunk data.
      ///
      std::size_t length() const;
      /// @brief Return the chunk tag of this parsed data.
      ///
      ChunkTag tag() const;
      /// @brief Return a copy of the data in this chunk.
      ///
      std::vector<std::uint8_t> data() const;
      /// @brief Return the CRC value of this chunk.
      ///
      /// Note that this does not calculate the CRC value-- it merely returns the CRC value within the chunk.
      ///
      std::uint32_t crc() const;
      /// @brief Calculate the CRC value of this chunk and validate it against the stored CRC value.
      /// @return True if the CRC is valid, false otherwise.
      ///
      bool validate() const;
      /// @brief Return the size, in bytes, of the full chunk.
      ///
      /// This differs from facade::png::ChunkPtr::length by calculating the length of the full chunk (i.e., includes the length and the CRC),
      /// rather than just the chunk's data length.
      ///
      std::size_t chunk_size() const;
      /// @brief Return all data representing this chunk.
      ///
      /// This differs from facade::png::ChunkPtr::data by returning all data, including the length, the tag and the CRC.
      ///
      std::vector<std::uint8_t> chunk_data() const;
      /// @brief Convert this ChunkPtr into a ChunkVec object.
      ///
      ChunkVec to_chunk_vec() const;
   };

   /// @brief The color type represented in the PNG file.
   ///
   enum ColorType
   {
      GRAYSCALE = 0,
      TRUE_COLOR = 2,
      PALETTE = 3,
      ALPHA_GRAYSCALE = 4,
      ALPHA_TRUE_COLOR = 6
   };

   /// @brief A sample of data for a given pixel.
   ///
   /// A *sample* is a unit of bits representing a given pixel value (e.g., a color channel). It can be a variety of sizes:
   /// * 1-bit
   /// * 2-bit
   /// * 4-bit
   /// * 8-bit
   /// * 16-bit
   ///
   /// As a result, it can be two different types:
   /// * uint8_t
   /// * uint16_t
   ///
   /// To make typing easier, a series of typedefs are created to declare what type of sample
   /// you want.
   ///
   /// @tparam _Base The base type that contains the sample value. Can be std::uint8_t or std::uint16_t.
   /// @tparam _Bits The size, in bits, of the desired sample. Can be 1, 2, 4, 8 or 16.
   /// @sa facade::png::Sample1Bit
   /// @sa facade::png::Sample2Bit
   /// @sa facade::png::Sample4Bit
   /// @sa facade::png::Sample8Bit
   /// @sa facade::png::Sample16Bit
   ///
   template <typename _Base=std::uint8_t, std::size_t _Bits=sizeof(_Base)*8>
   class
   PACK(1)
   EXPORT
      Sample
   {
      static_assert(_Bits == 1 || _Bits == 2 || _Bits == 4 || _Bits == 8 || _Bits == 16,
                    "Bits must be 1, 2, 4, 8 or 16.");
      static_assert(std::is_same<_Base,std::uint8_t>::value || std::is_same<_Base,std::uint16_t>::value,
                    "Base must be uint8_t or uint16_t.");
      static_assert((std::is_same<_Base,std::uint16_t>::value && _Bits == 16) ||
                    (std::is_same<_Base,std::uint8_t>::value && _Bits <= 8),
                    "Bit size does not match the base type.");

      _Base _value;

   public:
      /// @brief The base type used by this sample.
      using Base = _Base;
      /// @brief The size, in bits, of this sample.
      const static std::size_t Bits = _Bits;
      /// @brief The maximum value that can be used with this sample.
      const static std::size_t Max = (1 << Bits) - 1;
      
      Sample() : _value(0) {}
      Sample(Base value) { this->set_value(value); }
      Sample(const Sample &other) { this->set_value(other._value); }

      /// @brief Syntactic sugar to get the value of this sample.
      /// @sa facade::png::Sample::value
      ///
      Base operator*() const { return this->value(); }
      /// @brief Syntactic sugar to assign the value of this sample.
      /// @sa facade::png::Sample::set_value
      ///
      Sample &operator=(Base value) { this->set_value(value); return *this; }

      /// @brief Get the value retained by this sample.
      ///
      Base value() const {
         if constexpr (Bits == 16) { return endian_swap_16(this->_value); }
         else { return this->_value & this->Max; }
      }

      /// @brief Assign the given value to this sample.
      ///
      /// Throws an exception if the value provided is too large for the given sample size.
      ///
      /// @param value The value to assign to the sample.
      /// @throws facade::exception::IntegerOverflow
      ///
      void set_value(Base value) {
         if (value > this->Max) { throw exception::IntegerOverflow(value, this->Max); }

         if constexpr (Bits == 1 || Bits == 2 || Bits == 4)
         {
            this->_value = value & this->Max;
         }
         else if constexpr (Bits == 16) { this->_value = endian_swap_16(value); }
         else { this->_value = value; }
      }
   };
   UNPACK()

   using Sample1Bit = Sample<std::uint8_t, 1>;
   using Sample2Bit = Sample<std::uint8_t, 2>;
   using Sample4Bit = Sample<std::uint8_t, 4>;
   using Sample8Bit = Sample<std::uint8_t>;
   using Sample16Bit = Sample<std::uint16_t>;

   /// @brief A grayscale pixel object, based on the facade::png::Sample class.
   /// @tparam _Base The base type that contains the sample value. Can be std::uint8_t or std::uint16_t.
   /// @tparam _Bits The size, in bits, of the desired sample. Can be 1, 2, 4, 8 or 16.
   /// @sa facade::png::Sample
   /// @sa facade::png::GrayscalePixel1Bit
   /// @sa facade::png::GrayscalePixel2Bit
   /// @sa facade::png::GrayscalePixel4Bit
   /// @sa facade::png::GrayscalePixel8Bit
   /// @sa facade::png::GrayscalePixel16Bit
   ///
   template <typename _Base=std::uint8_t, std::size_t _Bits=sizeof(_Base)*8>
   class GrayscalePixel : public Sample<_Base, _Bits>
   {
   public:
      GrayscalePixel() : Sample<_Base, _Bits>(0) {}
      GrayscalePixel(_Base value) : Sample<_Base, _Bits>(value) {}
      GrayscalePixel(const GrayscalePixel &other) : Sample<_Base, _Bits>(other) {}

      /* TODO: color conversion functions */
   };

   /// @brief A facade::png::GrayscalePixel with a 1-bit facade::png::Sample value.
   using GrayscalePixel1Bit = GrayscalePixel<std::uint8_t, 1>;
   /// @brief A facade::png::GrayscalePixel with a 2-bit facade::png::Sample value.
   using GrayscalePixel2Bit = GrayscalePixel<std::uint8_t, 2>;
   /// @brief A facade::png::GrayscalePixel with a 4-bit facade::png::Sample value.
   using GrayscalePixel4Bit = GrayscalePixel<std::uint8_t, 4>;
   /// @brief A facade::png::GrayscalePixel with an 8-bit facade::png::Sample value.
   using GrayscalePixel8Bit = GrayscalePixel<std::uint8_t>;
   /// @brief A facade::png::GrayscalePixel with a 16-bit facade::png::Sample value.
   using GrayscalePixel16Bit = GrayscalePixel<std::uint16_t>;

   /// @brief An RGB pixel object.
   /// @tparam _Sample The given sample size of the RGB pixel object. Can be facade::png::Sample8Bit or facade::png::Sample16Bit
   /// @sa facade::png::TrueColorPixel8Bit
   /// @sa facade::png::TrueColorPixel16Bit
   ///
   template <typename _Sample=Sample8Bit>
   class
   PACK(1)
   EXPORT
      TrueColorPixel
   {
      static_assert(std::is_same<_Sample,Sample8Bit>::value || std::is_same<_Sample,Sample16Bit>::value,
                    "Sample type must be Sample8Bit or Sample16Bit.");
      
      _Sample _red, _green, _blue;

   public:
      /// @brief The sample class that was used in creating this class.
      using Sample = _Sample;
      /// @brief The base type of the sample class used in creating this class.
      using Base = typename Sample::Base;
      /// @brief The maximum value of the sample size.
      const static std::size_t Max = Sample::Max;
      /// @brief The size, in bits, of this pixel object.
      const static std::size_t Bits = Sample::Bits*3;
      
      TrueColorPixel() : _red(0), _green(0), _blue(0) {}
      TrueColorPixel(Sample red, Sample green, Sample blue)
         : _red(red),
           _green(green),
           _blue(blue) {}
      TrueColorPixel(const TrueColorPixel &other)
         : _red(other._red),
           _green(other._green),
           _blue(other._blue) {}

      /// @brief Retrieve a reference to the red channel.
      ///
      Sample &red() { return this->_red; }
      /// @brief Retrieve a const reference to the red channel.
      ///
      const Sample &red() const { return this->_red; }

      /// @brief Retrieve a reference to the green channel.
      ///
      Sample &green() { return this->_green; }
      /// @brief Retrieve a const reference to the green channel.
      ///
      const Sample &green() const { return this->_green; }

      /// @brief Retrieve a reference to the blue channel.
      ///
      Sample &blue() { return this->_blue; }
      /// @brief Retrieve a const reference to the blue channel.
      ///
      const Sample &blue() const { return this->_blue;}
   };
   UNPACK()

   /// @brief A facade::png::TrueColorPixel with an 8-bit facade::png::Sample value.
   using TrueColorPixel8Bit = TrueColorPixel<Sample8Bit>;
   /// @brief A facade::png::TrueColorPixel with a 16-bit facade::png::Sample value.
   using TrueColorPixel16Bit = TrueColorPixel<Sample16Bit>;

   /// @brief A paletted pixel object.
   /// @tparam _Bits The size, in bits, of the underlying value sample. Can be 1, 2, 4 or 8.
   /// @sa facade::png::PalettePixel1Bit
   /// @sa facade::png::PalettePixel2Bit
   /// @sa facade::png::PalettePixel4Bit
   /// @sa facade::png::PalettePixel8Bit
   ///
   template <std::size_t _Bits=sizeof(std::uint8_t)*8>
   class PalettePixel : public Sample<std::uint8_t, _Bits>
   {
      static_assert(_Bits == 1 || _Bits == 2 || _Bits == 4 || _Bits == 8,
                    "Bits can only be 1, 2, 4, or 8.");
      
   public:
      PalettePixel() : Sample<std::uint8_t, _Bits>() {}
      PalettePixel(std::uint8_t value) : Sample<std::uint8_t, _Bits>(value) {}
      PalettePixel(const PalettePixel &other) : Sample<std::uint8_t, _Bits>(other) {}

      /* TODO: convert to rgb */
   };

   /// @brief A facade::png::PalettePixel object with a 1-bit facade::png::Sample value.
   using PalettePixel1Bit = PalettePixel<1>;
   /// @brief A facade::png::PalettePixel object with a 2-bit facade::png::Sample value.
   using PalettePixel2Bit = PalettePixel<2>;
   /// @brief A facade::png::PalettePixel object with a 4-bit facade::png::Sample value.
   using PalettePixel4Bit = PalettePixel<4>;
   /// @brief A facade::png::PalettePixel object with an 8-bit facade::png::Sample value.
   using PalettePixel8Bit = PalettePixel<8>;

   /// @brief A grayscale pixel with alpha channel.
   /// @tparam _Base The base type for the sample value. Can be std::uint8_t or std::uint16_t.
   /// @sa facade::png::AlphaGrayscalePixel8Bit
   /// @sa facade::png::AlphaGrayscalePixel16Bit
   ///
   template <typename _Base=std::uint8_t>
   class
   PACK(1)
   EXPORT
      AlphaGrayscalePixel : public GrayscalePixel<_Base>
   {
      static_assert(std::is_same<_Base,std::uint8_t>::value || std::is_same<_Base,std::uint16_t>::value,
                    "Base must be uint8_t or uint16_t.");
   public:
      //using Sample = Sample<_Base>;
      /// @brief The type base of the samples in this pixel.
      ///
      using Base = _Base;

   private:
      Sample<_Base> _alpha;

   public:
      const static std::size_t Bits = Sample<_Base>::Bits*2;
      
      AlphaGrayscalePixel() : _alpha(0), GrayscalePixel<_Base>() {}
      AlphaGrayscalePixel(Sample<_Base> value, Sample<_Base> alpha) : _alpha(alpha), GrayscalePixel<_Base>(value) {}
      AlphaGrayscalePixel(const AlphaGrayscalePixel &other) : _alpha(other._alpha), GrayscalePixel<_Base>(other) {}

      /// @brief The alpha channel of this pixel.
      ///
      Sample<_Base> &alpha() { return this->_alpha; }

      /// @brief The const alpha channel of this pixel.
      ///
      const Sample<_Base> &alpha() const { return this->_alpha; }
   };
   UNPACK()

   /// @brief An facade::png::AlphaGrayscalePixel with an 8-bit facade::png::Sample size.
   using AlphaGrayscalePixel8Bit = AlphaGrayscalePixel<std::uint8_t>;
   /// @brief An facade::png::AlphaGrayscalePixel with a 16-bit facade::png::Sample size.
   using AlphaGrayscalePixel16Bit = AlphaGrayscalePixel<std::uint16_t>;

   /// @brief An RGB pixel with alpha channel.
   /// @tparam _Sample The sample class to base the values on. Accepted values are facade::png::Sample8Bit and facade::png::Sample16Bit
   /// @sa facade::png::AlphaTrueColorPixel8Bit
   /// @sa facade::png::AlphaTrueColorPixel16Bit
   ///
   template <typename _Sample=Sample8Bit>
   class
   PACK(1)
   EXPORT
   AlphaTrueColorPixel : public TrueColorPixel<_Sample>
   {
      _Sample _alpha;

   public:
      const static std::size_t Bits=_Sample::Bits*4;
      
      AlphaTrueColorPixel() : _alpha(0), TrueColorPixel<_Sample>() {}
      AlphaTrueColorPixel(_Sample red, _Sample green, _Sample blue, _Sample alpha)
         : _alpha(alpha),
           TrueColorPixel<_Sample>(red, green, blue) {}
      AlphaTrueColorPixel(const AlphaTrueColorPixel &other)
         : _alpha(other._alpha),
           TrueColorPixel<_Sample>(other) {}
      
      /// @brief The alpha channel of this pixel.
      ///
      _Sample &alpha() { return this->_alpha; }
      
      /// @brief The const alpha channel of this pixel.
      ///
      const _Sample &alpha() const { return this->_alpha; }
   };
   UNPACK()

   /// An facade::png::AlphaTrueColorPixel with an 8-bit facade::png::Sample size.
   using AlphaTrueColorPixel8Bit = AlphaTrueColorPixel<Sample8Bit>;
   /// An facade::png::AlphaTrueColorPixel with a 16-bit facade::png::Sample size.
   using AlphaTrueColorPixel16Bit = AlphaTrueColorPixel<Sample16Bit>;

   /// @brief An enum representing all available pixel types.
   ///
   /// This enum corresponds to the indexes in variants such as facade::png::Pixel.
   ///
   enum PixelEnum {
      GRAYSCALE_PIXEL_1BIT = 0,
      GRAYSCALE_PIXEL_2BIT,
      GRAYSCALE_PIXEL_4BIT,
      GRAYSCALE_PIXEL_8BIT,
      GRAYSCALE_PIXEL_16BIT,
      TRUE_COLOR_PIXEL_8BIT,
      TRUE_COLOR_PIXEL_16BIT,
      PALETTE_PIXEL_1BIT,
      PALETTE_PIXEL_2BIT,
      PALETTE_PIXEL_4BIT,
      PALETTE_PIXEL_8BIT,
      ALPHA_GRAYSCALE_PIXEL_8BIT,
      ALPHA_GRAYSCALE_PIXEL_16BIT,
      ALPHA_TRUE_COLOR_PIXEL_8BIT,
      ALPHA_TRUE_COLOR_PIXEL_16BIT,
   };

   /// @brief The variant type corresponding to all known pixel types for PNG images.
   ///
   using Pixel = std::variant<GrayscalePixel1Bit,
                              GrayscalePixel2Bit,
                              GrayscalePixel4Bit,
                              GrayscalePixel8Bit,
                              GrayscalePixel16Bit,
                              TrueColorPixel8Bit,
                              TrueColorPixel16Bit,
                              PalettePixel1Bit,
                              PalettePixel2Bit,
                              PalettePixel4Bit,
                              PalettePixel8Bit,
                              AlphaGrayscalePixel8Bit,
                              AlphaGrayscalePixel16Bit,
                              AlphaTrueColorPixel8Bit,
                              AlphaTrueColorPixel16Bit>;

   /// @brief Macro that creates a static_assert that the given argument class is a known base pixel type.
#define ASSERT_PIXEL(pixel) static_assert(std::is_same<pixel, GrayscalePixel1Bit>::value || \
                                          std::is_same<pixel, GrayscalePixel2Bit>::value || \
                                          std::is_same<pixel, GrayscalePixel4Bit>::value || \
                                          std::is_same<pixel, GrayscalePixel8Bit>::value || \
                                          std::is_same<pixel, GrayscalePixel16Bit>::value || \
                                          std::is_same<pixel, TrueColorPixel8Bit>::value || \
                                          std::is_same<pixel, TrueColorPixel16Bit>::value || \
                                          std::is_same<pixel, PalettePixel1Bit>::value || \
                                          std::is_same<pixel, PalettePixel2Bit>::value || \
                                          std::is_same<pixel, PalettePixel4Bit>::value || \
                                          std::is_same<pixel, PalettePixel8Bit>::value || \
                                          std::is_same<pixel, AlphaGrayscalePixel8Bit>::value || \
                                          std::is_same<pixel, AlphaGrayscalePixel16Bit>::value || \
                                          std::is_same<pixel, AlphaTrueColorPixel8Bit>::value || \
                                          std::is_same<pixel, AlphaTrueColorPixel16Bit>::value, \
                                          #pixel " is not a base pixel-type.")
                                             
   /// @brief A span of data representing the underlying bits or bytes of a pixel.
   ///
   /// This class is the binary bridge which allows us to access pixel data that is less than 8 bits in size. It relies on a union
   /// of a byte and a byte array to contain as low as 1-bit pixels (with 8 samples) and as large as 64-bit pixels (with 1 sample).
   /// This design is necessary to properly type pixel data as it exists within raw PNG image data, allowing us to essentially create
   /// objects such as vectors of pixels, which then become a facade::png::ScanlineBase.
   ///
   /// @tparam PixelType The base pixel type to use for this span of pixel data.
   ///
   template <typename PixelType>
   class
   EXPORT
   PixelSpan
   {
      ASSERT_PIXEL(PixelType);
      
      union {
         std::uint8_t bits;
         std::uint8_t bytes[PixelType::Bits/8];
      } _data;

   public:
      PixelSpan() {}
      PixelSpan(const PixelSpan &other) { std::memcpy(&this->_data, &other._data, sizeof(this->_data)); }

      /// @brief The amount of samples possibly contained within this pixel span.
      ///
      /// Pixels that are greater than or equal to 8 bits are considered 1 sample.
      ///
      const static std::size_t Samples = (8 / PixelType::Bits) + static_cast<int>(PixelType::Bits > 8);

      /// @brief Syntactic sugar to get a facade::png::Pixel variant out of this span.
      /// @param index The index of the sample to retrieve.
      /// @sa facade::png::PixelSpan::get
      ///
      Pixel operator[](std::size_t index) const { return this->get(index); }

      /// @brief Return the underlying raw byte data representing this pixel span.
      ///
      const std::uint8_t *data() const {
         auto u8_ptr = reinterpret_cast<const std::uint8_t *>(&this->_data);
         return u8_ptr;
      }

      /// @brief Return the size of this pixel span.
      ///
      std::size_t data_size() const { return sizeof(PixelType); }

      /// @brief Retrieve a facade::png::Pixel variant of the underlying pixel type at the given index.
      /// @param index The index of the sample in the pixel to retrieve. 0 for anything over or equal to
      ///              8-bit, anything less than facade::png::PixelSpan::Samples for less than 8-bit.
      /// @throws facade::exception::OutOfBounds
      ///
      Pixel get(std::size_t index=0) const {
         if (index > this->Samples) { throw exception::OutOfBounds(index, this->Samples); }
         
         if constexpr (PixelType::Bits < 8) {
            auto bits = this->_data.bits;
            bits >>= (this->Samples - 1 - index) * PixelType::Bits;
            bits &= PixelType::Max;

            return PixelType(bits);
         }
         else { return *reinterpret_cast<const PixelType *>(&this->_data.bytes[0]); }
      }

      /// @brief Set a given facade::png::Pixel variant at the given offset.
      /// @param pixel The pixel variant to write to the pixel span.
      /// @param index The index of the sample to set. For pixels greater than or equal to 8-bits, this should be 0.
      ///              For pixels less than 8-bits, this can be anything less than facade::png::PixelSpan::Samples.
      /// @sa facade::png::PixelSpan::get
      /// @throws facade::exception::PixelMismatch
      /// @throws facade::exception::OutOfBounds
      ///
      void set(const Pixel &pixel, std::size_t index=0) {
         if (!std::holds_alternative<PixelType>(pixel)) { throw exception::PixelMismatch(); }
         if (index > this->Samples) { throw exception::OutOfBounds(index, this->Samples); }
         
         if constexpr (PixelType::Bits < 8)
         {
            auto bits = this->_data.bits;
            auto shift = (this->Samples - 1 - index) * PixelType::Bits;
            auto mask = (PixelType::Max << shift) ^ 0xFF;

            auto value = std::get<PixelType>(pixel).value();
            this->_data.bits = (bits & mask) | (value << shift);
         }
         else {
            std::memcpy(&this->_data.bytes[0], &pixel, PixelType::Bits/8);
         }
      }
   };

   /// @brief A vector of a facade::png::PixelSpan of the given pixel type.
   /// @tparam PixelType The base type of pixel to make this vector.
   template <typename PixelType>
   using PixelRow = std::vector<PixelSpan<PixelType>>;

   /// @brief Convert a row of facade::png::PixelSpan of the given pixel type into a vector of bytes.
   /// @tparam PixelType The pixel type of the row of span objects.
   /// @param pixels The pixel data to convert to bytes.
   ///
   template <typename PixelType>
   EXPORT std::vector<std::uint8_t> pixels_to_raw(const PixelRow<PixelType> &pixels)
   {
      std::vector<std::uint8_t> result;

      for (auto &span : pixels)
      {
         auto data = span.data();
         result.insert(result.end(), &data[0], &data[span.data_size()]);
      }

      return result;
   }

   /// @brief A PNG header object.
   /// @sa facade::png::ChunkVec
   ///
   class
   EXPORT
   Header : public ChunkVec
   {
   public:
      Header() : ChunkVec(std::string("IHDR"), std::vector<std::uint8_t>(13)) {}
      Header(const void *ptr, std::size_t size) : ChunkVec(std::string("IHDR"), ptr, size) {}
      Header(const std::vector<std::uint8_t> &vec) : ChunkVec(std::string("IHDR"), vec) {}
      Header(std::uint32_t width, std::uint32_t height, std::uint8_t bit_depth,
             std::uint8_t color_type, std::uint8_t compression_method=0,
             std::uint8_t filter_method=0, std::uint8_t interlace_method=0)
         : ChunkVec(std::string("IHDR"), std::vector<std::uint8_t>(13))
      {
         this->set(width,
                   height,
                   bit_depth,
                   color_type,
                   compression_method,
                   filter_method,
                   interlace_method);
      }
      Header(const Header &other) : ChunkVec(other) {}

      /// @brief Set the values of this header all at once.
      /// @param width Set the width, in pixels, of this PNG image.
      /// @param height Set the height, in pixels, of this PNG image.
      /// @param bit_depth Set the bit depth of the image. Valid values are 1, 2, 4, 8 and 16, depending on color type.
      /// @param color_type Set the color type of this image. Use facade::png::ColorType for valid values.
      /// @param compression_method Set the compression method. The only valid type is 0.
      /// @param filter_method Set the filter method of this PNG image. The only currently supported type is 0.
      /// @param interlace_method Set the interlace method of this PNG image. The only currently supported type is 0.
      /// @warning This interface explicitly allows you to set invalid types! If you set an invalid type, you may encounter
      ///          exceptions when parsing or creating an image.
      void set(std::uint32_t width, std::uint32_t height, std::uint8_t bit_depth,
               std::uint8_t color_type, std::uint8_t compression_method=0,
               std::uint8_t filter_method=0, std::uint8_t interlace_method=0);

      /// @brief Get the width value of this header.
      ///
      std::uint32_t width() const;
      /// @brief Set the width value of this header.
      ///
      void set_width(std::uint32_t width);

      /// @brief Get the height value of this header.
      std::uint32_t height() const;
      /// @brief Set the height value of this header.
      void set_height(std::uint32_t height);

      /// @brief Get the bit depth of this header.
      std::uint8_t bit_depth() const;
      /// @brief Set the bit depth of this header.
      void set_bit_depth(std::uint8_t bit_depth);

      /// @brief Get the color type of this header.
      std::uint8_t color_type() const;
      /// @brief Set the color type of this header.
      void set_color_type(std::uint8_t color_type);

      /// @brief Get the compression method of this header.
      std::uint8_t compression_method() const;
      /// @brief Set the compression method of this header.
      void set_compression_method(std::uint8_t compression_method);

      /// @brief Get the filter method of this header.
      std::uint8_t filter_method() const;
      /// @brief Set the filter method of this header.
      void set_filter_method(std::uint8_t filter_method);

      /// @brief Get the interlace method of this header.
      std::uint8_t interlace_method() const;
      /// @brief Set the filter method of this header.
      void set_interlace_method(std::uint8_t interlace_method);

      /// @brief Get the pixel type associated with this PNG image.
      /// @sa facade::png::PixelEnum
      /// @throws facade::exception::InvalidBitDepth
      /// @throws facade::exception::InvalidColorType
      ///
      PixelEnum pixel_type() const;
      /// @brief Get the size, in **bits**, of the current pixel type.
      /// @throws facade::exception::InvalidPixelType
      ///
      std::size_t pixel_size() const;
      /// @brief Return the expected raw pixel data buffer size of the decompressed image data, in bytes.
      ///
      std::size_t buffer_size() const;
   };

   /// @brief A `tEXt` chunk object.
   ///
   class
   EXPORT
   Text : public ChunkVec {
   public:
      Text() : ChunkVec(std::string("tEXt")) {}
      Text(std::string keyword, std::string text) : ChunkVec(std::string("tEXt")) {
         this->set_keyword(keyword);
         this->set_text(text);
      }
      Text(const Text &other) : ChunkVec(other) {}

   protected:
      /// @brief Get the null terminator, if present, separating the keyword from the text value.
      /// @return std::nullopt if no null terminator is present, the offset to the null terminator otherwise.
      ///
      std::optional<std::size_t> null_terminator() const;

      /// @brief Return the offset to the underlying text data.
      ///
      std::size_t text_offset() const;

   public:
      /// @brief Check if this `tEXt` chunk has a keyword set.
      /// @return True if a keyword is present, false otherwise.
      ///
      bool has_keyword() const;
      /// @brief The keyword value, if present.
      /// @return A std::string representation of the keyword.
      /// @throws facade::exception::NoKeyword
      ///
      std::string keyword() const;
      /// @brief Set the keyword of this `tEXt` chunk, with the option to validate the value given.
      /// @param keyword The keyword string to set.
      /// @param validate Validate with an exception if the keyword is too long. Default is true.
      /// @throws facade::exception::KeywordTooLong
      ///
      void set_keyword(std::string keyword, bool validate=true);

      /// @brief Check if the given `tEXt` chunk has a text value.
      /// @return True if text data is present, false otherwise.
      ///
      bool has_text() const;
      /// @brief Get the text data from this `tEXt` chunk.
      ///
      std::string text() const;
      /// @brief Set the text data for this `tEXt chunk.
      ///
      void set_text(std::string text);
   };

   /// @brief A compressed text chunk.
   ///
   class
   EXPORT
   ZText : public ChunkVec {
   public:
      ZText() : ChunkVec(std::string("zTXt")) {}
      ZText(std::string keyword, std::string text) : ChunkVec(std::string("zTXt")) {
         this->set_keyword(keyword);
         this->set_compression_method(0);
         this->set_text(text);
      }
      ZText(const Text &other) : ChunkVec(other) {}

   protected:
      /// @brief Get the null terminator, if present, separating the keyword from the text value.
      /// @return std::nullopt if no null terminator is present, the offset to the null terminator otherwise.
      ///
      std::optional<std::size_t> null_terminator() const;

      /// @brief Return the offset to the underlying text data.
      ///
      std::size_t text_offset() const;

   public:
      /// @brief Check if this `ztXt` chunk has a keyword set.
      /// @return True if a keyword is present, false otherwise.
      ///
      bool has_keyword() const;
      /// @brief The keyword value, if present.
      /// @return A std::string representation of the keyword.
      /// @throws facade::exception::NoKeyword
      ///
      std::string keyword() const;
      /// @brief Set the keyword of this `zTXt` chunk, with the option to validate the value given.
      /// @param keyword The keyword string to set.
      /// @param validate Validate with an exception if the keyword is too long. Default is true.
      /// @throws facade::exception::KeywordTooLong
      ///
      void set_keyword(std::string keyword, bool validate=true);

      /// @brief Return the compression method used for this `zTXt` chunk.
      std::uint8_t compression_method() const;
      /// @brief Set the compression method for this `zTXt` chunk.
      ///
      /// The only valid method is 0.
      ///
      void set_compression_method(std::uint8_t compression_method);

      /// @brief Check if the given `zTXt` chunk has a text value.
      /// @return True if text data is present, false otherwise.
      ///
      bool has_text() const;
      /// @brief Get the text data from this `zTXt` chunk.
      ///
      std::string text() const;
      /// @brief Set the text data for this `zTXt chunk.
      ///
      void set_text(std::string text);
   };

   /// @brief The end chunk for a given PNG file.
   ///
   class
   EXPORT
   End : public ChunkVec {
   public:
      End() : ChunkVec(std::string("IEND")) {}
      End(const End &other) : ChunkVec(other) {}
   };

   /// @brief The filter type to use for a given scanline.
   ///
   enum FilterType
   {
      NONE = 0,
      SUB,
      UP,
      AVERAGE,
      PAETH
   };

   /// @brief The base scanline class containing a row of facade::png::PixelSpan of the given pixel type.
   /// @tparam PixelType The pixel type this scanline holds.
   ///
   template <typename PixelType>
   class
   EXPORT
   ScanlineBase
   {
      ASSERT_PIXEL(PixelType);
      
   public:
      /// @brief The pixel span this scanline holds.
      using Span = PixelSpan<PixelType>;

   private:
      std::uint8_t _filter_type;
      std::vector<Span> _pixel_data;

   public:
      ScanlineBase() : _filter_type(0) {}
      ScanlineBase(std::uint8_t filter_type, std::size_t width)
         : _filter_type(filter_type),
           _pixel_data(width / Span::Samples + static_cast<int>(width % Span::Samples != 0)) {}
      ScanlineBase(std::uint8_t filter_type, std::vector<Span> pixel_data) : _filter_type(filter_type), _pixel_data(pixel_data) {}
      ScanlineBase(const ScanlineBase &other) : _filter_type(other._filter_type), _pixel_data(other._pixel_data) {}

      /// @brief Create a scanline object from raw data at the given data offset.
      /// @param raw_data The raw data byte vector to read from.
      /// @param offset The offset to begin reading the scanline.
      /// @param width The width of the scanline.
      /// @return The parsed scanline from the raw data.
      /// @throws facade::exception::OutOfBounds
      ///
      static ScanlineBase read_line(const std::vector<std::uint8_t> &raw_data, std::size_t offset, std::size_t width);

      /// @brief Collect a vector of scanlines from the given raw data.
      /// @param header The header of the given image to collect scanlines from.
      /// @param raw_data The raw pixel data from the uncompressed `IDAT` chunks.
      /// @return A vector of scanlines of the given scanline type.
      /// @throws facade::exception::PixelMismatch
      /// @throws facade::exception::OutOfBounds
      ///
      static std::vector<ScanlineBase> from_raw(const Header &header, const std::vector<std::uint8_t> &raw_data);

      /// @brief Syntactic sugar for getting a facade::png::Pixel variant.
      /// @sa facade::png::ScanlineBase::get_pixel
      ///
      Pixel operator[](std::size_t index) const;

      /// @brief Get the filter type of this scanline.
      ///
      std::uint8_t filter_type() const;
      /// @brief Set the filter type of this scanline.
      ///
      void set_filter_type(std::uint8_t filter_type);

#if !defined(LIBFACADE_WIN32)
      __attribute__((used))
#endif
      /// @brief Return a const reference to the underlying pixel data array.
      ///
      const std::vector<Span> &pixel_data() const;

      /// @brief Return the size, in terms of pixel span objects, of the underlying scanline.
      /// @warning This is not always equivalent to facade::png::Header::width-- sometimes it's less than that,
      ///          due to pixel types that go below 8 bits.
      /// @sa facade::png::ScanlineBase::pixel_width
      ///
      std::size_t pixel_span() const;
      /// @brief Return the size, in samples, of the given scanline.
      /// @warning This is not always equivalent to facade::png::Header::width-- sometimes it's more than that,
      ///          due to padding issues with samples less than 8 bits. You should know that this returns what the
      ///          scanline is *capable* of holding, and not necessarily so much what it *is* holding.
      ///
      std::size_t pixel_width() const;

      /// @brief Get the facade::png::PixelSpan reference at the given index.
      ///
      /// Index, in this case, is bound by facade::png::ScanlineBase::pixel_span.
      ///
      /// @throws facade::exception::OutOfBounds
      ///
      Span &get_span(std::size_t index);
      /// @brief Get the const facade::png::PixelSpan reference at the given index.
      ///
      /// Index, in this case, is bound by facade::png::ScanlineBase::pixel_span.
      ///
      /// @throws facade::exception::OutOfBounds
      ///
      const Span &get_span(std::size_t index) const;
      /// @brief Set the facade::png::PixelSpan object at the given index.
      ///
      /// Index, in this case, is bound by facade::png::ScanlineBase::pixel_span.
      ///
      /// @param span The pixel span to set.
      /// @param index The index of the span to set it to.
      /// @throws facade::exception::OutOfBounds
      ///
      void set_span(const Span &span, std::size_t index);

      /// @brief Get the pixel at the given index.
      ///
      /// Index, in this case, is bound by the value returned by facade::png::Header::width.
      ///
      /// @throws facade::exception::OutOfBounds
      ///
      Pixel get_pixel(std::size_t index) const;
      /// @brief Set the pixel at the given index.
      ///
      /// Index, in this case, is bound by the value returned by facade::png::Header::width.
      ///
      /// @throws facade::exception::OutOfBounds
      ///
      void set_pixel(const Pixel &pixel, std::size_t index);

      /// @brief Convert this scanline to raw byte form.
      ///
      std::vector<std::uint8_t> to_raw() const;

      /// @brief Reconstruct the scanline based on its filter value.
      ///
      /// This essentially performs the inverse of what the facade::png::ScanlineBase::filter functions do.
      ///
      /// @param previous The previous scanline, if any. This is sometimes necessary for reconstruction procedures.
      /// @return The reconstructed scanline.
      /// @throws facade::exception::ScanlineMismatch
      /// @throws facade::exception::NoPixels
      /// @throws facade::exception::InvalidFilterType
      ///
      ScanlineBase reconstruct(std::optional<ScanlineBase> previous) const;
      /// @brief Calculate all filters and determine the best compressed filter among them.
      /// @return The newly filtered scanline.
      /// @param previous The previous scanline, if any. This is sometimes necessary for reconstruction procedures.
      /// @sa facade::png::ScanlineBase::filter(std::uint8_t,std::optional<facade::png::ScanlineBase<PixelType>>)
      ///
      ScanlineBase filter(std::optional<ScanlineBase> previous) const;
      /// @brief Calculate the given filter type on this scanline.
      /// @param filter_type The facade::png::FilterType to assign on the scanline.
      /// @param previous The previous scanline that is sometimes necessary for filtering. Typically the previous scanline if y > 0.
      /// @return The filtered scanline result.
      /// @throws facade::exception::AlreadyFiltered
      /// @throws facade::exception::ScanlineMismatch
      /// @throws facade::exception::NoPixels
      /// @throws facade::exception::InvalidFilterType
      ScanlineBase filter(FilterType filter_type, std::optional<ScanlineBase> previous) const;
   };

   /// @brief A 1-bit grayscale scanline, using facade::png::GrayscalePixel1Bit as a pixel base.
   using GrayscaleScanline1Bit = ScanlineBase<GrayscalePixel1Bit>;
   /// @brief A 2-bit grayscale scanline, using facade::png::GrayscalePixel2Bit as a pixel base.
   using GrayscaleScanline2Bit = ScanlineBase<GrayscalePixel2Bit>;
   /// @brief A 4-bit grayscale scanline, using facade::png::GrayscalePixel4Bit as a pixel base.
   using GrayscaleScanline4Bit = ScanlineBase<GrayscalePixel4Bit>;
   /// @brief An 8-bit grayscale scanline, using facade::png::GrayscalePixel8Bit as a pixel base.
   using GrayscaleScanline8Bit = ScanlineBase<GrayscalePixel8Bit>;
   /// @brief A 16-bit grayscale scanline, using facade::png::GrayscalePixel16Bit as a pixel base.
   using GrayscaleScanline16Bit = ScanlineBase<GrayscalePixel16Bit>;
   /// @brief An 8-bit RGB scanline, using facade::png::TrueColorPixel8Bit as a pixel base.
   using TrueColorScanline8Bit = ScanlineBase<TrueColorPixel8Bit>;
   /// @brief A 16-bit RGB scanline, using facade::png::TrueColorPixel16Bit as a pixel base.
   using TrueColorScanline16Bit = ScanlineBase<TrueColorPixel16Bit>;
   /// @brief A 1-bit palette scanline, using facade::png::PalettePixel1Bit as a pixel base.
   using PaletteScanline1Bit = ScanlineBase<PalettePixel1Bit>;
   /// @brief A 2-bit palette scanline, using facade::png::PalettePixel2Bit as a pixel base.
   using PaletteScanline2Bit = ScanlineBase<PalettePixel2Bit>;
   /// @brief A 4-bit palette scanline, using facade::png::PalettePixel4Bit as a pixel base.
   using PaletteScanline4Bit = ScanlineBase<PalettePixel4Bit>;
   /// @brief An 8-bit palette scanline, using facade::png::PalettePixel8Bit as a pixel base.
   using PaletteScanline8Bit = ScanlineBase<PalettePixel8Bit>;
   /// @brief An 8-bit grayscale scanline with alpha channel, using facade::png::AlphaGrayscalePixel8Bit as a pixel base.
   using AlphaGrayscaleScanline8Bit = ScanlineBase<AlphaGrayscalePixel8Bit>;
   /// @brief A 16-bit grayscale scanline with alpha channel, using facade::png::AlphaGrayscalePixel16Bit as a pixel base.
   using AlphaGrayscaleScanline16Bit = ScanlineBase<AlphaGrayscalePixel16Bit>;
   /// @brief An 8-bit RGB scanline with alpha channel, using facade::png::AlphaTrueColorPixel8Bit as a pixel base.
   using AlphaTrueColorScanline8Bit = ScanlineBase<AlphaTrueColorPixel8Bit>;
   /// @brief A 16-bit RGB scanline with alpha channel, using facade::png::AlphaTrueColorPixel16Bit a pixel base.
   using AlphaTrueColorScanline16Bit = ScanlineBase<AlphaTrueColorPixel16Bit>;

   /// @brief A variant type containing all accepted scanline types.
   ///
   /// This is primarily to make building a new class on top of the variant easier.
   ///
   using ScanlineVariant = std::variant<GrayscaleScanline1Bit,
                                        GrayscaleScanline2Bit,
                                        GrayscaleScanline4Bit,
                                        GrayscaleScanline8Bit,
                                        GrayscaleScanline16Bit,
                                        TrueColorScanline8Bit,
                                        TrueColorScanline16Bit,
                                        PaletteScanline1Bit,
                                        PaletteScanline2Bit,
                                        PaletteScanline4Bit,
                                        PaletteScanline8Bit,
                                        AlphaGrayscaleScanline8Bit,
                                        AlphaGrayscaleScanline16Bit,
                                        AlphaTrueColorScanline8Bit,
                                        AlphaTrueColorScanline16Bit>;

   /// @brief A wrapper object for ScanlineVariant.
   ///
   class
   EXPORT
   Scanline : public ScanlineVariant
   {
   public:
      Scanline() : ScanlineVariant() {}
      Scanline(const GrayscaleScanline1Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const GrayscaleScanline2Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const GrayscaleScanline4Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const GrayscaleScanline8Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const GrayscaleScanline16Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const TrueColorScanline8Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const TrueColorScanline16Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const PaletteScanline1Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const PaletteScanline2Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const PaletteScanline4Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const PaletteScanline8Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const AlphaGrayscaleScanline8Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const AlphaGrayscaleScanline16Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const AlphaTrueColorScanline8Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const AlphaTrueColorScanline16Bit &scanline) : ScanlineVariant(scanline) {}
      Scanline(const Scanline &other) : ScanlineVariant(other) {}

      /// @brief Syntactic sugar for accessing a pixel in the variant.
      /// @sa facade::png::Scanline::get_pixel
      /// @sa facade::png::ScanlineBase::get_pixel
      Pixel operator[](std::size_t index) const;

      /// @brief Visit the variant type held and get the filter type of the held scanline.
      /// @sa facade::png::ScanlineBase::filter_type
      ///
      std::uint8_t filter_type() const;
      /// @brief Visit the variant type held and set the filter type.
      /// @sa facade::png::ScanlineBase::set_filter_type
      ///
      void set_filter_type(std::uint8_t filter_type);

      /// @brief Visit the variant type held and get the pixel span value.
      /// @sa facade::png::ScanlineBase::pixel_span
      ///
      std::size_t pixel_span() const;
      /// @brief Visit the variant type held and get the pixel width value.
      /// @sa facade::png::ScanlineBase::pixel_width
      ///
      std::size_t pixel_width() const;

      /// @brief Visit the variant type held and get the pixel at the given index.
      /// @sa facade::png::ScanlineBase::get_pixel
      ///
      Pixel get_pixel(std::size_t index) const;
      /// @brief Visit the variant type held and set the pixel at the given index.
      /// @sa facade::png::ScanlineBase::set_pixel
      ///
      void set_pixel(const Pixel &pixel, std::size_t index);

      /// @brief Visit the variant type held and get the raw bytes of the pixels represented.
      /// @sa facade::png::ScanlineBase::to_raw
      ///
      std::vector<std::uint8_t> to_raw() const;
   };

   /// @brief A class for loading and manipulating PNG images.
   ///
   /// Here is an example of how to use this class object in particular:
   /// @include png_manipulation.cpp
   ///
   class
   EXPORT
   Image
   {
   public:
      /// @brief The header signature of a valid PNG file.
      static const std::uint8_t Signature[8];
            
   protected:
      /// @brief A map of chunk tags to their corresponding facade::png::ChunkVec types.
      std::map<std::string, std::vector<ChunkVec>> chunk_map;
      /// @brief A container for trailing data, if present when parsing or when writing afterward.
      std::optional<std::vector<std::uint8_t>> trailing_data;
      /// @brief The loaded image data from the compressed `IDAT` chunks.
      std::optional<std::vector<Scanline>> image_data;

   public:
      Image() {}
      Image(const void *ptr, std::size_t size, bool validate=true) { this->parse(ptr, size, validate); }
      Image(const std::vector<std::uint8_t> &data, bool validate=true) { this->parse(data, validate); }
      Image(const std::string &filename, bool validate=true) { this->parse(filename, validate); }
      Image(const Image &other) : chunk_map(other.chunk_map), trailing_data(other.trailing_data), image_data(other.image_data) {}

      /// @brief Syntatic sugar for assigning to an image object.
      Image &operator=(const Image &other);

      /// @brief Syntactic sugar for getting a scanline from the loaded image.
      /// @sa facade::png::Image::scanline
      ///
      Scanline &operator[](std::size_t index);
      /// @brief Syntactic sugar for getting a const scanline from the loaded image.
      /// @sa facade::png::Image::scanline
      ///
      const Scanline &operator[](std::size_t index) const;

      /// @brief Check for the presence of a given chunk tag.
      /// @return True if one or more chunks with that chunk tag were found, false otherwise.
      ///
      bool has_chunk(const std::string &tag) const;
      /// @brief Get the chunk data for the corresponding tag.
      /// @return A vector of one or more chunks corresponding to the given chunk tag.
      /// @throws facade::exception::ChunkNotFound
      ///
      std::vector<ChunkVec> get_chunks(const std::string &tag) const;
      /// @brief Add a given chunk to the underlying image.
      ///
      void add_chunk(const ChunkVec &chunk);

      /// @brief Return whether or not this PNG image has trailing data.
      ///
      bool has_trailing_data() const;
      /// @brief Get the trailing data in the image.
      /// @return The trailing data in the image.
      /// @throws facade::exception::NoTrailingData
      ///
      std::vector<std::uint8_t> &get_trailing_data();
      /// @brief Get the const trailing data in the image.
      /// @return The trailing data in the image.
      /// @throws facade::exception::NoTrailingData
      ///
      const std::vector<std::uint8_t> &get_trailing_data() const;
      /// @brief Set the trailing data of the PNG image.
      ///
      void set_trailing_data(const std::vector<std::uint8_t> &data);
      /// @brief Clear the trailing data in the PNG image.
      ///
      void clear_trailing_data();

      /// @brief Parse a given data buffer into its individual chunks for further processing.
      /// @param ptr The data pointer to parse.
      /// @param size The size, in bytes, of the data pointer.
      /// @param validate If true, validate the checksums in the PNG chunks. Default is true.
      /// @throws facade::exception::InsufficientSize
      /// @throws facade::exception::BadPNGSignature
      /// @throws facade::exception::BadCRC
      ///
      void parse(const void *ptr, std::size_t size, bool validate=true);
      /// @brief Parse a given data vector for a PNG image.
      /// @sa facade::png::Image::parse(const void *, std::size_t, bool)
      ///
      void parse(const std::vector<std::uint8_t> &data, bool validate=true);
      /// @brief Read a file and parse it as a PNG file.
      /// @throws facade::exception::OpenFileFailure
      /// @sa facade::png::Image::parse(const void *, std::size_t, bool)
      /// @sa facade::read_file
      ///
      void parse(const std::string &filename, bool validate=true);

      /// @brief Load the image data from the `IDAT` chunks.
      ///
      /// This decompresses and reconstructs the image data in the parsed file or stream.
      ///
      /// @sa facade::png::Image::decompress
      /// @sa facade::png::Image::reconstruct
      ///
      void load();

      /// @brief Get the scanline at the given y index.
      /// @return The scanline at the given Y index.
      /// @throws facade::exception::NoImageData
      /// @throws facade::exception::OutOfBounds
      ///
      Scanline &scanline(std::size_t index);
      /// @brief Get the const scanline at the given y index.
      /// @return The scanline at the given Y index.
      /// @throws facade::exception::NoImageData
      /// @throws facade::exception::OutOfBounds
      ///
      const Scanline &scanline(std::size_t index) const;

      /// @brief Check if this PNG image has a header present.
      ///
      bool has_header() const;
      /// @brief Get the header present in the PNG image.
      /// @return A facade::png::Header reference to the data.
      /// @throws facade::exception::NoHeaderChunk
      ///
      Header &header();
      /// @brief Get the const header present in the PNG image.
      /// @return A const facade::png::Header reference to the data.
      /// @throws facade::exception::NoHeaderChunk
      ///
      const Header &header() const;
      /// @brief Create and return a new blank header in the PNG image.
      ///
      Header &new_header();

      /// @brief Get the width, in pixels, of this image.
      /// @throws facade::exception::NoHeaderChunk
      ///
      std::size_t width() const;
      /// @brief Get the height, in pixels, of this image.
      /// @throws facade::exception::NoHeaderChunk
      ///
      std::size_t height() const;

      /// @brief Return if the image has any `IDAT` chunks present.
      ///
      bool has_image_data() const;
      /// @brief Check if the image data has been extracted from the `IDAT` chunks.
      ///
      /// Note this does not check if the image has been reconstructed.
      ///
      bool is_loaded() const;

      /// @brief Decompress the `IDAT` chunks in the image.
      /// @throws facade::exception::ZLibError
      /// 
      void decompress();
      /// @brief Compress the image data into `IDAT` chunks.
      /// @param chunk_size The optional chunk size of the fully compressed image data. If present, it splits
      ///                   the image data into as many data chunks as necessary at the given boundary.
      ///                   If set to std::nullopt, the compressed data will be present in one single chunk.
      ///                   The default value is 8192.
      /// @param level The level of compression to employ. Default is -1.
      /// @throws facade::exception::ZLibError
      /// @throws facade::exception::NoImageData
      /// @sa facade::compress
      ///
      void compress(std::optional<std::size_t> chunk_size=8192, int level=-1);

      /// @brief Reconstruct the filtered image data into their raw, unfiltered form.
      /// @throws facade::exception::NoImageData
      /// @sa facade::png::ScanlineBase::reconstruct
      ///
      void reconstruct();
      /// @brief Filter the image data to prepare it for compression.
      /// @throws facade::exception::NoImageData
      /// @sa facade::png::ScanlineBase::filter
      ///
      void filter();

      /// @brief Convert this object into an image buffer fit for saving to a .png file.
      ///
      /// This essentially reconstructs all the chunks and their various types (including
      /// custom chunks) into an appropriate order, tacks on an `IEND` chunk if not present,
      /// the trailing data if present, then returns the vector data.
      ///
      std::vector<std::uint8_t> to_file() const;
      /// @brief Call facade::png::Image::to_file and save that file to disk.
      /// @sa facade::png::Image::to_file
      /// @sa facade::write_file
      ///
      void save(const std::string &filename) const;

      /// @brief Return whether or not the image contains a `tEXt` chunk.
      ///
      bool has_text() const;
      /// @brief Add a text chunk to the PNG image.
      /// @param keyword The keyword to give the `tEXt` chunk.
      /// @param text The text to give the `tEXt` chunk.
      /// @return A facade::png::Text chunk reference of the newly added `tEXt` section.
      /// @sa facade::png::Text
      ///
      Text &add_text(const std::string &keyword, const std::string &text);
      /// @brief Remove the given `tEXt` section from the image.
      /// @throws facade::exception::TextNotFound
      ///
      void remove_text(const Text &text);
      /// @brief Remove a `tEXt` section by keyword and text.
      /// @throws facade::exception::TextNotFound
      ///
      void remove_text(const std::string &keyword, const std::string &text);
      /// @brief Get the `tEXt` sections with the following keyword.
      /// @return A vector of returned results. An empty vector means the keyword wasn't found.
      ///
      std::vector<Text> get_text(const std::string &keyword) const;

      /// @brief Return whether or not the image contains a `zTXt` chunk.
      ///
      bool has_ztext() const;
      /// @brief Add a `zTXt` chunk to the PNG image.
      /// @param keyword The keyword to give the `zTXt` chunk.
      /// @param text The text to give the `zTXt` chunk.
      /// @return A facade::png::ZText chunk reference of the newly added `zTXt` section.
      /// @sa facade::png::ZText
      ///
      ZText &add_ztext(const std::string &keyword, const std::string &text);
      /// @brief Remove the given `zTXt` section from the image.
      /// @throws facade::exception::TextNotFound
      ///
      void remove_ztext(const ZText &ztext);
      /// @brief Remove a `zTXt` section by keyword and text.
      /// @throws facade::exception::TextNotFound
      ///
      void remove_ztext(const std::string &keyword, const std::string &text);
      /// @brief Get the `zTXt` sections with the following keyword.
      /// @return A vector of returned results. An empty vector means the keyword wasn't found.
      ///
      std::vector<ZText> get_ztext(const std::string &keyword) const;
   };
}}

#endif
