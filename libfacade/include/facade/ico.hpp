#ifndef __FACADE_ICO_HPP
#define __FACADE_ICO_HPP

//! @file ico.hpp
//! @brief Code functionality for handling Windows icon files.

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>

#include <facade/platform.hpp>
#include <facade/exception.hpp>
#include <facade/png.hpp>
#include <facade/utility.hpp>

namespace facade
{
namespace ico
{
   struct
   PACK(2)
   IconDirEntry
   {
      std::uint8_t        width;
      std::uint8_t        height;
      std::uint8_t        color_count;
      std::uint8_t        reserved;
      std::uint16_t       planes;
      std::uint16_t       bit_count;
      std::uint32_t       bytes;
      std::uint32_t       offset;
   };
   UNPACK()

   struct
   PACK(2)
   IconDir
   {
      std::uint16_t           reserved;
      std::uint16_t           type;
      std::uint16_t           count;
      IconDirEntry            entries[1];
   };
   UNPACK()

   struct
   BitmapInfoHeader
   {
      std::uint32_t size;
      std::int32_t width;
      std::int32_t height;
      std::uint16_t planes;
      std::uint16_t bit_count;
      std::uint32_t compression;
      std::uint32_t image_size;
      std::int32_t x_pels_per_meter;
      std::int32_t y_pels_per_meter;
      std::uint32_t color_used;
      std::uint32_t color_important;
   };

   struct
   PACK(1)
   RGBQuad
   {
      std::uint8_t blue;
      std::uint8_t green;
      std::uint8_t red;
      std::uint8_t reserved;
   };
   UNPACK()

   struct
   BitmapInfo
   {
      BitmapInfoHeader header;
      RGBQuad colors[1];
   };

   class
   EXPORT
   Icon
   {
   public:
      using Entry = std::pair<IconDirEntry, std::vector<std::uint8_t>>;

   private:
      std::vector<Entry> _entries;

   public:
      enum EntryType
      {
         ENTRY_BMP = 0,
         ENTRY_PNG
      };
      
      Icon() {}
      Icon(const void *ptr, std::size_t size) { this->parse(ptr, size); }
      Icon(const std::vector<std::uint8_t> &vec) { this->parse(vec); }
      Icon(const std::string &filename) { this->parse(filename); }
      Icon(const Icon &other) : _entries(other._entries) {}

      Icon &operator=(const Icon &other) {
         this->_entries = other._entries;

         return *this;
      }
      Entry &operator[](std::size_t index) {
         return this->get_entry(index);
      }
      const Entry &operator[](std::size_t index) const {
         return this->get_entry(index);
      }

      std::size_t size(void) const;

      void parse(const void *ptr, std::size_t size);
      void parse(const std::vector<std::uint8_t> &vec);
      void parse(const std::string &filename);

      Entry &get_entry(std::size_t index);
      const Entry &get_entry(std::size_t index) const;
      void set_entry(std::size_t index, const Entry &data);
      void set_entry(std::size_t index, const IconDirEntry &entry, const std::vector<std::uint8_t> &data);

      EntryType entry_type(std::size_t index) const;

      std::vector<std::uint8_t> to_file() const;
      void save(const std::string &filename) const;

      void resize(std::size_t size);
      Entry &insert_entry(std::size_t index, const Entry &entry);
      Entry &insert_entry(std::size_t index, const IconDirEntry &entry, const std::vector<std::uint8_t> &data);
      Entry &append_entry(const Entry &entry);
      Entry &append_entry(const IconDirEntry &entry, const std::vector<std::uint8_t> &data);
      void remove_entry(std::size_t index);
   };
}}
#endif
