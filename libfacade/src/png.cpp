#include <facade.hpp>

using namespace facade;
using namespace facade::png;

const std::uint8_t Image::Signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };

bool png::operator==(const ChunkTag &left, const ChunkTag &right) {
   return left.tag() == right.tag();
}

void ChunkTag::set_tag(const std::string tag) {
   if (tag.size() != 4) { throw exception::InvalidChunkTag(); }

   this->set_tag(reinterpret_cast<const std::uint8_t *>(tag.c_str()));
}

void ChunkTag::set_tag(const std::uint8_t *tag) {
   std::memcpy(this->_tag, tag, sizeof(std::uint8_t) * 4);
}

std::uint8_t *ChunkTag::tag() { return &this->_tag[0]; }
const std::uint8_t *ChunkTag::tag() const { return &this->_tag[0]; }
         
std::string ChunkTag::to_string() const { return std::string(&this->_tag[0], &this->_tag[4]); }

bool png::operator==(const ChunkVec &left, const ChunkVec &right) {
   return left.tag() == right.tag() && left.data() == right.data();
}

std::size_t ChunkVec::length() const {
   return this->_data.size();
}

ChunkTag &ChunkVec::tag() {
   return this->_tag;
}

const ChunkTag &ChunkVec::tag() const {
   return this->_tag;
}

std::vector<std::uint8_t> &ChunkVec::data() {
   return this->_data;
}

const std::vector<std::uint8_t> &ChunkVec::data() const {
   return this->_data;
}

void ChunkVec::set_data(std::vector<std::uint8_t> &vec) {
   this->_data = vec;
}

std::uint32_t ChunkVec::crc() const {
   auto crc = crc32(this->tag().tag(), 4, 0);

   if (this->length() > 0) { crc = crc32(this->data().data(), this->data().size(), crc); }

   return crc;
}

std::pair<std::vector<std::uint8_t>, ChunkPtr> ChunkVec::to_chunk_ptr() const {
   std::vector<std::uint8_t> data;
   auto length = endian_swap_32(static_cast<std::uint32_t>(this->length()));
   auto length_u8 = reinterpret_cast<std::uint8_t *>(&length);
   data.insert(data.end(), &length_u8[0], &length_u8[4]);

   auto tag = this->tag().tag();
   data.insert(data.end(), &tag[0], &tag[4]);
   
   data.insert(data.end(), this->data().begin(), this->data().end());
   
   auto crc = endian_swap_32(this->crc());
   auto crc_u8 = reinterpret_cast<std::uint8_t *>(&crc);
   data.insert(data.end(), &crc_u8[0], &crc_u8[4]);

   auto ptr = ChunkPtr::parse(data.data(), data.size(), 0);

   return std::make_pair(data, ptr);
}

ChunkVec &ChunkVec::as_chunk_vec() {
   return *static_cast<ChunkVec *>(this);
}

const ChunkVec &ChunkVec::as_chunk_vec() const {
   return *static_cast<const ChunkVec *>(this);
}

ChunkPtr ChunkPtr::parse(const void *ptr, std::size_t size, std::size_t offset) {
   if (ptr == nullptr) { throw exception::NullPointer(); }
   if (size == 0) { throw exception::NoData(); }
   
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);

   if (offset+sizeof(std::uint32_t) >= size) { throw exception::OutOfBounds(offset+sizeof(std::uint32_t), size); }
   auto length_ptr = &u8_ptr[offset];
   offset += sizeof(std::uint32_t);

   if (offset+sizeof(ChunkTag) >= size) { throw exception::OutOfBounds(offset+sizeof(ChunkTag), size); }
   auto tag_ptr = &u8_ptr[offset];
   offset += sizeof(ChunkTag);

   auto data_ptr = &u8_ptr[offset];
   auto length = endian_swap_32(*reinterpret_cast<const std::uint32_t *>(length_ptr));
   offset += length;
   if (offset >= size) { throw exception::OutOfBounds(offset, size); }
   if (offset+sizeof(std::uint32_t) > size) { throw exception::OutOfBounds(offset+sizeof(std::uint32_t), size); }
   auto crc_ptr = &u8_ptr[offset];

   return ChunkPtr(reinterpret_cast<const std::uint32_t *>(length_ptr),
                   reinterpret_cast<const ChunkTag *>(tag_ptr),
                   data_ptr,
                   reinterpret_cast<const std::uint32_t *>(crc_ptr));
}

ChunkPtr ChunkPtr::parse(const std::vector<std::uint8_t> &vec, std::size_t offset) {
   return ChunkPtr::parse(vec.data(), vec.size(), offset);
}

std::size_t ChunkPtr::length() const {
   if (this->_length == nullptr) { throw exception::NullPointer(); }

   return endian_swap_32(*this->_length);
}

ChunkTag ChunkPtr::tag() const {
   if (this->_tag == nullptr) { throw exception::NullPointer(); }

   return *this->_tag;
}

std::vector<std::uint8_t> ChunkPtr::data() const {
   if (this->_data == nullptr) { throw exception::NullPointer(); }

   auto length = this->length();

   if (length == 0) { return std::vector<std::uint8_t>(); }
   else { return std::vector<std::uint8_t>(&this->_data[0], &this->_data[length]); }
}

std::uint32_t ChunkPtr::crc() const {
   if (this->_crc == nullptr) { throw exception::NullPointer(); }

   return endian_swap_32(*this->_crc);
}

bool ChunkPtr::validate() const {
   if (this->_tag == nullptr) { throw exception::NullPointer(); }
   auto crc = facade::crc32(this->_tag->tag(), 4, 0);

   if (this->length() > 0) { crc = facade::crc32(this->data().data(), this->length(), crc); }

   return crc == this->crc();
}

std::size_t ChunkPtr::chunk_size() const {
   return sizeof(std::uint32_t)+sizeof(ChunkTag)+this->length()+sizeof(std::uint32_t);
}

std::vector<std::uint8_t> ChunkPtr::chunk_data() const {
   if (this->_length == nullptr) { throw exception::NullPointer(); }

   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(this->_length);
   
   return std::vector<std::uint8_t>(&u8_ptr[0], &u8_ptr[this->chunk_size()]);
}

ChunkVec ChunkPtr::to_chunk_vec() const {
   return ChunkVec(this->tag(), this->data());
}

