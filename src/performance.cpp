#include "performance.hpp"

#include <chrono>
#include <iostream>
#ifdef GPU
    #include <sycl/sycl.hpp>
#endif

#include "utils.hpp"
#include "sha512.hpp"

using std::chrono::nanoseconds;

void performance_comparison(const std::string& input_file_path){
  // Welcome user
  std::cout << "Starting performance measurement" << std::endl;
  std::cout << "Current averaging sample size is "<< PERFORMANCE_ITERATIONS << std::endl;
  
  // get passwords to hash
  auto password_list = read_in_passwords(input_file_path);
  std::cout << "Read in " << password_list.size() << " passwords"<< std::endl;
  // allocate message  and hash memory
  auto messages = (uint64_t *)std::malloc(1024 / 8 * password_list.size());
  auto hashes = (uint64_t *) std::malloc(512 / 8 * password_list.size());
  // memory safety
  if(messages == nullptr || hashes == nullptr){
    throw std::runtime_error("Not enough system memory to process given list");
  }
  // preprocess passwords
  for(size_t i=0; i<password_list.size(); i++){
    preprocess(password_list[i], messages + i*1024/64);
  }
  // allocate timing memory
  std::chrono::high_resolution_clock::time_point start, stop;
  nanoseconds whole_duration{0};
  
  // run cpu cycles
  std::cout << "Starting CPU section" << std::endl;
  // Run and time many iteration
  for(size_t i = 0; i < PERFORMANCE_ITERATIONS; i++){
    start = std::chrono::high_resolution_clock::now();
    // process all messages
    for(size_t j=0; j<password_list.size(); j++){
      hash_message(messages, hashes, j);
    }
    stop = std::chrono::high_resolution_clock::now();
    // calculate time delta
    whole_duration += std::chrono::duration_cast<nanoseconds>(stop-start);
  }
  // display results
  std::cout << "CPU took on average: " <<  nanoseconds(whole_duration / PERFORMANCE_ITERATIONS) <<std::endl;
  
  // GPU AREA
  #ifdef GPU
    std::cout << "Starting GPU section" << std::endl;
    // reset accumulator
    whole_duration = nanoseconds(0);
    // Create GPU context
    auto q = sycl::queue{ sycl::gpu_selector_v };
    // allocate GPU memory
    auto message_d = sycl::malloc_device<uint64_t>(1024/64 * password_list.size(), q);
    auto hashes_d = sycl::malloc_device<uint64_t>(512/64 * password_list.size(), q);

    // run warmup cycles
    for(size_t i = 0; i<10; i++){
      q.memcpy(message_d, messages, 1024/8 * password_list.size()).wait();

      q.parallel_for(sycl::range<1>{password_list.size()},
        [=](sycl::id<1> id){
          hash_message(message_d, hashes_d, id.get(0));
        }).wait();

      q.memcpy(hashes, hashes_d, 512/8 * password_list.size()).wait();
    }

    // run actual cycles
    for(size_t i = 0; i<PERFORMANCE_ITERATIONS; i++){
      start = std::chrono::high_resolution_clock::now();
      // memory transfers are included in timing as they are an important overhead
      q.memcpy(message_d, messages, 1024/8 * password_list.size()).wait();

      q.parallel_for(sycl::range<1>{password_list.size()},
                     [=](sycl::id<1> id){
                       hash_message(message_d, hashes_d, id.get(0));
                     }).wait();

      q.memcpy(hashes, hashes_d, 512/8 * password_list.size()).wait();
      stop = std::chrono::high_resolution_clock::now();
      // calculate time delta
      whole_duration += std::chrono::duration_cast<nanoseconds>(stop-start);
    }

    // display results
    std::cout << "GPU took on average: " <<  nanoseconds(whole_duration / PERFORMANCE_ITERATIONS) <<std::endl;

    //free device memory
    sycl::free(message_d, q);
    sycl::free(hashes_d, q);
  #endif
  // free memory
  std::free(messages);
  std::free(hashes);
}
