#include <facade.hpp>

using namespace facade;

png::Text &PNGPayload::add_text_payload(const std::string &keyword, const void *ptr, std::size_t size) {
   return this->add_text(keyword, facade::base64_encode(ptr, size));
}

png::Text &PNGPayload::add_text_payload(const std::string &keyword, const std::vector<std::uint8_t> &data) {
   return this->add_text_payload(keyword, data.data(), data.size());
}

void PNGPayload::remove_text_payload(const png::Text &text) {
   this->remove_text(text);
}

std::vector<png::Text> PNGPayload::get_text_payloads(const std::string &keyword) const {
   auto potential_payloads = this->get_text(keyword);
   std::vector<png::Text> result;

   for (auto &text : potential_payloads)
   {
      if (!facade::is_base64_string(text.text())) { throw exception::InvalidBase64String(text.text()); }
      result.push_back(text);
   }

   return result;
}

std::vector<std::vector<std::uint8_t>> PNGPayload::extract_text_payloads(const std::string &keyword) const {
   auto payloads = this->get_text_payloads(keyword);
   std::vector<std::vector<std::uint8_t>> result;

   for (auto &payload : payloads)
      result.push_back(facade::base64_decode(payload.text()));

   return result;
}

png::ZText &PNGPayload::add_ztext_payload(const std::string &keyword, const void *ptr, std::size_t size) {
   return this->add_ztext(keyword, facade::base64_encode(ptr, size));
}

png::ZText &PNGPayload::add_ztext_payload(const std::string &keyword, const std::vector<std::uint8_t> &data) {
   return this->add_ztext_payload(keyword, data.data(), data.size());
}

void PNGPayload::remove_ztext_payload(const png::ZText &ztext) {
   this->remove_ztext(ztext);
}

std::vector<png::ZText> PNGPayload::get_ztext_payloads(const std::string &keyword) const {
   auto potential_payloads = this->get_ztext(keyword);
   std::vector<png::ZText> result;

   for (auto &ztext : potential_payloads)
   {
      if (!facade::is_base64_string(ztext.text())) { throw exception::InvalidBase64String(ztext.text()); }
      result.push_back(ztext);
   }

   return result;
}

std::vector<std::vector<std::uint8_t>> PNGPayload::extract_ztext_payloads(const std::string &keyword) const {
   auto payloads = this->get_ztext_payloads(keyword);
   std::vector<std::vector<std::uint8_t>> result;

   for (auto &payload : payloads)
      result.push_back(facade::base64_decode(payload.text()));

   return result;
}

std::vector<std::uint8_t> PNGPayload::read_stego_data(std::size_t bit_offset, std::size_t size) const {
   if (!this->is_loaded()) { throw exception::NoImageData(); }
   if (bit_offset % 4 != 0) { throw exception::InvalidBitOffset(bit_offset); }

   auto &header = this->header();
   auto max_size = (header.width() * header.height() * 3 * 4);
   auto checked_size = bit_offset + size * 8;
   if (checked_size > max_size) { throw exception::OutOfBounds(checked_size, max_size); }

   std::vector<std::uint8_t> result;

   for (auto bits=bit_offset; bits<checked_size; bits+=4)
   {
      auto pixel_index = bits/12;
      auto color_index = (bits%12)/4;
      auto byte_index = (bits-bit_offset) / 8;
      auto bit_index = (bits-bit_offset) % 8;
      auto pixel_y = pixel_index / header.width();
      auto pixel_x = pixel_index % header.width();
      auto pixel = (*this)[pixel_y][pixel_x];

      std::uint8_t lsb;

      switch (color_index)
      {
      case 0: // red
      {
         if (png::TrueColorPixel8Bit *tc = std::get_if<png::TrueColorPixel8Bit>(&pixel))
            lsb = *tc->red() & 0xF;
         else if (png::AlphaTrueColorPixel8Bit *atc = std::get_if<png::AlphaTrueColorPixel8Bit>(&pixel))
            lsb = *atc->red() & 0xF;
         else
            throw exception::PixelMismatch();

         break;
      }
      case 1: // green
      {
         if (png::TrueColorPixel8Bit *tc = std::get_if<png::TrueColorPixel8Bit>(&pixel))
            lsb = *tc->green() & 0xF;
         else if (png::AlphaTrueColorPixel8Bit *atc = std::get_if<png::AlphaTrueColorPixel8Bit>(&pixel))
            lsb = *atc->green() & 0xF;
         else
            throw exception::PixelMismatch();

         break;
      }
      case 2: // blue
      {
         if (png::TrueColorPixel8Bit *tc = std::get_if<png::TrueColorPixel8Bit>(&pixel))
            lsb = *tc->blue() & 0xF;
         else if (png::AlphaTrueColorPixel8Bit *atc = std::get_if<png::AlphaTrueColorPixel8Bit>(&pixel))
            lsb = *atc->blue() & 0xF;
         else
            throw exception::PixelMismatch();

         break;
      }
      }

      if (bit_index == 0) { result.push_back(lsb); }
      else { result[byte_index] |= lsb << bit_index; }
   }

   return result;
}