void Header::set(std::uint32_t width, std::uint32_t height, std::uint8_t bit_depth,
                           std::uint8_t color_type, std::uint8_t compression_method,
                           std::uint8_t filter_method, std::uint8_t interlace_method)
{
   this->set_width(width);
   this->set_height(height);
   this->set_bit_depth(bit_depth);
   this->set_color_type(color_type);
   this->set_compression_method(compression_method);
   this->set_filter_method(filter_method);
   this->set_interlace_method(interlace_method);
}

std::uint32_t Header::width() const {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   return endian_swap_32(*reinterpret_cast<const std::uint32_t *>(&this->data().data()[0]));
}

void Header::set_width(std::uint32_t width) {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   *reinterpret_cast<std::uint32_t *>(&this->data().data()[0]) = endian_swap_32(width);
}

std::uint32_t Header::height() const {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   return endian_swap_32(*reinterpret_cast<const std::uint32_t *>(&this->data().data()[4]));
}

void Header::set_height(std::uint32_t height) {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   *reinterpret_cast<std::uint32_t *>(&this->data().data()[4]) = endian_swap_32(height);
}

std::uint8_t Header::bit_depth() const {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   return this->data().data()[8];
}

void Header::set_bit_depth(std::uint8_t bit_depth) {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   this->data().data()[8] = bit_depth;
}

std::uint8_t Header::color_type() const {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   return this->data().data()[9];
}

void Header::set_color_type(std::uint8_t color_type) {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   this->data().data()[9] = color_type;
}

std::uint8_t Header::compression_method() const {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   return this->data().data()[10];
}

void Header::set_compression_method(std::uint8_t compression_method) {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   this->data().data()[10] = compression_method;
}

std::uint8_t Header::filter_method() const {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   return this->data().data()[11];
}

void Header::set_filter_method(std::uint8_t filter_method) {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   this->data().data()[11] = filter_method;
}

std::uint8_t Header::interlace_method() const {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   return this->data().data()[12];
}

void Header::set_interlace_method(std::uint8_t interlace_method) {
   if (this->length() != 13) { throw exception::InsufficientSize(this->length(), 13); }
   this->data().data()[12] = interlace_method;
}

PixelEnum Header::pixel_type() const {
   switch (this->color_type())
   {
   case ColorType::Grayscale:
   {
      switch (this->bit_depth())
      {
      case 1:
         return PixelEnum::GRAYSCALE_PIXEL_1BIT;
         
      case 2:
         return PixelEnum::GRAYSCALE_PIXEL_2BIT;
         
      case 4:
         return PixelEnum::GRAYSCALE_PIXEL_4BIT;
         
      case 8:
         return PixelEnum::GRAYSCALE_PIXEL_8BIT;

      case 16:
         return PixelEnum::GRAYSCALE_PIXEL_16BIT;

      default:
         throw exception::InvalidBitDepth(this->bit_depth());
      }
   }

   case ColorType::TrueColor:
   {
      switch (this->bit_depth())
      {
      case 8:
         return PixelEnum::TRUE_COLOR_PIXEL_8BIT;

      case 16:
         return PixelEnum::TRUE_COLOR_PIXEL_16BIT;

      default:
         throw exception::InvalidBitDepth(this->bit_depth());
      }
   }

   case ColorType::Palette:
   {
      switch (this->bit_depth())
      {
      case 1:
         return PixelEnum::PALETTE_PIXEL_1BIT;

      case 2:
         return PixelEnum::PALETTE_PIXEL_2BIT;

      case 4:
         return PixelEnum::PALETTE_PIXEL_4BIT;

      case 8:
         return PixelEnum::PALETTE_PIXEL_8BIT;

      default:
         throw exception::InvalidBitDepth(this->bit_depth());
      }
   }

   case ColorType::AlphaGrayscale:
   {
      switch (this->bit_depth())
      {
      case 8:
         return PixelEnum::ALPHA_GRAYSCALE_PIXEL_8BIT;

      case 16:
         return PixelEnum::ALPHA_GRAYSCALE_PIXEL_16BIT;

      default:
         throw exception::InvalidBitDepth(this->bit_depth());
      }
   }

   case ColorType::AlphaTrueColor:
   {
      switch (this->bit_depth())
      {
      case 8:
         return PixelEnum::ALPHA_TRUE_COLOR_PIXEL_8BIT;

      case 16:
         return PixelEnum::ALPHA_TRUE_COLOR_PIXEL_16BIT;

      default:
         throw exception::InvalidBitDepth(this->bit_depth());
      }
   }

   default:
      throw exception::InvalidColorType(this->color_type());
   }
}

std::size_t Header::pixel_size() const {
   switch (this->pixel_type())
   {
   case PixelEnum::GRAYSCALE_PIXEL_1BIT:
      return GrayscalePixel1Bit::Bits;

   case PixelEnum::GRAYSCALE_PIXEL_2BIT:
      return GrayscalePixel2Bit::Bits;
      
   case PixelEnum::GRAYSCALE_PIXEL_4BIT:
      return GrayscalePixel4Bit::Bits;
         
   case PixelEnum::GRAYSCALE_PIXEL_8BIT:
      return GrayscalePixel8Bit::Bits;

   case PixelEnum::GRAYSCALE_PIXEL_16BIT:
      return GrayscalePixel16Bit::Bits;

   case PixelEnum::TRUE_COLOR_PIXEL_8BIT:
      return TrueColorPixel8Bit::Bits;

   case PixelEnum::TRUE_COLOR_PIXEL_16BIT:
      return TrueColorPixel16Bit::Bits;

   case PixelEnum::PALETTE_PIXEL_1BIT:
      return PalettePixel1Bit::Bits;

   case PixelEnum::PALETTE_PIXEL_2BIT:
      return PalettePixel2Bit::Bits;

   case PixelEnum::PALETTE_PIXEL_4BIT:
      return PalettePixel4Bit::Bits;

   case PixelEnum::PALETTE_PIXEL_8BIT:
      return PalettePixel8Bit::Bits;

   case PixelEnum::ALPHA_GRAYSCALE_PIXEL_8BIT:
      return AlphaGrayscalePixel8Bit::Bits;

   case PixelEnum::ALPHA_GRAYSCALE_PIXEL_16BIT:
      return AlphaGrayscalePixel16Bit::Bits;

   case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_8BIT:
      return AlphaTrueColorPixel8Bit::Bits;

   case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_16BIT:
      return AlphaTrueColorPixel16Bit::Bits;
   }

   throw exception::InvalidPixelType(this->pixel_type());
}

std::size_t Header::buffer_size() const {
   auto scanline = this->width() * this->pixel_size();
   auto padded_scanline = scanline + ((scanline % 8 != 0) ? 8 - scanline % 8 : 0);
      
   return (padded_scanline * this->height() + this->height() * 8) / 8;
}

