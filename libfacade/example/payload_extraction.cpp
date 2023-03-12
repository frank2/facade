#include <facade.hpp>
#include <cassert>

using namespace facade;

int main(int argc, char *argv[]) {
   PNGPayload image("art.payload.png");
   std::string expected_string("Just an arbitrary payload, nothing suspicious here!");
   std::vector<std::uint8_t> expected_data(expected_string.begin(), expected_string.end());

   // we can then extract the payload from the end of the file
   assert(image.get_trailing_data() == expected_data);

   // or the text section
   assert(image.extract_text_payloads("tEXt payload")[0] == expected_data);

   // or the ztxt section
   assert(image.extract_ztext_payloads("zTXt payload")[0] == expected_data);

   // in order to get stego data, we need to load the image first, THEN extract it
   image.load();
   assert(image.extract_stego_payload() == expected_data);

   return 0;
}
