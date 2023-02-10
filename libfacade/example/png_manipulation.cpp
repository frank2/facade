#include <facade.hpp>

using namespace facade;

int main(int argc, char *argv[])
{
   png::Image image;

   // first, parse the image.
   image.parse("../test/art.png");

   // we could also just call image.load(), which does the same steps.
   image.decompress();
   image.reconstruct();

   // get the header of the image.
   png::Header &header = image.header();

   // let's black out all the pixels in the image.
   for (std::uint32_t y=0; y<header.height(); ++y)
   {
      for (std::uint32_t x=0; x<header.width(); ++x)
      {
         // get the pixel variant that holds our pixel type.
         png::Pixel pixel_var = image[y][x];

         // we already know the image we're dealing with is an AlphaTrueColor image,
         // so get that pixel type.
         png::AlphaTrueColorPixel8Bit &pixel = std::get<png::AlphaTrueColorPixel8Bit>(pixel_var);

         // invert the pixels
         pixel.red().set_value(0xFF - *pixel.red());
         pixel.green().set_value(0xFF - *pixel.green());
         pixel.blue().set_value(0xFF - *pixel.blue());

         // reassign the pixel
         image[y].set_pixel(pixel, x);
      }
   }

   // filter the new image
   image.filter();

   // compress the new image
   image.compress();

   // save the new image
   image.save("art.inverted.png");

   return 0;
}