std::optional<std::size_t> Text::null_terminator() const {
   if (this->data().size() == 0) { return std::nullopt; }

   std::size_t zero = 0;

   while (zero < this->data().size())
   {
      if (this->data()[zero] == 0)
         break;

      ++zero;
   }

   if (zero >= this->data().size()) { return std::nullopt; }

   return zero;
}

std::size_t Text::text_offset() const {
   auto zero = this->null_terminator();

   if (zero.has_value()) { return *zero+1; }
   else { return 0; }
}

bool Text::has_keyword() const { return this->null_terminator().has_value(); }

std::string Text::keyword() const {
   if (!this->has_keyword()) { throw exception::NoKeyword(); }

   auto zero = this->null_terminator();

   return std::string(&this->data()[0], &this->data()[*zero]);
}

void Text::set_keyword(std::string keyword, bool validate) {
   if (validate && keyword.size() > 79) { throw exception::KeywordTooLong(); }

   if (this->has_keyword())
   {
      auto zero = this->null_terminator();
      this->data().erase(this->data().begin(), std::next(this->data().begin(), *zero+1));
   }
   
   this->data().insert(this->data().begin(), &keyword.c_str()[0], &keyword.c_str()[keyword.size()+1]);
}

bool Text::has_text() const {
   auto zero = this->null_terminator();

   return zero.has_value() && this->data().size() > *zero+1 || !zero.has_value() && this->data().size() > 0;
}

std::string Text::text() const {
   return std::string(&this->data().at(this->text_offset()), &this->data().at(this->data().size()-1)+1);
}

void Text::set_text(std::string text) {
   if (this->has_text())
      this->data().erase(std::next(this->data().begin(), this->text_offset()), this->data().end());

   this->data().insert(this->data().end(), text.begin(), text.end());
}


std::optional<std::size_t> ZText::null_terminator() const {
   if (this->data().size() == 0) { return std::nullopt; }

   std::size_t zero = 0;

   while (zero < this->data().size())
   {
      if (this->data()[zero] == 0)
         break;

      ++zero;
   }

   if (zero >= this->data().size() || zero == 0) { return std::nullopt; }

   return zero;
}

std::size_t ZText::text_offset() const {
   auto zero = this->null_terminator();

   if (zero.has_value()) { return *zero+2; }
   else { return 1; }
}

bool ZText::has_keyword() const { return this->null_terminator().has_value(); }

std::string ZText::keyword() const {
   if (!this->has_keyword()) { throw exception::NoKeyword(); }

   auto zero = this->null_terminator();

   return std::string(&this->data()[0], &this->data()[*zero]);
}

void ZText::set_keyword(std::string keyword, bool validate) {
   if (validate && keyword.size() > 79) { throw exception::KeywordTooLong(); }

   if (this->has_keyword())
   {
      auto zero = this->null_terminator();
      this->data().erase(this->data().begin(), std::next(this->data().begin(), *zero+1));
   }
   
   this->data().insert(this->data().begin(), &keyword.c_str()[0], &keyword.c_str()[keyword.size()+1]);
}

std::uint8_t ZText::compression_method() const {
   if (!this->has_keyword()) { throw exception::NoKeyword(); }
   
   auto zero = this->null_terminator();

   if (*zero+1 == this->data().size()) { throw exception::OutOfBounds(*zero+1, this->data().size()); }

   return this->data()[*zero+1];
}

void ZText::set_compression_method(std::uint8_t compression_method) {
   if (!this->has_keyword()) { throw exception::NoKeyword(); }
   
   auto zero = this->null_terminator();

   if (*zero+1 == this->data().size()) { this->data().push_back(compression_method); }
   else { this->data()[*zero+1] = compression_method; }
}

bool ZText::has_text() const {
   auto zero = this->null_terminator();

   return zero.has_value() && this->data().size() > *zero+2 || !zero.has_value() && this->data().size() > 0;
}

std::string ZText::text() const {
   auto data = std::vector<std::uint8_t>(&this->data().at(this->text_offset()), &this->data().at(this->data().size()-1)+1);
   auto decompressed = facade::decompress(data);
   return std::string(decompressed.begin(), decompressed.end());
}

void ZText::set_text(std::string text) {
   if (this->has_text())
      this->data().erase(std::next(this->data().begin(), this->text_offset()), this->data().end());

   if (this->data().size() == 0 || (this->has_keyword() && this->data().size() == this->keyword().size()+1)) { this->data().push_back(0); }

   auto vec_data = std::vector<std::uint8_t>(text.begin(), text.end());
   auto compressed = facade::compress(vec_data, 9);

   this->data().insert(this->data().end(), compressed.begin(), compressed.end());
}

template <typename PixelType>
ScanlineBase<PixelType> ScanlineBase<PixelType>::read_line(const std::vector<std::uint8_t> &raw_data, std::size_t offset, std::size_t width) {
   if (offset >= raw_data.size()) { throw exception::OutOfBounds(offset, raw_data.size()); }
   
   auto filter_type = raw_data[offset];
   auto bit_width = PixelType::Bits * width;
   auto byte_width = bit_width / 8 + static_cast<int>(bit_width % 8 != 0);
   if (offset+1+byte_width > raw_data.size()) { throw exception::OutOfBounds(offset+1+byte_width, raw_data.size()); }

   auto sample_width = (width / Span::Samples) + static_cast<int>(width % Span::Samples != 0);
   auto span_ptr = reinterpret_cast<const Span *>(&raw_data[offset+1]);
   auto span_vec = std::vector<Span>(&span_ptr[0], &span_ptr[sample_width]);

   return ScanlineBase<PixelType>(filter_type, span_vec);
}

template <typename PixelType>
std::vector<ScanlineBase<PixelType>> ScanlineBase<PixelType>::from_raw(const Header &header, const std::vector<std::uint8_t> &raw_data)
{
   auto width = header.width();
   auto pixel_size = header.pixel_size();
   auto buffer_size = header.buffer_size();
   if (raw_data.size() != buffer_size) { throw exception::PixelMismatch(); }

   auto bit_width = PixelType::Bits * width;
   auto byte_width = bit_width / 8 + static_cast<int>(bit_width % 8 != 0);
   std::vector<ScanlineBase<PixelType>> result;

   for (std::size_t i=0; i<buffer_size; i+=byte_width+1)
      result.push_back(ScanlineBase<PixelType>::read_line(raw_data, i, width));

   return result;
}

