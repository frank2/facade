#include <facade.hpp>

using namespace facade;

std::uint16_t facade::endian_swap_16(std::uint16_t value) {
   std::uint16_t result = value;
   std::uint8_t *ptr = reinterpret_cast<std::uint8_t *>(&result);
   std::swap(ptr[0],ptr[1]);
   return result;
}

std::uint32_t facade::endian_swap_32(std::uint32_t value) {
   std::uint32_t result = value;
   std::uint8_t *ptr = reinterpret_cast<std::uint8_t *>(&result);
   std::swap(ptr[0],ptr[3]);
   std::swap(ptr[1],ptr[2]);
   return result;
}

std::uint32_t facade::crc32(const void *ptr, std::size_t size, std::uint32_t init_crc) {
   auto crc = init_crc ^ 0xFFFFFFFF;
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);

   for (std::size_t i=0; i<size; ++i)
      crc = CRC32_TABLE[(crc ^ u8_ptr[i]) & 0xFF] ^ (crc >> 8);

   return crc ^ 0xFFFFFFFF;
}

std::vector<std::uint8_t> facade::compress(const void *ptr, std::size_t size, int level) {
   int z_result;
   z_stream stream;
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);
   std::vector<std::uint8_t> result;

   stream.zalloc = Z_NULL;
   stream.zfree = Z_NULL;
   stream.opaque = Z_NULL;
   z_result = deflateInit(&stream, level);
   if (z_result != Z_OK) { throw exception::ZLibError(z_result); }

   stream.avail_in = static_cast<std::uint32_t>(size);
   stream.next_in = const_cast<std::uint8_t *>(u8_ptr);

   do
   {
      std::uint8_t chunk[8192];
      std::memset(chunk, 0, sizeof(chunk));

      stream.avail_out = 8192;
      stream.next_out = &chunk[0];
      z_result = deflate(&stream, Z_FINISH);
      if (z_result != Z_STREAM_END && z_result != Z_OK) { throw exception::ZLibError(z_result); }

      result.insert(result.end(), &chunk[0], &chunk[8192 - stream.avail_out]);
   } while (stream.avail_out == 0);

   deflateEnd(&stream);

   return result;
}

std::vector<std::uint8_t> facade::compress(const std::vector<std::uint8_t> &vec, int level) {
   return facade::compress(vec.data(), vec.size(), level);
}

std::vector<std::uint8_t> facade::decompress(const void *ptr, std::size_t size) {
   int z_result;
   z_stream stream;
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);
   std::vector<std::uint8_t> result;

   stream.zalloc = Z_NULL;
   stream.zfree = Z_NULL;
   stream.opaque = Z_NULL;
   stream.avail_in = 0;
   stream.next_in = Z_NULL;
   z_result = inflateInit(&stream);
   if (z_result != Z_OK) { throw exception::ZLibError(z_result); }

   stream.avail_in = static_cast<std::uint32_t>(size);
   stream.next_in = const_cast<std::uint8_t *>(u8_ptr);

   do
   {
      std::uint8_t chunk[8192];

      stream.avail_out = 8192;
      stream.next_out = &chunk[0];
      z_result = inflate(&stream, Z_NO_FLUSH);
      if (z_result != Z_OK && z_result != Z_STREAM_END) { throw exception::ZLibError(z_result); }

      result.insert(result.end(), &chunk[0], &chunk[8192 - stream.avail_out]);
   } while (z_result != Z_STREAM_END);

   inflateEnd(&stream);

   return result;
}

std::vector<std::uint8_t> facade::decompress(const std::vector<std::uint8_t> &vec) {
   return facade::decompress(vec.data(), vec.size());
}

bool facade::is_base64_string(const std::string &base64) {
   std::size_t i=0;

   while (i<base64.size() && base64[i] != '=')
      if (facade::BASE64_ALPHA.find(base64[i++]) == std::string::npos)
         return false;

   if (i != base64.size())
      while (i<base64.size())
         if (base64[i++] != '=')
            return false;

   return true;
}

// implementation copped and modified from here: http://www.adp-gmbh.ch/cpp/common/base64.html
std::string facade::base64_encode(const void *ptr, std::size_t size) {
   std::string ret;
   std::size_t i = 0;
   std::size_t j = 0;
   std::uint8_t char_array_3[3];
   std::uint8_t char_array_4[4];
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);

   while (size--) {
      char_array_3[i++] = *(u8_ptr++);
      
      if (i == 3) {
         char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
         char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
         char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
         char_array_4[3] = char_array_3[2] & 0x3f;

         for(i = 0; (i <4) ; i++)
            ret += facade::BASE64_ALPHA[char_array_4[i]];
         
         i = 0;
      }
   }

   if (i)
   {
      for(j = i; j < 3; j++)
         char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (j = 0; (j < i + 1); j++)
         ret += facade::BASE64_ALPHA[char_array_4[j]];

      while((i++ < 3))
         ret += '=';
   }

   return ret;
}

std::string facade::base64_encode(const std::vector<std::uint8_t> &data) {
   return base64_encode(data.data(), data.size());
}

std::vector<std::uint8_t> facade::base64_decode(const std::string &data) {
   std::size_t in_len = data.size();
   std::size_t i = 0;
   std::size_t j = 0;
   std::size_t in_ = 0;
   std::uint8_t char_array_4[4], char_array_3[3];
   std::vector<std::uint8_t> ret;

   while (in_len-- && data[in_] != '=') {
      if (!std::isalnum(data[in_]) && data[in_] != '+' && data[in_] != '/') {
         throw exception::InvalidBase64Character(data[in_]);
      }
      
      char_array_4[i++] = data[in_]; in_++;
      
      if (i ==4) {
         for (i = 0; i <4; i++)
            char_array_4[i] = static_cast<std::uint8_t>(facade::BASE64_ALPHA.find(char_array_4[i]));

         char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
         char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
         char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

         for (i = 0; (i < 3); i++)
            ret.push_back(char_array_3[i]);
         
         i = 0;
      }
   }

   if (i) {
      for (j = i; j <4; j++)
         char_array_4[j] = 0;

      for (j = 0; j <4; j++)
         char_array_4[j] = static_cast<std::uint8_t>(facade::BASE64_ALPHA.find(char_array_4[j]));

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
   }

   return ret;
}

std::vector<std::uint8_t> facade::read_file(const std::string &filename)
{
   std::ifstream fp(filename, std::ios::binary);
   if (!fp.is_open()) { throw exception::OpenFileFailure(filename); }
   
   auto vec_data = std::vector<std::uint8_t>();
   vec_data.insert(vec_data.end(),
                   std::istreambuf_iterator<char>(fp),
                   std::istreambuf_iterator<char>());

   return vec_data;
}

void facade::write_file(const std::string &filename, const void *ptr, std::size_t size)
{
   std::basic_ofstream<char> fp(filename, std::ios::binary);
   if (!fp.is_open()) { throw exception::OpenFileFailure(filename); }

   fp.write(reinterpret_cast<const char *>(ptr), size);
   fp.close();
}

void facade::write_file(const std::string &filename, const std::vector<std::uint8_t> &vec) {
   write_file(filename, vec.data(), vec.size());
}
