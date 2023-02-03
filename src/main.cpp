#include <iostream>
#include <fstream>
#include <cstdarg>

#include <argparse/argparse.hpp>
#include <facade.hpp>

#ifdef LIBFACADE_WIN32
#include <windows.h>
#endif

using namespace facade;

#define HEADER \
" ▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▄▄▀▀▀▀▀▀▀▀▀▀▄\n"\
"█ ▀▀▀███████ ▀▀▀███████ ▀▀▀███████ ▀▀▀███████ ▀▀▀██████▄ ▀▀▀███████ █\n"\
"█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀    █▓█ ▀▀▀ ▄▄▄▄▄▄▄▀\n"\
"█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀    █▓█ ▀▀▀ █\n"\
"█ ▀▀▀        ▀▀▀    █▒█ ▀▀▀        ▀▀▀    █▒█ ▀▀▀    █▒█ ▀▀▀ ▀▀▀▀▀▀▄\n"\
"█ ▀▀▀█████▓░ ▀▀▀██▓░█▒█ ▀▀▀        ▀▀▀██▓░█▒█ ▀▀▀    █▒█ ▀▀▀█████▓░ █\n"\
"█ ▀▀▀ ▄▄▄▄▄▄ ▀▀▀ ▄▄ █░█ ▀▀▀        ▀▀▀ ▄▄ █░█ ▀▀▀    █░█ ▀▀▀ ▄▄▄▄▄▄▀\n"\
"█ ▀▀▀ █    █ ▀▀▀ ██ █░█ ▀▀▀        ▀▀▀ ██ █░█ ▀▀▀    █░█ ▀▀▀ █▄▄▄▄▄▄\n"\
"█ ▀▀▀ █    █ ▀▀▀ ██ █ █ ▀▀▀▄▄▄▄▄▄▄ ▀▀▀ ██ █ █ ▀▀▀▄▄▄▄█ █ ▀▀▀▄▄▄▄▄▄▄ █\n"\
"▀▄▀▀▀▄▀    ▀▄▀▀▀▄▀▀▄▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▄▀▀▄▀▀▀▄▀▀▀▀▀▀▀▀▀▄▄▀▀▀▀▀▀▀▀▀▀▄▀\n"\
"  ▀▀▀        ▀▀▀    ▀▀▀ ▀▀▀▀▀▀▀▀▀▀ ▀▀▀    ▀▀▀ ▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀\n"

enum Status {
   NORMAL = 0,
   ALERT,
   ERR,
};

template <typename ...Args>
void status_args()
{
   std::cout << std::endl;
}

template <typename T, typename ...Args>
void status_args(const T& arg, const Args&... args)
{
   std::cout << arg;
   return status_args(args...);
}

template <typename ...Args>
void status(Status status, const Args&... args) {
   switch (status)
   {
   case Status::NORMAL:
      std::cout << "[+] ";
      break;

   case Status::ALERT:
      std::cout << "[!] ";
      break;

   case Status::ERR:
      std::cout << "[-] ";
      break;
   }
   
   status_args(args...);
}

template <typename ...Args>
void status_normal(const Args&... args) {
   status(Status::NORMAL, args...);
}

template <typename ...Args>
void status_alert(const Args&... args) {
   status(Status::ALERT, args...);
}

template <typename ...Args>
void status_error(const Args&... args) {
   status(Status::ERR, args...);
}

std::vector<std::uint8_t> read_file(const std::string &filename)
{
   std::basic_ifstream<std::uint8_t> fp(filename, std::ios::binary | std::ios::ate);
   if (!fp.is_open()) { throw exception::OpenFileFailure(filename); }
   
   auto vec_data = std::vector<std::uint8_t>(fp.tellg());
   fp.seekg(0, std::ios::beg);
   fp.read(vec_data.data(), vec_data.size());
   fp.close();

   return vec_data;
}

void write_file(const std::string &filename, const std::vector<std::uint8_t> data)
{
   std::basic_ofstream<std::uint8_t> fp(filename, std::ios::binary);
   if (!fp.is_open()) { throw exception::OpenFileFailure(filename); }

   fp.write(data.data(), data.size());
   fp.close();
}

