#include <facade.hpp>

using namespace facade;

int main(int argc, char *argv[]) {
   PNGPayload image("../test/art.png");
   std::string payload_string("Just an arbitrary payload, nothing suspicious here!");
   std::vector<std::uint8_t> payload_data(payload_string.begin(), payload_string.end());

   // we can add a payload to the end of the file
   image.set_trailing_data(payload_data);

   // or a text section
   image.add_text_payload("tEXt payload", payload_data);

   // or a ztxt section
   image.add_ztext_payload("zTXt payload", payload_data);

   // or a stego-encoded payload
   auto final_payload = image.create_stego_payload(payload_data);

   // finally, we can save our payload to a new file.
   final_payload.save("art.payload.png");

   return 0;
}
   
