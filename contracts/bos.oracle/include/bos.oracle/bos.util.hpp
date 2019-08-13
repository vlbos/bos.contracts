#pragma once
#include <eosio/eosio.hpp>
#include <vector>
#include <string>

class bos_util
{
  public:
   static std::vector<std::string> get_parameters(const std::string& source)  {
           std::vector<std::string> results;
           const string delimiter = ",";
           size_t prev = 0;
           size_t next = 0;

           while ((next = source.find_first_of(delimiter.c_str(), prev)) !=
                  std::string::npos) {
             if (next - prev != 0) {
               results.push_back(source.substr(prev, next - prev));
             }
             prev = next + 1;
           }

           if (prev < source.size()) {
             results.push_back(source.substr(prev));
           }

           return results;
         }

         static uint64_t convert_to_int(const string &parameter) {
           bool isOK = false;
           const char *nptr = parameter.c_str();
           char *endptr = NULL;
           errno = 0;
           uint64_t val = std::strtoull(nptr, &endptr, 10);
           // error ocur
           if ((errno == ERANGE && (val == ULLONG_MAX)) ||
               (errno != 0 && val == 0)) {

           }
           // no digit find
           else if (endptr == nptr) {

           } else if (*endptr != '\0') {
             // printf("Further characters after number: %s\n", endptr);
           } else {
             isOK = true;
           }

           return val;
         }
};