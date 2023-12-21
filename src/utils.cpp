#include "utils.hpp"

#include <fstream>

std::vector<std::string> read_in_passwords(const std::string& file_path){
  // open file
  std::ifstream password_file(file_path);

  // file safety
  if(!password_file){
    throw std::runtime_error("Could not open dictionary file");
  }

  // reading memory
  std::string line;
  std::vector<std::string> out;

  // read in file
  while(std::getline(password_file, line)){
    // length check not necessary as preprocessing does it already
    out.push_back(line);
  }

  password_file.close();
  return out;
}
