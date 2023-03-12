#include <facade.hpp>

using namespace facade;
using namespace facade::ico;

std::size_t Icon::size(void) const { return this->_entries.size(); }

void Icon::parse(const void *ptr, std::size_t size) {
   if (size < sizeof(IconDir))
      throw exception::InsufficientSize(size, sizeof(IconDir));

   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);
   auto dir = reinterpret_cast<const IconDir *>(u8_ptr);

   if (dir->reserved != 0 || dir->type != 1)
      throw exception::InvalidIconHeader();

   auto dir_size = sizeof(IconDir) - sizeof(IconDirEntry) + (sizeof(IconDirEntry) * dir->count);

   if (dir_size > size)
      throw exception::OutOfBounds(dir_size, size);

   std::vector<Entry> entries;

   for (std::uint16_t i=0; i<dir->count; ++i)
   {
      auto entry = dir->entries[i];

      if (entry.offset+entry.bytes > size)
         throw exception::OutOfBounds(entry.offset+entry.bytes, size);

      auto data = u8_ptr+entry.offset;
      auto data_end = data+entry.bytes;
      entries.push_back(std::make_pair(entry, std::vector<uint8_t>(data, data_end)));
   }

   this->_entries = entries;
}

void Icon::parse(const std::vector<std::uint8_t> &vec) {
   this->parse(vec.data(), vec.size());
}

void Icon::parse(const std::string &filename) {
   this->parse(read_file(filename));
}

Icon::Entry &Icon::get_entry(std::size_t index) {
   if (index >= this->size())
      throw exception::OutOfBounds(index, this->size());

   return this->_entries[index];
}

const Icon::Entry &Icon::get_entry(std::size_t index) const {
   if (index >= this->size())
      throw exception::OutOfBounds(index, this->size());

   return this->_entries[index];
}

void Icon::set_entry(std::size_t index, const Icon::Entry &entry) {
   if (index >= this->size())
      throw exception::OutOfBounds(index, this->size());

   this->_entries[index] = entry;
}

void Icon::set_entry(std::size_t index, const IconDirEntry &entry, const std::vector<std::uint8_t> &data) {
   this->set_entry(index, std::make_pair(entry, data));
}

Icon::EntryType Icon::entry_type(std::size_t index) const {
   auto entry = this->get_entry(index);

   if (entry.second.size() >= 8 && std::memcmp(entry.second.data(), png::Image::Signature, sizeof(png::Image::Signature)) == 0)
      return Icon::EntryType::ENTRY_PNG;
   else
      return Icon::EntryType::ENTRY_BMP;
}

std::vector<std::uint8_t> Icon::to_file(void) const {
   if (this->size() == 0)
      throw exception::NoIconData();
   
   std::size_t buffer_size = sizeof(IconDir) - sizeof(IconDirEntry) + (sizeof(IconDirEntry) * this->size());
   auto buffer = std::vector<std::uint8_t>(buffer_size);

   auto dir = reinterpret_cast<IconDir *>(buffer.data());
   dir->reserved = 0;
   dir->type = 1;
   dir->count = static_cast<std::uint16_t>(this->size());

   for (std::uint16_t i=0; i<dir->count; ++i)
   {
      auto &entry_ref = this->get_entry(i);
      std::memcpy(&dir->entries[i], &entry_ref.first, sizeof(IconDirEntry));

      auto entry_data = &dir->entries[i];
      entry_data->bytes = static_cast<std::uint32_t>(entry_ref.second.size());
      entry_data->offset = static_cast<std::uint32_t>(buffer.size());
      buffer.insert(buffer.end(), entry_ref.second.begin(), entry_ref.second.end());
      
      dir = reinterpret_cast<IconDir *>(buffer.data());
   }

   return buffer;
}

void Icon::save(const std::string &filename) const {
   write_file(filename, this->to_file());
}

void Icon::resize(std::size_t size) { this->_entries.resize(size); }

Icon::Entry &Icon::insert_entry(std::size_t index, const Entry &entry) {
   if (index > this->size())
      throw exception::OutOfBounds(index, this->size());

   this->_entries.insert(this->_entries.begin()+index, entry);

   return this->get_entry(index);
}

Icon::Entry &Icon::insert_entry(std::size_t index, const IconDirEntry &entry, const std::vector<std::uint8_t> &data)
{
   return this->insert_entry(index, std::make_pair(entry, data));
}

Icon::Entry &Icon::append_entry(const Entry &entry)
{
   return this->insert_entry(this->size(), entry);
}

Icon::Entry &Icon::append_entry(const IconDirEntry &entry, const std::vector<std::uint8_t> &data)
{
   return this->append_entry(std::make_pair(entry, data));
}

void Icon::remove_entry(std::size_t index) {
   if (index >= this->size())
      throw exception::OutOfBounds(index, this->size());

   this->_entries.erase(this->_entries.begin()+index);
}
