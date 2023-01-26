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
