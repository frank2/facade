#pragma once

#include <cstring>
#include <exception>
#include <iostream>

#define LOG_INFO(message) std::cout << "[!] [" << __FUNCTION__ << "] " << message << std::endl;
#define LOG_SUCCESS(message) std::cout << "[+] [" << __FUNCTION__ << "] " << message << std::endl
#define LOG_FAILURE(message) std::cout << "[-] [" << __FUNCTION__ << "] " << message << std::endl

#define INIT() \
   LOG_INFO("Running tests.");\
   int result = 0\

#define COMPLETE() \
   LOG_INFO(result << " errors."); \
   return result \
   
#define ASSERT(statement) \
   try { \
      if (statement) {                     \
         LOG_SUCCESS(#statement << ": OK");         \
      }\
      else {\
         LOG_FAILURE(#statement << ": ERROR");\
         LOG_FAILURE("Statement failed.");\
         result += 1;\
      }\
   }\
   catch (std::exception &e) {                  \
      LOG_FAILURE(#statement << ": EXCEPTION");\
      LOG_FAILURE("Unhandled exception: " << e.what());\
      result += 1;\
   }\

#define ASSERT_THROWS(statement, exc_class) \
   try { \
      (statement);\
      LOG_FAILURE(#statement << ": NO EXCEPTION");\
      LOG_FAILURE("Statement did not throw " << #exc_class);\
      result += 1;\
   }\
   catch (exc_class &e_success) {\
      LOG_SUCCESS(#statement << ": EXCEPTED");\
      LOG_SUCCESS("Got exception: " << e_success.what());\
   }\
   catch (std::exception &e_failure) {\
      LOG_FAILURE(#statement << ": WRONG EXCEPTION");                   \
      LOG_FAILURE("The wrong exception occurred: " << e_failure.what()); \
      result += 1;\
   }\

#define ASSERT_SUCCESS(statement) \
   try { \
      (statement);\
      LOG_SUCCESS(#statement << ": NO EXCEPTION");\
   }\
   catch (std::exception &e) {                   \
      LOG_FAILURE(#statement << ": EXCEPTED");\
      LOG_FAILURE("Got exception: " << e.what());\
      result += 1;\
   }\

#define PROCESS_RESULT(test) \
   try {\
      result += test();\
   }\
   catch (std::exception &e) {\
      LOG_FAILURE("Unhandled exception while running test: " << e.what()); \
      result += 1;\
   }\
   LOG_INFO(result << " errors so far." << std::endl);   \
   
