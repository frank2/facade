#ifndef __FACADE_PNG_HPP
#define __FACADE_PNG_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
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
   PACK(1)
   EXPORT class ChunkTag
   {
      std::uint8_t _tag[4];

   public:
      ChunkTag() { std::memset(this->_tag, 0, sizeof(std::uint8_t) * 4); }
      ChunkTag(const std::uint8_t *tag) { this->set_tag(tag); }
      ChunkTag(const char *tag) { this->set_tag(std::string(tag)); }
      ChunkTag(const std::string tag) { this->set_tag(tag); }
      ChunkTag(const ChunkTag &other) { this->set_tag(&other._tag[0]); }

      friend bool operator==(const ChunkTag &left, const ChunkTag &right);

      void set_tag(const std::string tag);
      void set_tag(const std::uint8_t *tag);
      std::uint8_t *tag();
      const std::uint8_t *tag() const;
      std::string to_string() const;
   };
   UNPACK()

   class ChunkPtr;

   EXPORT class ChunkVec
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

      friend bool operator==(const ChunkVec &left, const ChunkVec &right);

      std::size_t length() const;

      ChunkTag &tag();
      const ChunkTag &tag() const;
                  
      std::vector<std::uint8_t> &data();
      const std::vector<std::uint8_t> &data() const;
      void set_data(std::vector<std::uint8_t> &data);

      std::uint32_t crc() const;
         
      std::pair<std::vector<std::uint8_t>, ChunkPtr> to_chunk_ptr() const;

      template <typename T>
      T& upcast() {
         static_assert(std::is_base_of<ChunkVec, T>::value,
                       "Upcast type must derive ChunkVec");

         return *static_cast<T*>(this);
      }

      template <typename T>
      const T& upcast() const {
         static_assert(std::is_base_of<ChunkVec, T>::value,
                       "Upcast type must derive ChunkVec");
         
         return *static_cast<const T*>(this);
      }

      ChunkVec &as_chunk_vec();
      const ChunkVec &as_chunk_vec() const;
   };
      
   EXPORT class ChunkPtr
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
      
      static ChunkPtr parse(const void *ptr, std::size_t size, std::size_t offset);
      static ChunkPtr parse(const std::vector<std::uint8_t> &vec, std::size_t offset);

      std::size_t length() const;
      ChunkTag tag() const;
      std::vector<std::uint8_t> data() const;
      std::uint32_t crc() const;
      bool validate() const;
      std::size_t chunk_size() const;
      std::vector<std::uint8_t> chunk_data() const;
      ChunkVec to_chunk_vec() const;
   };

   EXPORT enum ColorType
   {
      Grayscale = 0,
      TrueColor = 2,
      Palette = 3,
      AlphaGrayscale = 4,
      AlphaTrueColor = 6
   };

   PACK(1)
   EXPORT
   template <typename _Base=std::uint8_t, std::size_t _Bits=sizeof(_Base)*8>
   class Sample
   {
      static_assert(_Bits == 1 || _Bits == 2 || _Bits == 4 || _Bits == 8 || _Bits == 16,
                    "Bits must be 1, 2, 4, 8 or 16.");
      static_assert(std::is_same<_Base,std::uint8_t>::value || std::is_same<_Base,std::uint16_t>::value,
                    "Base must be uint8_t or uint16_t.");
      static_assert(std::is_same<_Base,std::uint16_t>::value && _Bits == 16 ||
                    std::is_same<_Base,std::uint8_t>::value && _Bits <= 8,
                    "Bit size does not match the base type.");

      _Base _value;

   public:
      using Base = _Base;
      const static std::size_t Bits = _Bits;
      const static std::size_t Max = (1 << Bits) - 1;
      
      Sample() : _value(0) {}
      Sample(Base value) { this->set_value(value); }
      Sample(const Sample &other) { this->set_value(other._value); }

      Base operator*() const { return this->value(); }
      Sample &operator=(Base value) { this->set_value(value); return *this; }

      Base value() const {
         if constexpr (Bits == 16) { return endian_swap_16(this->_value); }
         else { return this->_value & this->Max; }
      }
      
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

   template <typename _Base=std::uint8_t, std::size_t _Bits=sizeof(_Base)*8>
   class GrayscalePixel : public Sample<_Base, _Bits>
   {
   public:
      GrayscalePixel() : Sample<_Base, _Bits>(0) {}
      GrayscalePixel(_Base value) : Sample<_Base, _Bits>(value) {}
      GrayscalePixel(const GrayscalePixel &other) : Sample<_Base, _Bits>(other) {}

      /* TODO: color conversion functions */
   };

   using GrayscalePixel1Bit = GrayscalePixel<std::uint8_t, 1>;
   using GrayscalePixel2Bit = GrayscalePixel<std::uint8_t, 2>;
   using GrayscalePixel4Bit = GrayscalePixel<std::uint8_t, 4>;
   using GrayscalePixel8Bit = GrayscalePixel<std::uint8_t>;
   using GrayscalePixel16Bit = GrayscalePixel<std::uint16_t>;
   
   PACK(1)
   EXPORT
   template <typename _Sample=Sample8Bit>
   class TrueColorPixel
   {
      static_assert(std::is_same<_Sample,Sample8Bit>::value || std::is_same<_Sample,Sample16Bit>::value,
                    "Sample type must be Sample8Bit or Sample16Bit.");
      
      _Sample _red, _green, _blue;

   public:
      using Sample = _Sample;
      using Base = typename Sample::Base;
      const static std::size_t Max = Sample::Max;
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
         
      Sample &red() { return this->_red; }
      const Sample &red() const { return this->_red; }
      
      Sample &green() { return this->_green; }
      const Sample &green() const { return this->_green; }

      Sample &blue() { return this->_blue; }
      const Sample &blue() const { return this->_blue;}
   };
   UNPACK()

   using TrueColorPixel8Bit = TrueColorPixel<Sample8Bit>;
   using TrueColorPixel16Bit = TrueColorPixel<Sample16Bit>;

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

   using PalettePixel1Bit = PalettePixel<1>;
   using PalettePixel2Bit = PalettePixel<2>;
   using PalettePixel4Bit = PalettePixel<4>;
   using PalettePixel8Bit = PalettePixel<8>;

   PACK(1)
   EXPORT
   template <typename _Base=std::uint8_t>
   class AlphaGrayscalePixel : public GrayscalePixel<_Base>
   {
      static_assert(std::is_same<_Base,std::uint8_t>::value || std::is_same<_Base,std::uint16_t>::value,
                    "Base must be uint8_t or uint16_t.");
   public:
      using Sample = Sample<_Base>;
      using Base = typename Sample::Base;

   private:
      Sample _alpha;

   public:
      const static std::size_t Bits = Sample::Bits*2;
      
      AlphaGrayscalePixel() : _alpha(0), GrayscalePixel<_Base>() {}
      AlphaGrayscalePixel(Sample value, Sample alpha) : _alpha(alpha), GrayscalePixel<_Base>(value) {}
      AlphaGrayscalePixel(const AlphaGrayscalePixel &other) : _alpha(other._alpha), GrayscalePixel<_Base>(other) {}

      Sample &alpha() { return this->_alpha; }
      const Sample &alpha() const { return this->_alpha; }
   };
   UNPACK()

   using AlphaGrayscalePixel8Bit = AlphaGrayscalePixel<std::uint8_t>;
   using AlphaGrayscalePixel16Bit = AlphaGrayscalePixel<std::uint16_t>;
   
   PACK(1)
   EXPORT
   template <typename _Sample=Sample8Bit>
   class AlphaTrueColorPixel : public TrueColorPixel<_Sample>
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
      
      _Sample &alpha() { return this->_alpha; }
      const _Sample &alpha() const { return this->_alpha; }
   };
   UNPACK()

   using AlphaTrueColorPixel8Bit = AlphaTrueColorPixel<Sample8Bit>;
   using AlphaTrueColorPixel16Bit = AlphaTrueColorPixel<Sample16Bit>;

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

   EXPORT
   template <typename PixelType>
   class PixelSpan
   {
      union {
         std::uint8_t bits;
         std::uint8_t bytes[PixelType::Bits/8];
      } _data;

   public:
      PixelSpan() {}
      PixelSpan(const PixelSpan &other) { std::memcpy(&this->_data, &other._data, sizeof(this->_data)); }

      const static std::size_t Samples = (8 >> (PixelType::Bits / 2)) + static_cast<int>(PixelType::Bits >= 8);
   
      Pixel operator[](std::size_t index) const { return this->get(index); }

      const std::uint8_t *data() const {
         auto u8_ptr = reinterpret_cast<const std::uint8_t *>(&this->_data);
         return u8_ptr;
      }

      std::size_t data_size() const { return sizeof(PixelType); }
      
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

   template <typename PixelType>
   using PixelRow = std::vector<PixelSpan<PixelType>>;

   EXPORT
   template <typename PixelType>
   std::vector<std::uint8_t> pixels_to_raw(const PixelRow<PixelType> &pixels)
   {
      std::vector<std::uint8_t> result;

      for (auto &span : pixels)
      {
         auto data = span.data();
         result.insert(result.end(), &data[0], &data[span.data_size()]);
      }

      return result;
   }
   
   EXPORT class Header : public ChunkVec
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

      void set(std::uint32_t width, std::uint32_t height, std::uint8_t bit_depth,
               std::uint8_t color_type, std::uint8_t compression_method=0,
               std::uint8_t filter_method=0, std::uint8_t interlace_method=0);
         
      std::uint32_t width() const;
      void set_width(std::uint32_t width);

      std::uint32_t height() const;
      void set_height(std::uint32_t height);

      std::uint8_t bit_depth() const;
      void set_bit_depth(std::uint8_t bit_depth);

      std::uint8_t color_type() const;
      void set_color_type(std::uint8_t color_type);

      std::uint8_t compression_method() const;
      void set_compression_method(std::uint8_t compression_method);

      std::uint8_t filter_method() const;
      void set_filter_method(std::uint8_t filter_method);

      std::uint8_t interlace_method() const;
      void set_interlace_method(std::uint8_t interlace_method);

      PixelEnum pixel_type() const;
      std::size_t pixel_size() const;
      std::size_t buffer_size() const;
   };

   EXPORT class Text : public ChunkVec {
   public:
      Text() : ChunkVec(std::string("tEXt")) {}
      Text(std::string keyword, std::string text) : ChunkVec(std::string("tEXt")) {
         this->set_keyword(keyword);
         this->set_text(text);
      }
      Text(const Text &other) : ChunkVec(other) {}

   protected:
      std::optional<std::size_t> null_terminator() const;
      std::size_t text_offset() const;

   public:
      bool has_keyword() const;
      std::string keyword() const;
      void set_keyword(std::string keyword, bool validate=true);

      bool has_text() const;
      std::string text() const;
      void set_text(std::string text);
   };

   
   EXPORT class ZText : public ChunkVec {
   public:
      ZText() : ChunkVec(std::string("zTXt")) {}
      ZText(std::string keyword, std::string text) : ChunkVec(std::string("zTXt")) {
         this->set_keyword(keyword);
         this->set_compression_method(0);
         this->set_text(text);
      }
      ZText(const Text &other) : ChunkVec(other) {}

   protected:
      std::optional<std::size_t> null_terminator() const;
      std::size_t text_offset() const;

   public:
      bool has_keyword() const;
      std::string keyword() const;
      void set_keyword(std::string keyword, bool validate=true);

      std::uint8_t compression_method() const;
      void set_compression_method(std::uint8_t compression_method);
      
      bool has_text() const;
      std::string text() const;
      void set_text(std::string text);
   };

   EXPORT class End : public ChunkVec {
   public:
      End() : ChunkVec(std::string("IEND")) {}
      End(const End &other) : ChunkVec(other) {}
   };

   enum FilterType
   {
      None = 0,
      Sub,
      Up,
      Average,
      Paeth
   };

   template <typename PixelType>
   EXPORT class ScanlineBase
   {
   public:
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

      static ScanlineBase read_line(const std::vector<std::uint8_t> &raw_data, std::size_t offset, std::size_t width);
      static std::vector<ScanlineBase> from_raw(const Header &header, const std::vector<std::uint8_t> &raw_data);

      Pixel operator[](std::size_t index) const;
      
      std::uint8_t filter_type() const;
      void set_filter_type(std::uint8_t filter_type);

      const std::vector<Span> &pixel_data() const;

      std::size_t pixel_span() const;
      std::size_t pixel_width() const;

      Span &get_span(std::size_t index);
      const Span &get_span(std::size_t index) const;
      void set_span(const Span &span, std::size_t index);

      Pixel get_pixel(std::size_t index) const;
      void set_pixel(const Pixel &pixel, std::size_t index);

      std::vector<std::uint8_t> to_raw() const;

      ScanlineBase reconstruct(std::optional<ScanlineBase> previous) const;
      ScanlineBase filter(std::optional<ScanlineBase> previous) const;
      ScanlineBase filter(std::uint8_t filter_type, std::optional<ScanlineBase> previous) const;
   };

   using GrayscaleScanline1Bit = ScanlineBase<GrayscalePixel1Bit>;
   using GrayscaleScanline2Bit = ScanlineBase<GrayscalePixel2Bit>;
   using GrayscaleScanline4Bit = ScanlineBase<GrayscalePixel4Bit>;
   using GrayscaleScanline8Bit = ScanlineBase<GrayscalePixel8Bit>;
   using GrayscaleScanline16Bit = ScanlineBase<GrayscalePixel16Bit>;
   using TrueColorScanline8Bit = ScanlineBase<TrueColorPixel8Bit>;
   using TrueColorScanline16Bit = ScanlineBase<TrueColorPixel16Bit>;
   using PaletteScanline1Bit = ScanlineBase<PalettePixel1Bit>;
   using PaletteScanline2Bit = ScanlineBase<PalettePixel2Bit>;
   using PaletteScanline4Bit = ScanlineBase<PalettePixel4Bit>;
   using PaletteScanline8Bit = ScanlineBase<PalettePixel8Bit>;
   using AlphaGrayscaleScanline8Bit = ScanlineBase<AlphaGrayscalePixel8Bit>;
   using AlphaGrayscaleScanline16Bit = ScanlineBase<AlphaGrayscalePixel16Bit>;
   using AlphaTrueColorScanline8Bit = ScanlineBase<AlphaTrueColorPixel8Bit>;
   using AlphaTrueColorScanline16Bit = ScanlineBase<AlphaTrueColorPixel16Bit>;
   
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

   class Scanline : public ScanlineVariant
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

      Pixel operator[](std::size_t index) const;

      std::uint8_t filter_type() const;
      void set_filter_type(std::uint8_t filter_type);

      std::size_t pixel_span() const;
      std::size_t pixel_width() const;

      Pixel get_pixel(std::size_t index) const;
      void set_pixel(const Pixel &pixel, std::size_t index);

      std::vector<std::uint8_t> to_raw() const;
   };
   
   EXPORT class Image
   {
   public:
      static const std::uint8_t Signature[8];
            
   protected:
      std::map<std::string, std::vector<ChunkVec>> chunk_map;
      std::optional<std::vector<std::uint8_t>> trailing_data;
      std::optional<std::vector<Scanline>> image_data;

   public:
      Image() {}
      Image(const void *ptr, std::size_t size, bool validate=true) { this->parse(ptr, size, validate); }
      Image(const std::vector<std::uint8_t> &data, bool validate=true) { this->parse(data, validate); }
      Image(const std::string &filename, bool validate=true) { this->parse(filename, validate); }
      Image(const Image &other) : chunk_map(other.chunk_map), trailing_data(other.trailing_data), image_data(other.image_data) {}

      Scanline &operator[](std::size_t index);
      const Scanline &operator[](std::size_t index) const;

      bool has_chunk(const std::string &tag) const;
      std::vector<ChunkVec> get_chunks(const std::string &tag) const;
      void add_chunk(const std::string &tag, const ChunkVec &chunk);

      bool has_trailing_data() const;
      std::vector<std::uint8_t> &get_trailing_data();
      const std::vector<std::uint8_t> &get_trailing_data() const;
      void set_trailing_data(const std::vector<std::uint8_t> &data);
      void clear_trailing_data();

      void parse(const void *ptr, std::size_t size, bool validate=true);
      void parse(const std::vector<std::uint8_t> &data, bool validate=true); 
      void parse(const std::string &filename, bool validate=true);

      void load();

      Scanline &scanline(std::size_t index);
      const Scanline &scanline(std::size_t index) const;

      bool has_header() const;
      Header &header();
      const Header &header() const;
      Header &new_header();

      std::size_t width() const;
      std::size_t height() const;

      bool has_image_data() const;
      bool is_loaded() const;

      void decompress();
      void compress(std::optional<std::size_t> chunk_size=8192, int level=-1);

      void reconstruct();
      void filter();

      std::vector<std::uint8_t> to_file() const;
      void save(const std::string &filename) const;

      bool has_text() const;
      Text &add_text(const std::string &keyword, const std::string &text);
      void remove_text(const Text &text);
      void remove_text(const std::string &keyword, const std::string &text);
      std::vector<Text> get_text(const std::string &keyword) const;

      bool has_ztext() const;
      ZText &add_ztext(const std::string &keyword, const std::string &text);
      void remove_ztext(const ZText &ztext);
      void remove_ztext(const std::string &keyword, const std::string &text);
      std::vector<ZText> get_ztext(const std::string &keyword) const;
   };
}}

#endif
