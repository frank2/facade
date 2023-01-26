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
