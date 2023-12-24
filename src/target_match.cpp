#include "target_match.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "utils.hpp"
#include "sha512.hpp"

std::set<std::array<uint64_t, 512/64>> read_in_targets(const std::string& file_path){
  // open file
  std::ifstream targets_file(file_path);
  
  // file safety
  if(!targets_file){
    throw std::runtime_error("Could not open target file");
  }
  
  // prepare reading memory
  std::string line;
  std::set<std::array<uint64_t, 512/64>> out;
  
  // go through file
  while(std::getline(targets_file, line)){
    // format safety
    if(!std::all_of(line.begin(), line.end(), ::isxdigit)){
      throw std::runtime_error("Target hash " + line + " is not a valid hexadecimal string");
    }
    // allocate memory
    std::array<uint64_t, 512/64> hash{};
    // convert to numerical
    for(size_t i=0;i<hash.size();i++){
      std::stringstream ss;
      ss << std::hex << line.substr(i*16,16);
      ss >> hash[i];
    }
    // add to set
    out.insert(hash);
  }
  
  return out;
}

void target_cracking(const std::string& dictionary_path, const std::string& target_path){
  // read in data
  auto dictionary = read_in_passwords(dictionary_path);
  auto targets = read_in_targets(target_path);
  
  // allocate message  and hash memory
  auto messages = (uint64_t *)std::malloc(1024 / 8 * dictionary.size());
  auto hashes = (uint64_t *) std::malloc(512 / 8 * dictionary.size());
  // memory safety
  if(messages == nullptr || hashes == nullptr){
    throw std::runtime_error("Not enough system memory to process given list");
  }
  // preprocess dictionary
  for(size_t i=0; i<dictionary.size(); i++){
    preprocess(dictionary[i], messages + i*1024/64);
  }
  
  #ifdef GPU
    // Create GPU context
    auto q = sycl::queue{ sycl::gpu_selector_v };
    // allocate GPU memory
    auto message_d = sycl::malloc_device<uint64_t>(1024/64 * dictionary.size(), q);
    auto hashes_d = sycl::malloc_device<uint64_t>(512/64 * dictionary.size(), q);
    // copy over preprocessed data
    q.memcpy(message_d, messages, 1024/8 * dictionary.size()).wait();
    // run computation
    q.parallel_for(sycl::range<1>{dictionary.size()},
                   [=](sycl::id<1> id){
                     hash_message(message_d, hashes_d, id.get(0));
                   }).wait();
    // get results
    q.memcpy(hashes, hashes_d, 512/8 * dictionary.size()).wait();
    // free GPU memory
    sycl::free(message_d, q);
    sycl::free(hashes_d, q);
  #else
    // process on the CPU
    for(size_t i=0; i<dictionary.size(); i++){
      hash_message(messages, hashes, i);
    }
  #endif
  
  // find matches
  size_t found = 0;
  for(size_t i=0; i<dictionary.size();i++){
    // convert from a raw pointer
    std::array<uint64_t, 512/64> hash{};
    std::copy(hashes + i*8, hashes + (i+1)*8, std::begin(hash));
    // match test
    if(targets.contains(hash)){
      std::cout<<dictionary[i]<<" "<<hash_to_string(hash.data())<<std::endl;
      found++;
    }
  }
  // failure message
  if(found < targets.size()){
    std::cout << "Couldn't find all target hashes" << std::endl;
  }
  
  // free memory
  std::free(messages);
  std::free(hashes);
}
