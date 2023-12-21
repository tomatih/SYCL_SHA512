#include <iostream>

#include "performance.hpp"
#include "target_match.hpp"

#ifdef CORRECTNESS
    #include "correctness.hpp"
#endif

void print_usage(){
  std::cout << "Usage:\n"
               "\t./Task4 {mode} [arguments]\n"
                "Available modes:\n"
               #ifdef CORRECTNESS
               "\tc {input_file} - runs a check whether the implementations returns"
               " expected has values for given inputs. Checked agains OpenSSL\n"
               #endif
               "\tp {input_file} - runs a performance comparison between the CPU and GPU on a given input file\n"
               "\tt {dictionary_file} {target_file} - runs a dictionary attack against supplied target hashes\n";
  std::cout<< std::endl;
}

int main(int argc, char *argv[]){
    // initial argument safety
    if(argc < 2){
        print_usage();
        return 1;
    }

    // get argument
    std::string mode(argv[1]);

    // execute selected mode
    if(mode == "p" && argc == 3){
        performance_comparison(argv[2]);
    }
    else if(mode=="t" && argc == 4){
        target_cracking(argv[2], argv[3]);
    }
    #ifdef CORRECTNESS
    else if(mode == "c" && argc == 3){
      check_correctness(argv[2]);
    }
    #endif
    else{
      print_usage();
      return 1;
    }

    return 0;
}