int create_payload(const argparse::ArgumentParser &parser) {
   std::cout << HEADER << std::endl;

   status_normal("Creating a new payload!");

   auto input = parser.get<std::string>("--input");
   auto output = parser.get<std::string>("--output");

   status_normal("-> input file:  ", input);
   status_normal("-> output file: ", output, "\n");

   if (!parser.is_used("--trailing-data-payload")
       && !parser.is_used("--text-section-payload")
       && !parser.is_used("--ztxt-section-payload")
       && !parser.is_used("--stego-payload"))
   {
      status_error("No payload type specified.");
      return 1;
   }

   PNGPayload payload;

   try
   {
      status_normal("Parsing ", input, "...");
      payload = PNGPayload(input);
      status_alert("Image parsed!\n");
   }
   catch (exception::Exception &exc)
   {
      status_error("Failed to load input file: ", exc.error);
      return 2;
   }

   if (parser.is_used("--trailing-data-payload"))
   {
      status_normal("Adding trailing data payload to ", input, "...");
      
      auto trailing_file = parser.get<std::string>("--trailing-data-payload");
      std::vector<std::uint8_t> data;

      try {
         status_normal("-> Reading \"", trailing_file, "\"...");
         data = read_file(trailing_file);
         status_alert("-> Got data!");
      }
      catch (exception::Exception &exc) {
         status_error("-> Failed to load payload: ", exc.error);
         return 3;
      }

      status_normal("-> Setting trailing data payload...");
      payload.set_trailing_data(data);
      status_alert("Trailing data payload set!\n");
   }

   if (parser.is_used("--text-section-payload"))
   {
      status_normal("Adding tEXt payload(s) to ", input, "...");

      auto payloads = parser.get<std::vector<std::string>>("--text-section-payload");

      for (std::size_t i=0; i<payloads.size(); i+=2)
      {
         auto keyword = payloads[i];
         auto payload_file = payloads[i+1];

         status_normal("-> Processing payload ", (i/2)+1, "...");
         status_normal("---> Keyword: ", keyword);
         status_normal("---> Payload: ", payload_file);

         std::vector<std::uint8_t> data;
         
         try {
            status_normal("---> Reading file \"", payload_file, "\"...");
            data = read_file(payload_file);
            status_alert("---> Got payload data!");
         }
         catch (exception::Exception &exc)
         {
            status_error("---> Failed to read payload: ", exc.error);
            return 4;
         }

         try {
            status_normal("---> Adding payload to \"", input, "\"...");
            payload.add_text_payload(keyword, data);
            status_normal("---> Payload added!");
         }
         catch (exception::Exception &exc)
         {
            status_error("---> Failed to add payload: ", exc.error);
            return 5;
         }

         status_alert("-> Payload ", (i/2)+1, " processed.\n");
      }

      status_alert("tEXt payloads added!\n");
   }

   if (parser.is_used("--ztxt-section-payload"))
   {
      status_normal("Adding zTXt payload(s) to ", input, "...");

      auto payloads = parser.get<std::vector<std::string>>("--ztxt-section-payload");

      for (std::size_t i=0; i<payloads.size(); i+=2)
      {
         auto keyword = payloads[i];
         auto payload_file = payloads[i+1];

         status_normal("-> Processing payload ", (i/2)+1, "...");
         status_normal("---> Keyword: ", keyword);
         status_normal("---> Payload: ", payload_file);

         std::vector<std::uint8_t> data;
         
         try {
            status_normal("---> Reading file \"", payload_file, "\"...");
            data = read_file(payload_file);
            status_alert("---> Got payload data!");
         }
         catch (exception::Exception &exc)
         {
            status_error("---> Failed to read payload: ", exc.error);
            return 6;
         }

         try {
            status_normal("---> Adding payload to \"", input, "\"...");
            payload.add_ztext_payload(keyword, data);
            status_alert("---> Payload added!");
         }
         catch (exception::Exception &exc)
         {
            status_error("---> Failed to add payload: ", exc.error);
            return 7;
         }

         status_alert("-> Payload ", (i/2)+1, " processed.\n");
      }

      status_alert("zTXt payloads added!\n");
   }

   if (parser.is_used("--stego-payload"))
   {
      status_normal("Adding steganographic payload to ", input, "...");

      auto payload_file = parser.get<std::string>("--stego-payload");
      std::vector<std::uint8_t> data;

      try {
         status_normal("-> Loading file \"", payload_file, "\"...");
         data = read_file(payload_file);
         status_alert("-> Got payload data!");
      }
      catch (exception::Exception &exc)
      {
         status_error("-> Failed to read payload file: ", exc.error);
         return 8;
      }

      status_normal("-> Creating stego payload...");
      status_normal("-> This may take a moment, depending on the size of the image in pixels.");

      payload = payload.create_stego_payload(data);

      status_alert("Stego payload created!\n");
   }

   try {
      status_normal("Saving payload to \"", output, "\"...");
      payload.save(output);
      status_alert("Payload saved!");
   }
   catch (exception::Exception &exc)
   {
      status_error("Failed to save payload: ", exc.error);
      return 9;
   }
   
   return 0;
}