template <typename PixelType>
Pixel ScanlineBase<PixelType>::operator[](std::size_t index) const {
   return this->get_pixel(index);
}

template <typename PixelType>
std::uint8_t ScanlineBase<PixelType>::filter_type() const { return this->_filter_type; }

template <typename PixelType>
void ScanlineBase<PixelType>::set_filter_type(std::uint8_t filter_type) { this->_filter_type = filter_type; }

template <typename PixelType>
std::vector<typename ScanlineBase<PixelType>::Span> ScanlineBase<PixelType>::pixel_data() const { return this->_pixel_data; }

template <typename PixelType>
std::size_t ScanlineBase<PixelType>::pixel_span() const { return this->_pixel_data.size(); }

template <typename PixelType>
std::size_t ScanlineBase<PixelType>::pixel_width() const { return this->pixel_span() * Span::Samples; }

template <typename PixelType>
typename ScanlineBase<PixelType>::Span &ScanlineBase<PixelType>::get_span(std::size_t index) {
   if (index > this->pixel_span()) { throw exception::OutOfBounds(index, this->pixel_span()); }

   return this->_pixel_data[index];
}

template <typename PixelType>
const typename ScanlineBase<PixelType>::Span &ScanlineBase<PixelType>::get_span(std::size_t index) const {
   if (index > this->pixel_span()) { throw exception::OutOfBounds(index, this->pixel_span()); }

   return this->_pixel_data[index];
}

template <typename PixelType>
void ScanlineBase<PixelType>::set_span(const typename ScanlineBase<PixelType>::Span &span, std::size_t index) {
   if (index > this->pixel_span()) { throw exception::OutOfBounds(index, this->pixel_span()); }

   this->_pixel_data[index] = span;
}

template <typename PixelType>
Pixel ScanlineBase<PixelType>::get_pixel(std::size_t index) const {
   if (index > this->pixel_width()) { throw exception::OutOfBounds(index, this->pixel_width()); }

   return this->get_span(index / Span::Samples)[index % Span::Samples];
}

template <typename PixelType>
void ScanlineBase<PixelType>::set_pixel(const Pixel &pixel, std::size_t index)
{
   if (index > this->pixel_width()) { throw exception::OutOfBounds(index, this->pixel_width()); }

   this->get_span(index / Span::Samples).set(pixel, index % Span::Samples);
}

template <typename PixelType>
std::vector<std::uint8_t> ScanlineBase<PixelType>::to_raw() const {
   std::vector<std::uint8_t> result;

   result.push_back(this->filter_type());

   auto raw_pixels = pixels_to_raw<PixelType>(this->_pixel_data);
   result.insert(result.end(), raw_pixels.begin(), raw_pixels.end());

   return result;
}

template <typename PixelType>
ScanlineBase<PixelType> ScanlineBase<PixelType>::reconstruct(std::optional<ScanlineBase<PixelType>> previous) const
{
   if (this->filter_type() == 0) { return *this; }
   if (previous.has_value() && previous->pixel_span() != this->pixel_span()) { throw exception::ScanlineMismatch(); }
   if (this->_pixel_data.size() == 0) { throw exception::NoPixels(); }

   auto result = *this;
   std::size_t pixel_size = sizeof(Span);
   
   for (std::size_t i=0; i<result.pixel_span(); ++i)
   {
      std::uint8_t *curr_ptr = reinterpret_cast<std::uint8_t *>(&result.get_span(i));
      std::uint8_t *left_ptr = ((i == 0) ? nullptr : reinterpret_cast<std::uint8_t *>(&result.get_span(i-1)));
      std::uint8_t *prev_ptr = ((!previous.has_value()) ? nullptr : reinterpret_cast<std::uint8_t *>(&previous->get_span(i)));
      std::uint8_t *prev_left_ptr = ((i == 0 || !previous.has_value()) ? nullptr : reinterpret_cast<std::uint8_t *>(&previous->get_span(i-1)));

      for (std::size_t j=0; j<pixel_size; ++j)
      {
         std::int32_t curr, left, prev, prev_left;

         curr = curr_ptr[j];
         left = ((left_ptr == nullptr) ? 0 : left_ptr[j]);
         prev = ((prev_ptr == nullptr) ? 0 : prev_ptr[j]);
         prev_left = ((prev_left_ptr == nullptr) ? 0 : prev_left_ptr[j]);

         switch (this->filter_type())
         {
         case FilterType::Sub:
         {
            curr_ptr[j] = (curr + left) & 0xFF;
            break;
         }

         case FilterType::Up:
         {
            curr_ptr[j] = (curr + prev) & 0xFF;
            break;
         }

         case FilterType::Average:
         {
            curr_ptr[j] = (curr + (left+prev)/2) & 0xFF;
            break;
         }

         case FilterType::Paeth:
         {
            auto paeth = left + prev - prev_left;
            auto paeth_left = std::abs(paeth - left);
            auto paeth_prev = std::abs(paeth - prev);
            auto paeth_prev_left = std::abs(paeth - prev_left);

            if (paeth_left <= paeth_prev && paeth_left <= paeth_prev_left)
               curr_ptr[j] = (curr + left) & 0xFF;
            else if (paeth_prev <= paeth_prev_left)
               curr_ptr[j] = (curr + prev) & 0xFF;
            else
               curr_ptr[j] = (curr + prev_left) & 0xFF;

            break;
         }

         default:
            throw exception::InvalidFilterType(this->filter_type());
         }
      }
   }

   result.set_filter_type(FilterType::None);

   return result;
}

template <typename PixelType>
ScanlineBase<PixelType> ScanlineBase<PixelType>::filter(std::optional<ScanlineBase<PixelType>> previous) const {
   std::pair<std::size_t, ScanlineBase<PixelType>> best_scanline;

   for (std::uint8_t i=0; i<5; ++i)
   {
      auto filtered = this->filter(i, previous);
      auto raw = pixels_to_raw<PixelType>(filtered.pixel_data());
      auto signed_raw = std::vector<std::int8_t>(raw.begin(), raw.end());

      std::intptr_t sum = 0;

      for (auto i8 : signed_raw)
         sum += i8;

      std::size_t abs = std::abs(sum);

      if (i == 0 || abs < best_scanline.first)
         best_scanline = std::make_pair(abs, filtered);
   }

   return best_scanline.second;
}

