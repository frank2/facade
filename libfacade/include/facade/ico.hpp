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
   /// @brief The C header for an icon bitmap entry.
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

   /// @brief The C header for an icon directory, the root of an icon file.
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

   /// @brief A Windows icon file.
   class
   EXPORT
   Icon
   {
   public:
      /// @brief A C++ representation of a bitmap entry within the icon file.
      using Entry = std::pair<IconDirEntry, std::vector<std::uint8_t>>;

   private:
      std::vector<Entry> _entries;

   public:
      /// @brief A simple enumeration to differentiate between PNG sections and bitmap sections.
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
      /// @brief Syntactic sugar to get an entry within the icon file.
      Entry &operator[](std::size_t index) {
         return this->get_entry(index);
      }
      /// @brief Syntactic sugar to get a const entry within the icon file.
      const Entry &operator[](std::size_t index) const {
         return this->get_entry(index);
      }

      /// @brief Return the number of bitmap entries within the icon file.
      std::size_t size(void) const;

      /// @brief Parse the given pointer and size as an icon file.
      /// @throws exception::InsufficientSize
      /// @throws exception::InvalidIconHeader
      /// @throws exception::OutOfBounds
      ///
      void parse(const void *ptr, std::size_t size);
      /// @brief Parse the given byte vector as an icon file.
      /// @sa parse(const void *, std::size_t)
      ///
      void parse(const std::vector<std::uint8_t> &vec);
      /// @brief Parse the given file as an icon file.
      /// @sa parse(const void *, std::size_t)
      ///
      void parse(const std::string &filename);

      /// @brief Get a bitmap entry within the icon file.
      /// @returns An Icon::Entry pair.
      /// @throws exception::OutOfBounds
      ///
      Entry &get_entry(std::size_t index);
      /// @brief Get a const bitmap entry within the icon file.
      /// @returns A const Icon::Entry pair.
      /// @throws exception::OutOfBounds
      ///
      const Entry &get_entry(std::size_t index) const;
      /// @brief Set the given entry at the given index in the icon file's bitmap directory.
      /// @throws exception::OutOfBounds
      ///
      void set_entry(std::size_t index, const Entry &data);
      /// @brief Set the given entry object and image data at the given index in the icon file's bitmap directory.
      /// @throws exception::OutOfBounds
      ///
      void set_entry(std::size_t index, const IconDirEntry &entry, const std::vector<std::uint8_t> &data);

      /// @brief Return the type of bitmap the given directory bitmap is. Possible values are Icon::EntryType::ENTRY_BMP and Icon::EntryType::ENTRY_PNG.
      /// @throws exception::OutOfBounds
      ///
      EntryType entry_type(std::size_t index) const;

      /// @brief Convert this icon object to its file representation.
      /// @throws exception::NoIconData
      ///
      std::vector<std::uint8_t> to_file() const;
      /// @brief Convert this icon object to a file and save it to disk.
      /// @sa Icon::to_file
      /// @sa facade::write_file
      ///
      void save(const std::string &filename) const;

      /// @brief Resize the number of entries this icon object can hold.
      ///
      void resize(std::size_t size);

      /// @brief Insert and return the given bitmap entry at the given index.
      /// @throws exception::OutOfBounds
      ///
      Entry &insert_entry(std::size_t index, const Entry &entry);
      /// @brief Insert and return the given entry header and bitmap data at the given index.
      /// @throws exception::OutOfBounds
      ///
      Entry &insert_entry(std::size_t index, const IconDirEntry &entry, const std::vector<std::uint8_t> &data);
      /// @brief Append a bitmap entry pair to the end of this icon's bitmap directory.
      ///
      Entry &append_entry(const Entry &entry);
      /// @brief Append an entry header and bitmap data to the end of the icon's bitmap directory.
      ///
      Entry &append_entry(const IconDirEntry &entry, const std::vector<std::uint8_t> &data);
      /// @brief Remove the bitmap entry at the given index.
      /// @throws exception::OutOfBounds
      ///
      void remove_entry(std::size_t index);
   };
}}
#endif
