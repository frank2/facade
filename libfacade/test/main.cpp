#include <framework.hpp>
#include <facade.hpp>

using namespace facade;

int
test_pngimage()
{
   INIT();

   png::Image image;

   ASSERT_SUCCESS(image = png::Image("../test/test.png"));
   ASSERT(image.has_header());

   if (image.has_header())
   {
      auto &header = image.header();
      ASSERT(header.width() == 256);
      ASSERT(header.height() == 256);
      ASSERT(header.bit_depth() == 8);
      ASSERT(header.color_type() == 6);
      ASSERT(header.compression_method() == 0);
      ASSERT(header.filter_method() == 0);
      ASSERT(header.interlace_method() == 0);
      ASSERT(header.pixel_size() == png::AlphaTrueColorPixel8Bit::Bits);
      ASSERT(header.buffer_size() == (256 * 4) * 256 + 256);
   }
   
   ASSERT_THROWS(image[0], exception::NoImageData);
   ASSERT(!image.is_loaded());
   ASSERT_SUCCESS(image.decompress());
   ASSERT(image.is_loaded());
   
   ASSERT_SUCCESS(image.reconstruct());

   std::vector<std::uint8_t> image_raw;

   for (std::size_t i=0; i<image.header().height(); ++i)
   {
      auto raw = png::pixels_to_raw<png::AlphaTrueColorPixel8Bit>(std::get<png::AlphaTrueColorScanline8Bit>(image[i]).pixel_data());
      image_raw.insert(image_raw.end(), raw.begin(), raw.end());
   }

   std::basic_ifstream<std::uint8_t> raw_fp("../test/test.raw", std::ios::binary);
   auto known_raw = std::vector<std::uint8_t>();
   known_raw.insert(known_raw.end(),
                    std::istreambuf_iterator<std::uint8_t>(raw_fp),
                    std::istreambuf_iterator<std::uint8_t>());

   ASSERT(image_raw == known_raw);

   ASSERT_SUCCESS(image.filter());

   /*
   std::basic_ofstream<std::uint8_t> outfile_filtered("art.dumped.filtered", std::ios::binary);

   for (std::size_t i=0; i<image.header().height(); ++i)
   {
      auto raw = std::get<png::AlphaTrueColorScanline8Bit>(image[i]).to_raw();
      outfile_filtered.write(raw.data(), raw.size());
   }

   outfile_filtered.close();
   */

   ASSERT_SUCCESS(image.compress());
   ASSERT_SUCCESS(image.save("test.processed.png"));

   png::Image processed;

   ASSERT_SUCCESS(processed.parse("test.processed.png"));
   ASSERT_SUCCESS(processed.load());

   if (processed.is_loaded())
   {
      image_raw.clear();

      for (std::size_t i=0; i<processed.header().height(); ++i)
      {
         auto raw = png::pixels_to_raw<png::AlphaTrueColorPixel8Bit>(std::get<png::AlphaTrueColorScanline8Bit>(processed[i]).pixel_data());
         image_raw.insert(image_raw.end(), raw.begin(), raw.end());
      }

      ASSERT(image_raw == known_raw);
   }
      
   COMPLETE();
}

int
test_embedding()
{
   INIT();

   png::Image image;

   ASSERT_SUCCESS(image = png::Image("../test/test.png"));

   auto trail_test = image;
   auto test_string = std::string("Hello, Facade!");
   auto test_data = std::vector<std::uint8_t>(test_string.begin(), test_string.end());
   ASSERT_SUCCESS(trail_test.set_trailing_data(test_data));
   ASSERT_SUCCESS(trail_test.save("test.trailing.png"));

   auto trail_load = png::Image("test.trailing.png");
   ASSERT(trail_load.has_trailing_data());

   if (trail_load.has_trailing_data())
   {
      ASSERT(trail_load.get_trailing_data() == test_data);
   }

   auto text_test = image;
   ASSERT(!text_test.has_text());
   ASSERT_SUCCESS(text_test.add_text("FACADE", "This could also contain some arbitrary data!"));
   ASSERT_SUCCESS(text_test.save("test.text.png"));

   auto text_load = png::Image("test.text.png");
   ASSERT(text_load.has_text());

   if (text_load.has_text())
   {
      std::vector<png::Text> facade_text;

      ASSERT_SUCCESS(facade_text = text_load.get_text("FACADE"));
      ASSERT(facade_text.size() > 0);
      
      if (facade_text.size() > 0)
      {
         ASSERT(facade_text[0].has_text());

         if (facade_text[0].has_text())
         {
            auto got_text = facade_text[0].text();
            ASSERT(got_text == "This could also contain some arbitrary data!");
         }
      }
   }

   auto ztext_test = image;
   ASSERT(!ztext_test.has_ztext());
   ASSERT_SUCCESS(ztext_test.add_ztext("FACADE", "This payload is compressed!"));
   ASSERT_SUCCESS(ztext_test.save("test.ztext.png"));

   auto ztext_load = png::Image("test.ztext.png");
   ASSERT(ztext_load.has_ztext());

   if (ztext_load.has_ztext())
   {
      std::vector<png::ZText> facade_text;

      ASSERT_SUCCESS(facade_text = ztext_load.get_ztext("FACADE"));
      ASSERT(facade_text.size() > 0);
      
      if (facade_text.size() > 0)
      {
         ASSERT(facade_text[0].has_text());

         if (facade_text[0].has_text())
         {
            auto got_text = facade_text[0].text();
            ASSERT(got_text == "This payload is compressed!");
         }
      }
   }

   COMPLETE();
}

int
main
(int argc, char *argv[])
{
   INIT();

   LOG_INFO("Testing png::Image objects.");
   PROCESS_RESULT(test_pngimage);

   LOG_INFO("Testing embedding data into PNG objects.");
   PROCESS_RESULT(test_embedding);

   COMPLETE();
}