int extract_payloads(const argparse::ArgumentParser &parser) {
   std::cout << HEADER << std::endl;
      
   status_normal("Attempting to extract payloads!");

   auto input = parser.get<std::string>("--input");
   auto output = parser.get<std::string>("--output");

   status_normal("-> input file:       ", input);
   status_normal("-> output directory: ", output, "\n");

   PNGPayload payload;

   try {
      status_normal("Loading input ", input, "...");
      payload = PNGPayload(input);
      status_alert("Input parsed!");
   }
   catch (exception::Exception &exc) {
      status_error("Failed to load input: ", exc.error);
      return 1;
   }

   bool all_techniques = false;
   
   if (parser.is_used("--all")
       || (!parser.is_used("--trailing-data-payload")
           && !parser.is_used("--text-section-payload")
           && !parser.is_used("--ztxt-section-payload")
           && !parser.is_used("--stego-payload")))
   {
      status_normal("Attempting to extract all techniques.");
      all_techniques = true;
   }

   std::size_t payloads_found = 0;

   if (all_techniques || parser.is_used("--trailing-data-payload"))
   {
      status_normal("Searching for trailing data...");

      if (payload.has_trailing_data())
      {
         status_alert("Trailing data found!");
         auto trailing_data = payload.get_trailing_data();
         status_normal("Trailing data size: ", trailing_data.size());

         std::string trailing_filename = output + std::string("/trailing_data.bin");

         try {
            status_normal("Saving trailing data to ", trailing_filename, "...");
            write_file(trailing_filename, trailing_data);
            status_alert("Payload extracted!\n");
         }
         catch (exception::Exception &exc)
         {
            status_error("Failed to save trailing data: ", exc.error);
            return 2;
         }

         ++payloads_found;
      }
      else {
         if (all_techniques) { status_normal("No trailing data found.\n"); }
         else { status_error("No trailing data found."); return 3; }
      }
   }
      
   std::map<std::string,std::size_t> found_payloads;

   if (all_techniques || parser.is_used("--text-section-payload"))
   {
      if (all_techniques)
      {
         if (payload.has_chunk("tEXt"))
         {
            status_normal("Scanning tEXt sections for possible payloads...");

            auto text_chunks = payload.get_chunks("tEXt");
      
            for (auto &chunk : text_chunks)
            {
               auto text = chunk.upcast<png::Text>();
               auto keyword = text.keyword();
               auto data = text.text();

               if (is_base64_string(data))
               {
                  status_alert("Found payload with keyword \"", keyword, "\"!");

                  std::vector<std::uint8_t> decoded_data;
               
                  try {
                     decoded_data = base64_decode(data);
                  }
                  catch (exception::Exception &exc) {
                     status_error("Failed to decode payload: ", exc.error);
                     return 4;
                  }

                  found_payloads[keyword] += 1;
                  std::stringstream decoded_filename;

                  decoded_filename << output << "/" << keyword << "." << std::setw(4) << std::setfill('0') << found_payloads[keyword] << ".bin";
                  
                  try {
                     status_normal("Saving payload to \"", decoded_filename.str(), "\"...");
                     write_file(decoded_filename.str(), decoded_data);
                     status_alert("Payload saved!\n");
                  }
                  catch (exception::Exception &exc) {
                     status_error("Failed to write file: ", exc.error);
                     return 5;
                  }

                  ++payloads_found;
               }
               else { status_normal("Chunk with keyword \"", keyword, "\" is not a payload."); }
            }

            if (payloads_found > 0) { status_normal("Finished extracting payloads!\n"); }
            else { status_normal("No payloads found.\n"); }
         }
         else { status_normal("No tEXt sections found to scan.\n"); }
      }
      else {
         auto keyword = parser.get<std::string>("--text-section-payload");

         if (!payload.has_chunk("tEXt")) {
            status_error("No tEXt sections found in input.");
            return 6;
         }

         std::vector<std::vector<std::uint8_t>> text_payloads;
         
         try {
            status_normal("Attempting to extract payloads with keyword \"", keyword, "\"...");
            text_payloads = payload.extract_text_payloads(keyword);

            if (text_payloads.size() == 0) {
               status_error("No payloads found.");
               return 7;
            }

            status_alert("Found ", text_payloads.size(), " payload", ((text_payloads.size() == 1) ? "!\n" : "s!\n"));
         }
         catch (exception::Exception &exc) {
            status_error("Failed to extract payloads: ", exc.error);
            return 8;
         }

         payloads_found += text_payloads.size();

         for (auto &extracted : text_payloads)
         {
            found_payloads[keyword] += 1;
            std::stringstream decoded_filename;

            decoded_filename << output << "/" << keyword << "." << std::setw(4) << std::setfill('0') << found_payloads[keyword] << ".bin";
                  
            try {
               status_normal("Saving payload to \"", decoded_filename.str(), "\"...");
               write_file(decoded_filename.str(), extracted);
               status_alert("Payload saved!");
            }
            catch (exception::Exception &exc) {
               status_error("Failed to write file: ", exc.error);
               return 9;
            }
         }
      
         if (payloads_found > 0) { status_normal("Finished extracting payloads!\n"); }
         else { status_normal("No payloads found.\n"); }
      }
   }

   if (all_techniques || parser.is_used("--ztxt-section-payload"))
   {
      if (all_techniques)
      {
         if (payload.has_chunk("zTXt"))
         {
            status_normal("Scanning zTXt sections for possible payloads...");

            auto text_chunks = payload.get_chunks("zTXt");
      
            for (auto &chunk : text_chunks)
            {
               auto text = chunk.upcast<png::ZText>();
               auto keyword = text.keyword();
               std::string data;

               try {
                  status_normal("Attempting to decompress zTXt section...");
                  data = text.text();
                  status_alert("Text decompressed!");
               }
               catch (exception::Exception &exc)
               {
                  status_error("Failed to decompress: ", exc.error);
                  return 10;
               }

               if (is_base64_string(data))
               {
                  status_alert("Found payload with keyword \"", keyword, "\"!");

                  std::vector<std::uint8_t> decoded_data;
               
                  try {
                     decoded_data = base64_decode(data);
                  }
                  catch (exception::Exception &exc) {
                     status_error("Failed to decode payload: ", exc.error);
                     return 10;
                  }

                  found_payloads[keyword] += 1;
                  std::stringstream decoded_filename;

                  decoded_filename << output << "/" << keyword << "." << std::setw(4) << std::setfill('0') << found_payloads[keyword] << ".bin";
                  
                  try {
                     status_normal("Saving payload to \"", decoded_filename.str(), "\"...");
                     write_file(decoded_filename.str(), decoded_data);
                     status_alert("Payload saved!\n");
                  }
                  catch (exception::Exception &exc) {
                     status_error("Failed to write file: ", exc.error);
                     return 11;
                  }

                  ++payloads_found;
               }
               else { status_normal("Chunk with keyword \"", keyword, "\" is not a payload."); }
            }
      
            if (payloads_found > 0) { status_normal("Finished extracting payloads!\n"); }
            else { status_normal("No payloads found.\n"); }
         }
         else { status_normal("No zTXt sections found to scan.\n"); }
      }
      else {
         auto keyword = parser.get<std::string>("--ztxt-section-payload");

         if (!payload.has_chunk("zTXt")) {
            status_error("No zTXt sections found in input.");
            return 12;
         }

         std::vector<std::vector<std::uint8_t>> text_payloads;
         
         try {
            status_normal("Attempting to extract payloads with keyword \"", keyword, "\"...");
            text_payloads = payload.extract_ztext_payloads(keyword);

            if (text_payloads.size() == 0) {
               status_error("No payloads found.");
               return 13;
            }

            status_alert("Found ", text_payloads.size(), " payload", ((text_payloads.size() == 1) ? "!\n" : "s!\n"));
         }
         catch (exception::Exception &exc) {
            status_error("Failed to extract payloads: ", exc.error);
            return 14;
         }

         payloads_found += text_payloads.size();

         for (auto &extracted : text_payloads)
         {
            found_payloads[keyword] += 1;
            std::stringstream decoded_filename;

            decoded_filename << output << "/" << keyword << "." << std::setw(4) << std::setfill('0') << found_payloads[keyword] << ".bin";
                  
            try {
               status_normal("Saving payload to \"", decoded_filename.str(), "\"...");
               write_file(decoded_filename.str(), extracted);
               status_alert("Payload saved!");
            }
            catch (exception::Exception &exc) {
               status_error("Failed to write file: ", exc.error);
               return 15;
            }
         }
      
         if (payloads_found > 0) { status_normal("Finished extracting payloads!\n"); }
         else { status_normal("No payloads found.\n"); }
      }
   }

   if (all_techniques || parser.is_used("--stego-payload"))
   {
      try {
         status_normal("Loading input to check for stego data...");
         payload.load();
         status_normal("Input loaded.");
      }
      catch (exception::Exception &exc)
      {
         status_error("Failed to load payload: ", exc.error);
         return 16;
      }

      if (payload.has_stego_payload()) {
         status_alert("Found stego payload!");

         std::vector<std::uint8_t> stego_data;

         try {
            status_normal("Attempting to decode stego data...");
            stego_data = payload.extract_stego_payload();
            status_alert("Payload extracted!");
         }
         catch (exception::Exception &exc) {
            status_error("Failed to extract stego payload: ", exc.error);
            return 17;
         }

         std::string stego_filename = output + std::string("/stego_payload.bin");

         try {
            status_normal("Attempting to save stego payload to \"", stego_filename, "\"...");
            write_file(stego_filename, stego_data);
            status_alert("Stego data saved!\n");
         }
         catch (exception::Exception &exc) {
            status_error("Failed to save stego data: ", exc.error);
            return 18;
         }

         ++payloads_found;
      }
      else {
         if (all_techniques) { status_normal("No stego payload found."); }
         else { status_error("No stego payload found."); return 19; }
      }
   }
   
   status_normal("Extraction techniques exhausted. Found ", payloads_found, " payload", ((payloads_found == 1) ? "." : "s."));

   return 0;
}

