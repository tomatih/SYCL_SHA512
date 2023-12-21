#include "correctness.hpp"

#include <cstdint>
#include <iostream>
#include <openssl/sha.h>
#include <stdexcept>

#ifdef GPU
  #include <sycl/sycl.hpp>
#endif

#include "utils.hpp"
#include "sha512.hpp"

std::string professionalSHA512(const std::string& password){
  // prepare memory
  uint8_t hash[SHA512_DIGEST_LENGTH];
  // run the calculations
  SHA512((unsigned char*)password.c_str(), password.length(), hash);
  // convert and return
  return hash_to_string<uint8_t>(hash);
}

void compare_results(const std::vector<std::string>& password_list, uint64_t* hashes){
  // remember if a fail occurred
  bool failed = false;
  // for all passwords
  for(size_t i = 0; i < password_list.size(); ++i){
    // convert to unified format
    std::string my_hash = hash_to_string<uint64_t>(hashes + i*512/64);
    // get reference value
    std::string correct_hash = professionalSHA512(password_list[i]);
    // compare
    if(my_hash != correct_hash){
      std::cout << "FAIL at index "<<i<<std::endl;
      failed = true;
    }
  }
  // display success message
  if(!failed){
    std::cout << "All hashes match expectation" << std::endl;
  }
}

void check_correctness(const std::string& input_file_path){
  // Common setup

  // get passwords to hash
  auto password_list = read_in_passwords(input_file_path);
  // allocate message  and hash memory
  uint64_t* messages = (uint64_t *)std::malloc(1024/8 * password_list.size());
  uint64_t* hashes = (uint64_t *) std::malloc(512/8 * password_list.size());
  // memory safety
  if(messages == nullptr || hashes == nullptr){
    throw std::runtime_error("Not enough system memory to process given list");
  }
  // preprocess passwords
  for(size_t i=0; i<password_list.size(); i++){
    preprocess(password_list[i], messages + i*1024/64);
  }

  // CPU section

  std::cout << "Checking CPU" << std::endl;
  // process all messages
  for(size_t i=0; i<password_list.size(); i++){
    hash_message(messages, hashes, i);
  }
  // 3rd party test
  compare_results(password_list, hashes);

  // GPU section

  #ifdef GPU
    std::cout << "Checking GPU" << std::endl;
    // Create GPU context
    auto q = sycl::queue{ sycl::gpu_selector_v };
    // allocate GPU memory
    auto message_d = sycl::malloc_device<uint64_t>(1024/64 * password_list.size(), q);
    auto hashes_d = sycl::malloc_device<uint64_t>(512/64 * password_list.size(), q);
    // copy preprocessed into device memory
    q.memcpy(message_d, messages, 1024/8 * password_list.size()).wait();
    // process all messages
    q.parallel_for(sycl::range<1>{password_list.size()},
                   [=](sycl::id<1> id){
                     hash_message(message_d, hashes_d, id.get(0));
                   }).wait();
    // get results back
    q.memcpy(hashes, hashes_d, 512/8 * password_list.size()).wait();
    // free device memory
    sycl::free(message_d, q);
    sycl::free(hashes_d, q);
    // 3rd party test
    compare_results(password_list, hashes);
  #endif

  // release system memory
  std::free(messages);
  std::free(hashes);
}