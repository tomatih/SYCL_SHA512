#pragma once

#include <string>
#include <iomanip>
#include <vector>

template<typename T>
std::string hash_to_string(T* hash){
  // prepare output buffer
  std::stringstream hash_stream;

  // for all values in the hash
  for(unsigned int i=0;i<512/(sizeof(T)*8);i++){
    // convert to 2 hex digits
    hash_stream << std::hex << std::setw(sizeof(T)*2) << std::setfill('0') << +hash[i];
  }

  return hash_stream.str();
}

std::vector<std::string> read_in_passwords(const std::string& file_path);