int detect_payloads(const argparse::ArgumentParser &parser) {
   auto minimal = parser.get<bool>("--minimal");

   if (!minimal)
   {
      std::cout << HEADER;
      status_normal("Detecting possible payloads in PNG file!");
   }
   
   auto input = parser.get<std::string>("filename");

   if (!minimal) { status_normal("-> input: ", input, "\n"); }

   bool auto_detect = false;

   if (parser.is_used("--auto-detect")
       || (!parser.is_used("--trailing-data")
           && !parser.is_used("--text-data")
           && !parser.is_used("--ztxt-data")
           && !parser.is_used("--stego-data")))
   {
      auto_detect = true;

      if (!minimal)
         status_normal("Automatically detecting all techniques.");
   }

   PNGPayload payload;

   try {
      if (!minimal) { status_normal("Parsing input file \"", input, "\"..."); }
      payload = PNGPayload(input);
      if (!minimal) { status_alert("Input parsed!"); }
   }
   catch (exception::Exception &exc) {
      if (!minimal) { status_error("Failed to load input: ", exc.error); }
      return 1;
   }

   std::vector<std::string> minimal_report;

   if (auto_detect || parser.is_used("--trailing-data"))
   {
      if (!minimal) { status_normal("Checking for trailing data..."); }
      
      if (payload.has_trailing_data()) {
         if (!minimal) { status_alert("Trailing data found!\n"); }
         minimal_report.push_back("trailing-data");
      }
      else if (!minimal) { status_normal("No trailing data found.\n"); }
   }

   if (auto_detect || parser.is_used("--text-data"))
   {
      if (!minimal) { status_normal("Checking for tEXt payloads..."); }

      auto keyword = parser.get<std::string>("--text-data");

      if (keyword.size() == 0 && !minimal) { status_normal("tEXt keyword is blank, scanning tEXt sections."); }

      if (!payload.has_chunk("tEXt"))
      {
         if (!minimal) { status_normal("No tEXt sections present."); }
      }
      else
      {
         auto text_chunks = payload.get_chunks("tEXt");
         std::vector<std::string> found_payloads;

         for (auto &chunk : text_chunks)
         {
            auto text_chunk = chunk.upcast<png::Text>();
            auto found_keyword = text_chunk.keyword();
            if (keyword.size() > 0 && found_keyword != keyword) { continue; }

            auto data = text_chunk.text();

            if (is_base64_string(data)) {
               if (!minimal) { status_alert("Found payload keyword in tEXt: ", found_keyword); }
               found_payloads.push_back(found_keyword);
            }
         }

         for (auto &found_keyword : found_payloads)
         {
            minimal_report.push_back(std::string("tEXt:") + found_keyword);
         }
      }

      if (!minimal) { status_normal("Finished scanning for tEXt payloads.\n"); }
   }
   
   if (auto_detect || parser.is_used("--ztxt-data"))
   {
      if (!minimal) { status_normal("Checking for zTXt payloads..."); }

      auto keyword = parser.get<std::string>("--ztxt-data");

      if (keyword.size() == 0 && !minimal) { status_normal("zTXt keyword is blank, scanning zTXt sections."); }

      if (!payload.has_chunk("zTXt"))
      {
         if (!minimal) { status_normal("No zTXt sections present."); }
      }
      else
      {
         auto text_chunks = payload.get_chunks("zTXt");
         std::vector<std::string> found_payloads;

         for (auto &chunk : text_chunks)
         {
            auto text_chunk = chunk.upcast<png::ZText>();
            auto found_keyword = text_chunk.keyword();
            if (keyword.size() > 0 && found_keyword != keyword) { continue; }

            std::string data;
            try {
               if (!minimal) { status_normal("Attempting to decompress zTXt section with keyword \"", found_keyword, "\"..."); }
               data = text_chunk.text();
               if (!minimal) { status_normal("Text decompressed!"); }
            }
            catch (exception::Exception &exc) {
               if (!minimal) { status_error("Decompression failed: ", exc.error); }
               return 2;
            }
            
            if (is_base64_string(data)) {
               if (!minimal) { status_alert("Found payload keyword in zTXt: ", found_keyword); }
               found_payloads.push_back(found_keyword);
            }
         }

         for (auto &found_keyword : found_payloads)
         {
            minimal_report.push_back(std::string("zTXt:") + found_keyword);
         }
      }

      if (!minimal) { status_normal("Finished scanning for zTXt payloads.\n"); }
   }

   if (auto_detect || parser.is_used("--stego-data"))
   {
      if (!minimal) { status_normal("Checking for stego payload..."); }

      try {
         if (!minimal) { status_normal("Loading input to check for stego data..."); }
         payload.load();
         if (!minimal) { status_normal("Input loaded!"); }
      }
      catch (exception::Exception &exc) {
         if (!minimal) { status_error("Failed to load input: ", exc.error); }
         return 3;
      }

      if (payload.has_stego_payload()) {
         if (!minimal) { status_alert("Stego data present!\n"); }
         minimal_report.push_back("stego");
      }
      else if (!minimal) { status_normal("No stego data present.\n"); }
   }

   if (!minimal) { status_normal("Finished detecting payloads. Found ", minimal_report.size(), " payload", ((minimal_report.size() == 1) ? "." : "s.")); }
   else if (minimal_report.size() > 0) {
      for (std::size_t i=0; i<minimal_report.size(); ++i)
      {
         auto report = minimal_report[i];
         std::cout << report;

         if (i != minimal_report.size()-1)
            std::cout << ",";
      }

      std::cout << std::endl;
   }

   return 0;
}