template <typename PixelType>
ScanlineBase<PixelType> ScanlineBase<PixelType>::filter(std::uint8_t filter_type, std::optional<ScanlineBase<PixelType>> previous) const
{
   if (this->filter_type() != 0) { throw exception::AlreadyFiltered(); }
   if (previous.has_value() && previous->_pixel_data.size() != this->_pixel_data.size()) { throw exception::ScanlineMismatch(); }
   if (this->_pixel_data.size() == 0) { throw exception::NoPixels(); }
   if (filter_type == FilterType::None) { return *this; }

   auto result = *this;
   std::size_t pixel_size = sizeof(Span);
   
   for (std::size_t i=0; i<result.pixel_span(); ++i)
   {
      std::uint8_t *res_ptr = reinterpret_cast<std::uint8_t *>(&result.get_span(i));
      const std::uint8_t *curr_ptr = reinterpret_cast<const std::uint8_t *>(&this->get_span(i));
      const std::uint8_t *left_ptr = ((i == 0) ? nullptr : reinterpret_cast<const std::uint8_t *>(&this->get_span(i-1)));
      const std::uint8_t *prev_ptr = ((!previous.has_value()) ? nullptr : reinterpret_cast<const std::uint8_t *>(&previous->get_span(i)));
      const std::uint8_t *prev_left_ptr = ((i == 0 || !previous.has_value()) ? nullptr : reinterpret_cast<const std::uint8_t *>(&previous->get_span(i-1)));

      for (std::size_t j=0; j<pixel_size; ++j)
      {
         std::int32_t curr, left, prev, prev_left;

         curr = curr_ptr[j];
         left = ((left_ptr == nullptr) ? 0 : left_ptr[j]);
         prev = ((prev_ptr == nullptr) ? 0 : prev_ptr[j]);
         prev_left = ((prev_left_ptr == nullptr) ? 0 : prev_left_ptr[j]);

         switch (filter_type)
         {
         case FilterType::Sub:
         {
            res_ptr[j] = (curr - left) & 0xFF;
            //if (curr_ptr[j] > (curr - left)) { std::cout << "Overflow at byte " << i << ": " << curr << ", " << left << ", " << static_cast<int>(curr_ptr[j]) << std::endl; throw std::runtime_error("Sub filter overflowed"); }
            break;
         }

         case FilterType::Up:
         {
            res_ptr[j] = (curr - prev) & 0xFF;
            break;
         }

         case FilterType::Average:
         {
            res_ptr[j] = (curr - (left+prev)/2) & 0xFF;
            break;
         }

         case FilterType::Paeth:
         {
            auto paeth = left + prev - prev_left;
            auto paeth_left = std::abs(paeth - left);
            auto paeth_prev = std::abs(paeth - prev);
            auto paeth_prev_left = std::abs(paeth - prev_left);

            if (paeth_left <= paeth_prev && paeth_left <= paeth_prev_left)
               res_ptr[j] = (curr - left) & 0xFF;
            else if (paeth_prev <= paeth_prev_left)
               res_ptr[j] = (curr - prev) & 0xFF;
            else
               res_ptr[j] = (curr - prev_left) & 0xFF;

            break;
         }

         default:
            throw exception::InvalidFilterType(filter_type);
         }
      }
   }

   result.set_filter_type(filter_type);

   return result;
}

Pixel Scanline::operator[](std::size_t index) const { return this->get_pixel(index); }

std::uint8_t Scanline::filter_type() const {
   return std::visit([](auto &p) -> std::uint8_t { return p.filter_type(); }, *this);
}

void Scanline::set_filter_type(std::uint8_t filter_type) {
   std::visit([filter_type](auto &p) { p.set_filter_type(filter_type); }, *this);
}

std::size_t Scanline::pixel_span() const {
   return std::visit([](auto &p) -> std::size_t { return p.pixel_span(); }, *this);
}

std::size_t Scanline::pixel_width() const {
   return std::visit([](auto &p) -> std::size_t { return p.pixel_width(); }, *this);
}

Pixel Scanline::get_pixel(std::size_t index) const {
   return std::visit([index](auto &p) -> Pixel { return p.get_pixel(index); }, *this);
}

void Scanline::set_pixel(const Pixel &pixel, std::size_t index) {
   std::visit([&pixel, &index](auto &p) { p.set_pixel(pixel, index); }, *this);
}

std::vector<std::uint8_t> Scanline::to_raw() const {
   return std::visit([](auto &p) -> std::vector<std::uint8_t> { return p.to_raw(); }, *this);
}
            
Scanline &Image::operator[](std::size_t index) {
   return this->scanline(index);
}

const Scanline &Image::operator[](std::size_t index) const {
   return this->scanline(index);
}

bool Image::has_trailing_data() const { return this->trailing_data.has_value(); }

std::vector<std::uint8_t> &Image::get_trailing_data() { return *this->trailing_data; }

const std::vector<std::uint8_t> &Image::get_trailing_data() const { return *this->trailing_data; }

void Image::set_trailing_data(const std::vector<std::uint8_t> &data) { this->trailing_data = data; }

void Image::clear_trailing_data() { this->trailing_data = std::nullopt; }

void Image::parse(const void *ptr, std::size_t size, bool validate) {
   if (size < 8) { throw exception::InsufficientSize(size, 8); }
   if (std::memcmp(ptr, this->Signature, 8) != 0) { throw exception::BadPNGSignature(); }
   
   this->chunk_map.clear();
   this->trailing_data = std::nullopt;
   // this->image_data = std::nullopt;
   
   std::size_t offset = 8;
   ChunkPtr current_chunk;

   do
   {
      current_chunk = ChunkPtr::parse(ptr, size, offset);
      offset += current_chunk.chunk_size();
      //std::cout << "Parsed chunk: " << current_chunk.tag().to_string() << std::endl;
      //std::cout << "Chunk size: " << current_chunk.length() << std::endl;
      //std::cout << "Chunk CRC: " << std::showbase << std::hex << current_chunk.crc() << std::dec << std::endl;
      //std::cout << std::endl;
      
      auto chunk_vec = current_chunk.to_chunk_vec();
      if (validate && !current_chunk.validate()) { throw exception::BadCRC(current_chunk.crc(), chunk_vec.crc()); }
      
      this->chunk_map[chunk_vec.tag().to_string()].push_back(chunk_vec);
   } while (current_chunk.tag().to_string() != "IEND");

   if (offset < size) {
      auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);
      this->trailing_data = std::vector<std::uint8_t>(&u8_ptr[offset], &u8_ptr[size]);
   }
}

void Image::parse(const std::vector<std::uint8_t> &data, bool validate) {
   this->parse(data.data(), data.size(), validate);
}

