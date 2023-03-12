#ifndef __FACADE_PAYLOAD_HPP
#define __FACADE_PAYLOAD_HPP

//! @file payload.hpp
//! @brief Payload functionality as it relates to images.
//! @sa facade::PNGPayload
//!

#include <facade/png.hpp>
#include <facade/ico.hpp>

namespace facade
{
   /// @brief A PNG-based payload helper class.
   ///
   /// There are four main ways to add payloads to images:
   /// * **Trailing data**: data appended to the very end of a PNG image. This is the "quick and dirty" solution to adding arbitrary
   ///                      payload data to a PNG image. See facade::png::Image::set_trailing_data.
   /// * **tEXt section**: a `tEXt` chunk with base64-encoded binary data. See facade::PNGPayload::add_text_payload.
   /// * **zTXt section**: a `zTXt` chunk where the base64-encoded data is compressed. See facade::PNGPayload::add_ztext_payload.
   /// * **Steganography**: a steganographic payload across the raw PNG image data. See facade::PNGPayload::create_stego_payload.
   ///
   /// Here is an example of encoding payloads into a PNG image:
   /// @include payload_creation.cpp
   ///
   /// And here is an example of extracting payloads from a PNG image:
   /// @include payload_extraction.cpp
   ///
   class
   EXPORT
   PNGPayload : public png::Image
   {
   public:
      PNGPayload() : png::Image() {}
      PNGPayload(const void *ptr, std::size_t size) : png::Image(ptr, size) {}
      PNGPayload(const std::vector<std::uint8_t> &vec) : png::Image(vec) {}
      PNGPayload(const std::string &filename) : png::Image(filename) {}
      PNGPayload(const PNGPayload &other) : png::Image(other) {}

      /// @brief Add a `tEXt` section payload to the PNG file.
      ///
      /// The same keyword can be added multiple times.
      ///
      /// @param keyword The keyword to give to the `tEXt` section payload.
      /// @param ptr The pointer of data to add to the payload.
      /// @param size The size of the given data.
      /// @return A facade::png::Text object representing the newly added section to the PNG image.
      /// @sa facade::png::Text
      ///
      png::Text &add_text_payload(const std::string &keyword, const void *ptr, std::size_t size);

      /// @brief Add a `tEXt` section payload to the PNG file.
      ///
      /// The same keyword can be added multiple times.
      ///
      /// @param keyword The keyword to give to the `tEXt` section payload.
      /// @param data The byte vector to add to the payload.
      /// @return A facade::png::Text object representing the newly added section to the PNG image.
      /// @sa facade::png::Text
      ///
      png::Text &add_text_payload(const std::string &keyword, const std::vector<std::uint8_t> &data);

      /// @brief Remove the given `tEXt` payload from the PNG image.
      /// @param payload The payload section to remove.
      /// @throws facade::exception::TextNotFound
      ///
      void remove_text_payload(const png::Text &payload);

      /// @brief Get all corresponding `tEXt` payloads that match the given keyword.
      /// @param keyword The keyword to retrieve payloads from.
      /// @return A vector of facade::png::Text objects corresponding
      /// @throws facade::exception::InvalidBase64String
      ///
      std::vector<png::Text> get_text_payloads(const std::string &keyword) const;

      /// @brief Extract the binary data from the `tEXt` payloads corresponding to a given keyword.
      /// @param keyword The keyword of the given payloads.
      /// @return A vector of byte vectors corresponding to the payloads matching the keyword argument.
      /// @throws facade::exception::InvalidBase64String
      /// @throws facade::exception::InvalidBase64Character
      ///
      std::vector<std::vector<std::uint8_t>> extract_text_payloads(const std::string &keyword) const;

      /// @brief Add a `zTXt` section payload to the PNG file.
      ///
      /// The same keyword can be added multiple times.
      ///
      /// @param keyword The keyword to give to the `zTXt` section payload.
      /// @param ptr The given data pointer.
      /// @param size The size of the data pointer, in bytes.
      /// @return A facade::png::ZText object representing the newly added section to the PNG image.
      /// @sa facade::png::ZText
      ///
      png::ZText &add_ztext_payload(const std::string &keyword, const void *ptr, std::size_t size);

      /// @brief Add a `zTXt` section payload to the PNG file.
      ///
      /// The same keyword can be added multiple times.
      ///
      /// @param keyword The keyword to give to the `zTXt` section payload.
      /// @param data The byte vector to add to the payload.
      /// @return A facade::png::ZText object representing the newly added section to the PNG image.
      /// @sa facade::png::ZText
      ///
      png::ZText &add_ztext_payload(const std::string &keyword, const std::vector<std::uint8_t> &data);

      /// @brief Remove the given `tEXt` payload from the PNG image.
      /// @param payload The payload section to remove.
      /// @throws facade::exception::TextNotFound
      ///
      void remove_ztext_payload(const png::ZText &payload);

      /// @brief Get all corresponding `zTXt` payloads that match the given keyword.
      /// @param keyword The keyword to retrieve payloads from.
      /// @return A vector of facade::png::Text objects corresponding
      /// @throws facade::exception::InvalidBase64String
      /// @throws facade::exception::ZLibError
      ///
      std::vector<png::ZText> get_ztext_payloads(const std::string &keyword) const;

      /// @brief Extract the binary data from the `zTXt` payloads corresponding to a given keyword.
      /// @param keyword The keyword of the given payloads.
      /// @return A vector of byte vectors corresponding to the payloads matching the keyword argument.
      /// @throws facade::exception::InvalidBase64String
      /// @throws facade::exception::InvalidBase64Character
      /// @throws facade::exception::ZLibError
      ///
      std::vector<std::vector<std::uint8_t>> extract_ztext_payloads(const std::string &keyword) const;

      /// @brief Read steganographically-encoded data at an arbitrary bit offset in the image.
      ///
      /// Note that the bit offset must be a multiple of 4.
      ///
      /// @param bit_offset The offset, in bits, to start reading the data.
      /// @param size The size, in bytes, of the data to read.
      /// @return The stego-encoded data slice from the image.
      /// @throws facade::exception::NoImageData
      /// @throws facade::exception::OutOfBounds
      /// @throws facade::exception::PixelMismatch
      /// @throws facade::exception::InvalidBitOffset
      /// 
      std::vector<std::uint8_t> read_stego_data(std::size_t bit_offset, std::size_t size) const;
      /// @brief Write steganographically-encoded data at the given bit offset in the image.
      ///
      /// Note that the bit offset must be a multiple of 4.
      ///
      /// @param ptr The buffer pointer to write.
      /// @param size The size of the buffer, in bytes.
      /// @param bit_offset The offset, in bits, to start writing to.
      /// @throws facade::exception::NoImageData
      /// @throws facade::exception::OutOfBounds
      /// @throws facade::exception::PixelMismatch
      /// @throws facade::exception::InvalidBitOffset
      ///
      void write_stego_data(const void *ptr, std::size_t size, std::size_t bit_offset);
      /// @brief Write steganographically-encoded data at the given bit offset in the image.
      ///
      /// Note that the bit offset must be a multiple of 4.
      ///
      /// @param data The byte vector to write into the image.
      /// @param bit_offset The offset, in bits, to start writing to.
      /// @throws facade::exception::NoImageData
      /// @throws facade::exception::OutOfBounds
      /// @throws facade::exception::PixelMismatch
      /// @throws facade::exception::InvalidBitOffset
      ///
      void write_stego_data(const std::vector<std::uint8_t> &data, std::size_t bit_offset);

      /// @brief Check if the image has a steganographically-encoded payload.
      /// @return Whether or not this image has steganographically-encoded data.
      /// @throws facade::exception::NoImageData
      ///
      bool has_stego_payload() const;
      /// @brief Create a copy of the payload with a steganographically-encoded payload within the image data.
      /// @param ptr The buffer of data to encode in the image.
      /// @param size The size, in bytes, of the given pointer data.
      /// @return A facade::PNGPayload object with a steganographic payload.
      /// @throws facade::exception::UnsupportedPixelType
      /// @throws facade::exception::ImageTooSmall
      ///
      PNGPayload create_stego_payload(const void *ptr, std::size_t size) const;
      /// @brief Create a copy of the payload with a steganographically-encoded payload within the image data.
      /// @param data The vector of byte data to encode in the image.
      /// @return A facade::PNGPayload object with a steganographic payload.
      /// @throws facade::exception::UnsupportedPixelType
      /// @throws facade::exception::ImageTooSmall
      ///
      PNGPayload create_stego_payload(const std::vector<std::uint8_t> &data) const;
      /// @brief Return the steganographically-encoded data from the image.
      /// @return A byte vector of the encoded data.
      /// @throws facade::exception::NoImageData
      /// @throws facade::exception::NoStegoData
      ///
      std::vector<std::uint8_t> extract_stego_payload() const;
   };

   class
   EXPORT
   ICOPayload : public ico::Icon
   {
      std::optional<std::size_t> _index;
      std::optional<PNGPayload> _payload;
      
   public:
      ICOPayload() : ico::Icon() {}
      ICOPayload(const void *ptr, std::size_t size) : ico::Icon(ptr, size) { this->find_png(); }
      ICOPayload(const std::vector<std::uint8_t> &vec) : ico::Icon(vec) { this->find_png(); }
      ICOPayload(const std::string &filename) : ico::Icon(filename) { this->find_png(); }
      ICOPayload(const ICOPayload &other) : _index(other._index), _payload(other._payload), ico::Icon(other) {}

      ICOPayload &operator=(const ICOPayload &other);
      PNGPayload *operator->(void);
      PNGPayload &operator*(void);

      PNGPayload &png_payload(void);
      const PNGPayload &png_payload(void) const;
      
      void find_png(void);
      void reset_png(void);
      void set_png(void);
   };
}

#endif