void PNGPayload::write_stego_data(const void *ptr, std::size_t size, std::size_t bit_offset)
{
   if (!this->is_loaded()) { throw exception::NoImageData(); }
   if (bit_offset % 4 != 0) { throw exception::InvalidBitOffset(bit_offset); }

   auto &header = this->header();
   auto max_size = (header.width() * header.height() * 3 * 4);
   auto checked_size = bit_offset + size * 8;
   if (checked_size > max_size) { throw exception::OutOfBounds(checked_size, max_size); }

   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);

   for (std::size_t bits=bit_offset; bits<checked_size; bits+=4)
   {
      auto pixel_index = bits/12;
      auto color_index = (bits%12)/4;
      auto byte_index = (bits-bit_offset) / 8;
      auto bit_index = (bits-bit_offset) % 8;
      auto pixel_y = pixel_index / header.width();
      auto pixel_x = pixel_index % header.width();
      auto pixel = (*this)[pixel_y][pixel_x];

      switch (color_index)
      {
      case 0: // red
      {
         if (png::TrueColorPixel8Bit *tc = std::get_if<png::TrueColorPixel8Bit>(&pixel))
            tc->red() = ((*tc->red() & 0xF0) | ((u8_ptr[byte_index] >> bit_index) & 0xF));
         else if (png::AlphaTrueColorPixel8Bit *atc = std::get_if<png::AlphaTrueColorPixel8Bit>(&pixel))
            atc->red() = ((*atc->red() & 0xF0) | ((u8_ptr[byte_index] >> bit_index) & 0xF));
         else
            throw exception::PixelMismatch();
      }
      case 1: // green
      {
         if (png::TrueColorPixel8Bit *tc = std::get_if<png::TrueColorPixel8Bit>(&pixel))
            tc->green() = ((*tc->green() & 0xF0) | ((u8_ptr[byte_index] >> bit_index) & 0xF));
         else if (png::AlphaTrueColorPixel8Bit *atc = std::get_if<png::AlphaTrueColorPixel8Bit>(&pixel))
            atc->green() = ((*atc->green() & 0xF0) | ((u8_ptr[byte_index] >> bit_index) & 0xF));
         else
            throw exception::PixelMismatch();
      }
      case 2: // blue
      {
         if (png::TrueColorPixel8Bit *tc = std::get_if<png::TrueColorPixel8Bit>(&pixel))
            tc->blue() = ((*tc->blue() & 0xF0) | ((u8_ptr[byte_index] >> bit_index) & 0xF));
         else if (png::AlphaTrueColorPixel8Bit *atc = std::get_if<png::AlphaTrueColorPixel8Bit>(&pixel))
            atc->blue() = ((*atc->blue() & 0xF0) | ((u8_ptr[byte_index] >> bit_index) & 0xF));
         else
            throw exception::PixelMismatch();
      }
      }

      (*this)[pixel_y].set_pixel(pixel, pixel_x);
   }
}

void PNGPayload::write_stego_data(const std::vector<std::uint8_t> &data, std::size_t bit_offset) {
   this->write_stego_data(data.data(), data.size(), bit_offset);
}

bool PNGPayload::has_stego_payload() const {
   if (!this->is_loaded()) { throw exception::NoImageData(); }

   auto &header = this->header();

   auto stego_header = this->read_stego_data(0, 3);
   auto header_expected = std::vector<std::uint8_t>({ 'F', 'C', 'D' });
   if (stego_header != header_expected) { return false; }

   auto data_size = this->read_stego_data(3*8, 4);
   auto size_val = *reinterpret_cast<std::uint32_t *>(data_size.data());
   auto max_val = (header.width() * header.height() * 3 * 4);
   if (7*8+size_val*8+3*8 > max_val) { return false; }

   auto stego_footer = this->read_stego_data(7*8+size_val*8, 3);
   auto footer_expected = std::vector<std::uint8_t>({ 'D', 'C', 'F' });
   if (stego_footer != footer_expected) { return false; }

   return true;
}

PNGPayload PNGPayload::create_stego_payload(const void *ptr, std::size_t size) const {
   auto result = *this;
   auto &header = result.header();
   auto pixel_type = header.pixel_type();

   if (pixel_type != png::PixelEnum::TRUE_COLOR_PIXEL_8BIT && pixel_type != png::PixelEnum::ALPHA_TRUE_COLOR_PIXEL_8BIT)
      throw exception::UnsupportedPixelType(pixel_type);

   auto compressed = facade::compress(ptr, size, 9);
   auto stego_header = "FCD";
   auto u32_size = static_cast<std::uint32_t>(compressed.size());
   auto u8_size_ptr = reinterpret_cast<std::uint8_t *>(&u32_size);
   auto stego_footer = "DCF";

   std::vector<std::uint8_t> payload;
   payload.insert(payload.end(), &stego_header[0], &stego_header[3]);
   payload.insert(payload.end(), &u8_size_ptr[0], &u8_size_ptr[4]);
   payload.insert(payload.end(), compressed.begin(), compressed.end());
   payload.insert(payload.end(), &stego_footer[0], &stego_footer[3]);

   auto max_storage = (header.width() * header.height() * 3 * 4) / 8;
   if (payload.size() > max_storage) { throw exception::ImageTooSmall(max_storage, payload.size()); }

   result.load();
   //std::cout << "Encoding" << std::endl;
   result.write_stego_data(payload, 0);
   //std::cout << "Filtering" << std::endl;
   result.filter();
   //std::cout << "Compressing" << std::endl;
   result.compress();

   return result;
}

PNGPayload PNGPayload::create_stego_payload(const std::vector<std::uint8_t> &data) const {
   return this->create_stego_payload(data.data(), data.size());
}

std::vector<std::uint8_t> PNGPayload::extract_stego_payload() const {
   if (!this->is_loaded()) { throw exception::NoImageData(); }
   if (!this->has_stego_payload()) { throw exception::NoStegoData(); }

   auto data_size = this->read_stego_data(3*8, 4);
   auto size_val = *reinterpret_cast<std::uint32_t *>(data_size.data());

   return facade::decompress(this->read_stego_data(7*8, size_val));
}