void Image::parse(const std::string &filename, bool validate) {
   std::basic_ifstream<std::uint8_t> fp(filename, std::ios::binary);
   if (!fp.is_open()) { throw exception::OpenFileFailure(filename); }

   auto vec_data = std::vector<std::uint8_t>();
   vec_data.insert(vec_data.begin(),
                   std::istreambuf_iterator<std::uint8_t>(fp),
                   std::istreambuf_iterator<std::uint8_t>());

   fp.close();
   this->parse(vec_data, validate);
}

void Image::load() {
   this->decompress();
   this->reconstruct();
}

Scanline &Image::scanline(std::size_t index) {
   if (!this->image_data.has_value()) { throw exception::NoImageData(); }
   if (index > this->image_data->size()) { throw exception::OutOfBounds(index, this->image_data->size()); }

   return this->image_data->operator[](index);
}

const Scanline &Image::scanline(std::size_t index) const {
   if (!this->image_data.has_value()) { throw exception::NoImageData(); }
   if (index > this->image_data->size()) { throw exception::OutOfBounds(index, this->image_data->size()); }

   return this->image_data->operator[](index);
}
   
bool Image::has_header() const {
   return this->chunk_map.find("IHDR") != this->chunk_map.end();
}

Header &Image::header() {
   if (!this->has_header()) { throw exception::NoHeaderChunk(); }

   return this->chunk_map["IHDR"][0].upcast<Header>();
}

const Header &Image::header() const {
   if (!this->has_header()) { throw exception::NoHeaderChunk(); }

   return this->chunk_map.at("IHDR")[0].upcast<Header>();
}

Header &Image::new_header() {
   this->chunk_map["IHDR"].clear();
   this->chunk_map["IHDR"].push_back(Header().as_chunk_vec());

   return this->header();
}

bool Image::has_image_data() const {
   return this->chunk_map.find("IDAT") != this->chunk_map.end();
}

bool Image::is_loaded() const {
   return this->image_data.has_value();
}

