#ifndef __FACADE_PAYLOAD_HPP
#define __FACADE_PAYLOAD_HPP

#include <facade/png.hpp>

namespace facade
{
   EXPORT class PNGPayload : public png::Image
   {
   public:
      PNGPayload() : png::Image() {}
      PNGPayload(const std::string &filename) : png::Image(filename) {}
      PNGPayload(const PNGPayload &other) : png::Image(other) {}

      png::Text &add_text_payload(const std::string &keyword, const void *ptr, std::size_t size);
      png::Text &add_text_payload(const std::string &keyword, const std::vector<std::uint8_t> &data);

      void remove_text_payload(const png::Text &payload);

      std::vector<png::Text> get_text_payloads(const std::string &keyword) const;

      std::vector<std::vector<std::uint8_t>> extract_text_payloads(const std::string &keyword) const;

      png::ZText &add_ztext_payload(const std::string &keyword, const void *ptr, std::size_t size);
      png::ZText &add_ztext_payload(const std::string &keyword, const std::vector<std::uint8_t> &data);

      void remove_ztext_payload(const png::ZText &payload);

      std::vector<png::ZText> get_ztext_payloads(const std::string &keyword) const;

      std::vector<std::vector<std::uint8_t>> extract_ztext_payloads(const std::string &keyword) const;
   };
}

#endif