int main(int argc, char *argv[])
{
   #ifdef LIBFACADE_WIN32
   SetConsoleOutputCP(CP_UTF8);
   setvbuf(stdout, nullptr, _IOFBF, 1000);
   #endif

   argparse::ArgumentParser args("facade", "1.0");
   args.add_description(HEADER "Facade is a tool and library for adding arbitrary payloads to PNG files.");
   
   argparse::ArgumentParser create_args("create");
   create_args.add_description("Create a payload-filled PNG file.");
   
   create_args.add_argument("-i", "--input")
      .help("The input file. This is the PNG file to add a payload to.")
      .required();

   create_args.add_argument("-o", "--output")
      .help("The output file. This is the resulting PNG file from the various modifications.")
      .required();

   create_args.add_argument("-d", "--trailing-data-payload")
      .help("The filename to set as trailing data to the PNG file. Only one of these can be set.");

   create_args.add_argument("-t", "--text-section-payload")
      .nargs(2)
      .append()
      .help("The keyword and filename to add as a 'tEXt' section payload (e.g., -t facade payload.bin). "
            "This can be set multiple times, with the same or differing keywords.");

   create_args.add_argument("-z", "--ztxt-section-payload")
      .nargs(2)
      .append()
      .help("The keyword and filename to add as a 'zTXt' section payload (e.g., -z facade payload.bin). "
            "This can be set multiple times, with the same or differing keywords.");

   create_args.add_argument("-s", "--stego-payload")
      .help("Encode the given filename in the image with basic steganography.");

   args.add_subparser(create_args);
      
   argparse::ArgumentParser extract_args("extract");
   extract_args.add_description("Retrieve payloads from Facade-encoded PNG files.");
   
   extract_args.add_argument("-i", "--input")
      .help("The input file. This is the PNG file to extract payloads from.")
      .required();

   extract_args.add_argument("-o", "--output")
      .help("The output directory. Extracted files will be placed here.")
      .required();

   extract_args.add_argument("-a", "--all")
      .help("Extract all payloads with all implemented techniques. This is the default if nothing is specified.")
      .default_value(false)
      .implicit_value(true);
   
   extract_args.add_argument("-d", "--trailing-data-payload")
      .help("Extract a trailing data payload to the given file.")
      .default_value(false)
      .implicit_value(true);

   extract_args.add_argument("-t", "--text-section-payload")
      .help("The keyword of the 'tEXt' payload to extract. One keyword can have multiple payloads associated with it.");
   
   extract_args.add_argument("-z", "--ztxt-section-payload")
      .help("The keyword of the 'zTXt' payload to extract. One keyword can have multiple payloads associated with it.");

   extract_args.add_argument("-s", "--stego-payload")
      .help("Extract a stegonography-encoded file to the given file.")
      .default_value(false)
      .implicit_value(true);

   argparse::ArgumentParser detect_args("detect");
   detect_args.add_description("Detect what possible methods are encoded in this PNG file.");

   detect_args.add_argument("filename")
      .help("The file to scan.")
      .required();
   
   detect_args.add_argument("-a", "--auto-detect")
      .help("Automatically detect what's in the file. This is the default behavior if no arguments are supplied.")
      .default_value(false)
      .implicit_value(true);

   detect_args.add_argument("-m", "--minimal")
      .help("Return detections in a CSV format.")
      .default_value(false)
      .implicit_value(true);

   detect_args.add_argument("-d", "--trailing-data")
      .help("Check if this PNG has trailing data.")
      .default_value(false)
      .implicit_value(true);

   detect_args.add_argument("-t", "--text-data")
      .help("Check if this PNG has a 'tEXt' section payload. "
            "Supply a blank string to detect all 'tEXt' payloads, or supply a keyword to detect a specific payload.")
      .default_value(std::string(""));

   detect_args.add_argument("-z", "--ztxt-data")
      .help("Check if this PNG has a 'zTXt' section payload. "
            "Supply a blank string to detect all 'zTXt' payloads, or supply a keyword to detect a specific payload.")
      .default_value(std::string(""));

   detect_args.add_argument("-s", "--stego-data")
      .help("Check if this PNG image has a steganographic payload.");

   args.add_subparser(create_args);
   args.add_subparser(extract_args);
   args.add_subparser(detect_args);

   try {
      args.parse_args(argc, argv);
   }
   catch (const std::runtime_error &err)
   {
      std::cerr << "Argument parsing failed: " << err.what() << std::endl;
      std::cerr << args;
      std::exit(1);
   }

   if (!args.is_subcommand_used(create_args) && !args.is_subcommand_used(extract_args) && !args.is_subcommand_used(detect_args))
   {
      std::cerr << "Argument parsing failed: neither create, extract or detect command were used." << std::endl;
      std::cerr << args;
      std::exit(2);
   }
   
   int exit_code;

   try
   {
      if (args.is_subcommand_used(create_args))
         exit_code = create_payload(args.at<argparse::ArgumentParser>("create"));
      else if (args.is_subcommand_used(extract_args))
         exit_code = extract_payloads(args.at<argparse::ArgumentParser>("extract"));
      else if (args.is_subcommand_used(detect_args))
         exit_code = detect_payloads(args.at<argparse::ArgumentParser>("detect"));

      return exit_code;
   }
   catch (std::exception &exc)
   {
      status_error("Unhandled exception: ", std::string(exc.what()));
      return 3;
   }

   return 0;
}