void Image::decompress() {
   if (!this->has_image_data()) { throw exception::NoImageDataChunks(); }
   
   std::vector<std::uint8_t> combined;

   for (auto &chunk : this->chunk_map.at("IDAT"))
      combined.insert(combined.end(), chunk.data().begin(), chunk.data().end());

   auto decompressed = facade::decompress(combined);

   switch (this->header().pixel_type())
   {
   case PixelEnum::GRAYSCALE_PIXEL_1BIT:
   {
      auto scanlines = GrayscaleScanline1Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::GRAYSCALE_PIXEL_2BIT:
   {
      auto scanlines = GrayscaleScanline2Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::GRAYSCALE_PIXEL_4BIT:
   {
      auto scanlines = GrayscaleScanline4Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::GRAYSCALE_PIXEL_8BIT:
   {
      auto scanlines = GrayscaleScanline8Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::GRAYSCALE_PIXEL_16BIT:
   {
      auto scanlines = GrayscaleScanline16Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::TRUE_COLOR_PIXEL_8BIT:
   {
      auto scanlines = TrueColorScanline8Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::TRUE_COLOR_PIXEL_16BIT:
   {
      auto scanlines = TrueColorScanline16Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::PALETTE_PIXEL_1BIT:
   {
      auto scanlines = PaletteScanline1Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::PALETTE_PIXEL_2BIT:
   {
      auto scanlines = PaletteScanline2Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::PALETTE_PIXEL_4BIT:
   {
      auto scanlines = PaletteScanline4Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::PALETTE_PIXEL_8BIT:
   {
      auto scanlines = PaletteScanline8Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::ALPHA_GRAYSCALE_PIXEL_8BIT:
   {
      auto scanlines = AlphaGrayscaleScanline8Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::ALPHA_GRAYSCALE_PIXEL_16BIT:
   {
      auto scanlines = AlphaGrayscaleScanline16Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_8BIT:
   {
      auto scanlines = AlphaTrueColorScanline8Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_16BIT:
   {
      auto scanlines = AlphaTrueColorScanline16Bit::from_raw(this->header(), decompressed);
      this->image_data = std::vector<Scanline>(scanlines.begin(), scanlines.end());
      break;
   }
   }
}

void Image::compress(std::optional<std::size_t> chunk_size, int level) {
   if (!this->image_data.has_value()) { throw exception::NoImageData(); }

   std::vector<std::uint8_t> combined;

   for (auto &scanline : *this->image_data) {
      auto raw = scanline.to_raw();
      combined.insert(combined.end(), raw.begin(), raw.end());
   }

   auto compressed = facade::compress(combined.data(), combined.size(), level);
   std::vector<ChunkVec> idat_chunks;

   if (!chunk_size.has_value())
   {
      idat_chunks.push_back(ChunkVec(std::string("IDAT"), &compressed[0], compressed.size()));
   }
   else
   {
      for (std::size_t i=0; i<compressed.size(); i+=*chunk_size)
      {
         auto left = compressed.size() - i;
         idat_chunks.push_back(ChunkVec(std::string("IDAT"), &compressed[i], (left > *chunk_size) ? *chunk_size : left));
      }
   }

   this->chunk_map["IDAT"] = idat_chunks;
}

void Image::reconstruct() {
   if (!this->image_data.has_value()) { throw exception::NoImageData(); }

   auto &image_data = *this->image_data;
   
   for (std::size_t i=0; i<this->image_data->size(); ++i)
   {
      switch (this->header().pixel_type())
      {
      case PixelEnum::GRAYSCALE_PIXEL_1BIT:
      {
         std::optional<GrayscaleScanline1Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline1Bit>(std::nullopt)
            : std::get<GrayscaleScanline1Bit>(image_data[i-1]);
         image_data[i] = std::get<GrayscaleScanline1Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_2BIT:
      {
         std::optional<GrayscaleScanline2Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline2Bit>(std::nullopt)
            : std::get<GrayscaleScanline2Bit>(image_data[i-1]);
         image_data[i] = std::get<GrayscaleScanline2Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_4BIT:
      {
         std::optional<GrayscaleScanline4Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline4Bit>(std::nullopt)
            : std::get<GrayscaleScanline4Bit>(image_data[i-1]);
         image_data[i] = std::get<GrayscaleScanline4Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_8BIT:
      {
         std::optional<GrayscaleScanline8Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline8Bit>(std::nullopt)
            : std::get<GrayscaleScanline8Bit>(image_data[i-1]);
         image_data[i] = std::get<GrayscaleScanline8Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_16BIT:
      {
         std::optional<GrayscaleScanline16Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline16Bit>(std::nullopt)
            : std::get<GrayscaleScanline16Bit>(image_data[i-1]);
         image_data[i] = std::get<GrayscaleScanline16Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::TRUE_COLOR_PIXEL_8BIT:
      {
         std::optional<TrueColorScanline8Bit> previous = (i == 0)
            ? std::optional<TrueColorScanline8Bit>(std::nullopt)
            : std::get<TrueColorScanline8Bit>(image_data[i-1]);
         image_data[i] = std::get<TrueColorScanline8Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::TRUE_COLOR_PIXEL_16BIT:
      {
         std::optional<TrueColorScanline16Bit> previous = (i == 0)
            ? std::optional<TrueColorScanline16Bit>(std::nullopt)
            : std::get<TrueColorScanline16Bit>(image_data[i-1]);
         image_data[i] = std::get<TrueColorScanline16Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_1BIT:
      {
         std::optional<PaletteScanline1Bit> previous = (i == 0)
            ? std::optional<PaletteScanline1Bit>(std::nullopt)
            : std::get<PaletteScanline1Bit>(image_data[i-1]);
         image_data[i] = std::get<PaletteScanline1Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_2BIT:
      {
         std::optional<PaletteScanline2Bit> previous = (i == 0)
            ? std::optional<PaletteScanline2Bit>(std::nullopt)
            : std::get<PaletteScanline2Bit>(image_data[i-1]);
         image_data[i] = std::get<PaletteScanline2Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_4BIT:
      {
         std::optional<PaletteScanline4Bit> previous = (i == 0)
            ? std::optional<PaletteScanline4Bit>(std::nullopt)
            : std::get<PaletteScanline4Bit>(image_data[i-1]);
         image_data[i] = std::get<PaletteScanline4Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_8BIT:
      {
         std::optional<PaletteScanline8Bit> previous = (i == 0)
            ? std::optional<PaletteScanline8Bit>(std::nullopt)
            : std::get<PaletteScanline8Bit>(image_data[i-1]);
         image_data[i] = std::get<PaletteScanline8Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::ALPHA_GRAYSCALE_PIXEL_8BIT:
      {
         std::optional<AlphaGrayscaleScanline8Bit> previous = (i == 0)
            ? std::optional<AlphaGrayscaleScanline8Bit>(std::nullopt)
            : std::get<AlphaGrayscaleScanline8Bit>(image_data[i-1]);
         image_data[i] = std::get<AlphaGrayscaleScanline8Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::ALPHA_GRAYSCALE_PIXEL_16BIT:
      {
         std::optional<AlphaGrayscaleScanline16Bit> previous = (i == 0)
            ? std::optional<AlphaGrayscaleScanline16Bit>(std::nullopt)
            : std::get<AlphaGrayscaleScanline16Bit>(image_data[i-1]);
         image_data[i] = std::get<AlphaGrayscaleScanline16Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_8BIT:
      {
         std::optional<AlphaTrueColorScanline8Bit> previous = (i == 0)
            ? std::optional<AlphaTrueColorScanline8Bit>(std::nullopt)
            : std::get<AlphaTrueColorScanline8Bit>(image_data[i-1]);
         image_data[i] = std::get<AlphaTrueColorScanline8Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_16BIT:
      {
         std::optional<AlphaTrueColorScanline16Bit> previous = (i == 0)
            ? std::optional<AlphaTrueColorScanline16Bit>(std::nullopt)
            : std::get<AlphaTrueColorScanline16Bit>(image_data[i-1]);
         image_data[i] = std::get<AlphaTrueColorScanline16Bit>(image_data[i]).reconstruct(previous);
         break;
      }
      }
   }
}

void Image::filter() {
   if (!this->image_data.has_value()) { throw exception::NoImageData(); }

   auto &current_data = *this->image_data;
   auto new_data = *this->image_data;

   for (std::size_t i=0; i<current_data.size(); ++i)
   {
      switch (this->header().pixel_type())
      {
      case PixelEnum::GRAYSCALE_PIXEL_1BIT:
      {
         std::optional<GrayscaleScanline1Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline1Bit>(std::nullopt)
            : std::get<GrayscaleScanline1Bit>(current_data[i-1]);
         new_data[i] = std::get<GrayscaleScanline1Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_2BIT:
      {
         std::optional<GrayscaleScanline2Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline2Bit>(std::nullopt)
            : std::get<GrayscaleScanline2Bit>(current_data[i-1]);
         new_data[i] = std::get<GrayscaleScanline2Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_4BIT:
      {
         std::optional<GrayscaleScanline4Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline4Bit>(std::nullopt)
            : std::get<GrayscaleScanline4Bit>(current_data[i-1]);
         new_data[i] = std::get<GrayscaleScanline4Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_8BIT:
      {
         std::optional<GrayscaleScanline8Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline8Bit>(std::nullopt)
            : std::get<GrayscaleScanline8Bit>(current_data[i-1]);
         new_data[i] = std::get<GrayscaleScanline8Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::GRAYSCALE_PIXEL_16BIT:
      {
         std::optional<GrayscaleScanline16Bit> previous = (i == 0)
            ? std::optional<GrayscaleScanline16Bit>(std::nullopt)
            : std::get<GrayscaleScanline16Bit>(current_data[i-1]);
         new_data[i] = std::get<GrayscaleScanline16Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::TRUE_COLOR_PIXEL_8BIT:
      {
         std::optional<TrueColorScanline8Bit> previous = (i == 0)
            ? std::optional<TrueColorScanline8Bit>(std::nullopt)
            : std::get<TrueColorScanline8Bit>(current_data[i-1]);
         new_data[i] = std::get<TrueColorScanline8Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::TRUE_COLOR_PIXEL_16BIT:
      {
         std::optional<TrueColorScanline16Bit> previous = (i == 0)
            ? std::optional<TrueColorScanline16Bit>(std::nullopt)
            : std::get<TrueColorScanline16Bit>(current_data[i-1]);
         new_data[i] = std::get<TrueColorScanline16Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_1BIT:
      {
         std::optional<PaletteScanline1Bit> previous = (i == 0)
            ? std::optional<PaletteScanline1Bit>(std::nullopt)
            : std::get<PaletteScanline1Bit>(current_data[i-1]);
         new_data[i] = std::get<PaletteScanline1Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_2BIT:
      {
         std::optional<PaletteScanline2Bit> previous = (i == 0)
            ? std::optional<PaletteScanline2Bit>(std::nullopt)
            : std::get<PaletteScanline2Bit>(current_data[i-1]);
         new_data[i] = std::get<PaletteScanline2Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_4BIT:
      {
         std::optional<PaletteScanline4Bit> previous = (i == 0)
            ? std::optional<PaletteScanline4Bit>(std::nullopt)
            : std::get<PaletteScanline4Bit>(current_data[i-1]);
         new_data[i] = std::get<PaletteScanline4Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::PALETTE_PIXEL_8BIT:
      {
         std::optional<PaletteScanline8Bit> previous = (i == 0)
            ? std::optional<PaletteScanline8Bit>(std::nullopt)
            : std::get<PaletteScanline8Bit>(current_data[i-1]);
         new_data[i] = std::get<PaletteScanline8Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::ALPHA_GRAYSCALE_PIXEL_8BIT:
      {
         std::optional<AlphaGrayscaleScanline8Bit> previous = (i == 0)
            ? std::optional<AlphaGrayscaleScanline8Bit>(std::nullopt)
            : std::get<AlphaGrayscaleScanline8Bit>(current_data[i-1]);
         new_data[i] = std::get<AlphaGrayscaleScanline8Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::ALPHA_GRAYSCALE_PIXEL_16BIT:
      {
         std::optional<AlphaGrayscaleScanline16Bit> previous = (i == 0)
            ? std::optional<AlphaGrayscaleScanline16Bit>(std::nullopt)
            : std::get<AlphaGrayscaleScanline16Bit>(current_data[i-1]);
         new_data[i] = std::get<AlphaGrayscaleScanline16Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_8BIT:
      {
         std::cout << "Filtering row " << i << std::endl;
         std::optional<AlphaTrueColorScanline8Bit> previous = (i == 0)
            ? std::optional<AlphaTrueColorScanline8Bit>(std::nullopt)
            : std::get<AlphaTrueColorScanline8Bit>(current_data[i-1]);
         new_data[i] = std::get<AlphaTrueColorScanline8Bit>(new_data[i]).filter(previous);
         break;
      }
      case PixelEnum::ALPHA_TRUE_COLOR_PIXEL_16BIT:
      {
         std::optional<AlphaTrueColorScanline16Bit> previous = (i == 0)
            ? std::optional<AlphaTrueColorScanline16Bit>(std::nullopt)
            : std::get<AlphaTrueColorScanline16Bit>(current_data[i-1]);
         new_data[i] = std::get<AlphaTrueColorScanline16Bit>(new_data[i]).filter(previous);
         break;
      }
      }
   }

   this->image_data = new_data;
}

std::vector<std::uint8_t> Image::to_file() const
{
   std::vector<std::string> chunks = {
      /* critical chunks */
      "IHDR", "gAMA", "PLTE", "IDAT",

      /* ancillary chunks */
      "tRNS", "cHRM", "iCCP", "sBIT", "sRGB", "cICP",
      "tEXt", "zTXt", "iTXt", "bKGD", "hIST", "pHYs", "sPLT",
      "eXIf", "tIME", "acTL", "fcTL", "fdAT",
   };

   /* make sure nonstandard chunks get written too */
   for (auto pair : this->chunk_map)
      if (pair.first != "IEND" && std::find(chunks.begin(), chunks.end(), pair.first) == chunks.end())
         chunks.push_back(pair.first);

   chunks.push_back("IEND");

   std::vector<std::uint8_t> file_data;
   file_data.insert(file_data.end(), &this->Signature[0], &this->Signature[8]);

   for (auto chunk_label : chunks)
   {
      if (this->chunk_map.find(chunk_label) == this->chunk_map.end()) { continue; }

      for (auto &chunk : this->chunk_map.at(chunk_label))
      {
         auto ptr = chunk.to_chunk_ptr();
         auto &data = ptr.first;
         file_data.insert(file_data.end(), data.begin(), data.end());
      }
   }

   if (this->chunk_map.find("IEND") == this->chunk_map.end())
   {
      auto end = End().to_chunk_ptr();
      file_data.insert(file_data.end(), end.first.begin(), end.first.end());
   }

   if (this->trailing_data.has_value())
      file_data.insert(file_data.end(), this->trailing_data->begin(), this->trailing_data->end());

   return file_data;
}

void Image::save(const std::string &filename) const
{
   auto data = this->to_file();
   
   std::basic_ofstream<std::uint8_t> outfile(filename, std::ios::binary);
   if (!outfile) { throw exception::OpenFileFailure(filename); }

   outfile.write(data.data(), data.size());
   outfile.close();
}

bool Image::has_text() const {
   return this->chunk_map.find("tEXt") != this->chunk_map.end();
}

Text &Image::add_text(const std::string &keyword, const std::string &text) {
   this->chunk_map["tEXt"].push_back(Text(keyword, text).as_chunk_vec());

   return this->chunk_map["tEXt"].back().upcast<Text>();
}

void Image::remove_text(const Text &text) {
   for (auto chunk=this->chunk_map["tEXt"].begin(); chunk!=this->chunk_map["tEXt"].end(); ++chunk)
   {
      if (*chunk == text)
      {
         this->chunk_map["tEXt"].erase(chunk);
         return;
      }
   }

   throw exception::TextNotFound();
}

void Image::remove_text(const std::string &keyword, const std::string &text) {
   this->remove_text(Text(keyword, text));
}

std::vector<Text> Image::get_text(const std::string &keyword) const {
   std::vector<Text> result;
   
   for (auto &text : this->chunk_map.at("tEXt"))
   {
      auto &upcast = text.upcast<Text>();

      if (upcast.keyword() == keyword)
         result.push_back(upcast);
   }

   return result;
}

bool Image::has_ztext() const {
   return this->chunk_map.find("zTXt") != this->chunk_map.end();
}

ZText &Image::add_ztext(const std::string &keyword, const std::string &text) {
   this->chunk_map["zTXt"].push_back(ZText(keyword, text).as_chunk_vec());

   return this->chunk_map["zTXt"].back().upcast<ZText>();
}

void Image::remove_ztext(const ZText &text) {
   for (auto chunk=this->chunk_map["zTXt"].begin(); chunk!=this->chunk_map["zTXt"].end(); ++chunk)
   {
      if (*chunk == text)
      {
         this->chunk_map["zTXt"].erase(chunk);
         return;
      }
   }

   throw exception::TextNotFound();
}

void Image::remove_ztext(const std::string &keyword, const std::string &text) {
   this->remove_ztext(ZText(keyword, text));
}

std::vector<ZText> Image::get_ztext(const std::string &keyword) const {
   std::vector<ZText> result;
   
   for (auto &text : this->chunk_map.at("zTXt"))
   {
      auto &upcast = text.upcast<ZText>();

      if (upcast.keyword() == keyword)
         result.push_back(upcast);
   }

   return result;
}
