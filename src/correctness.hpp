#pragma once

#include <cstdint>
#include <string>
#include <vector>

std::string professionalSHA512(const std::string& password);
void compare_results(const std::vector<std::string>& password_list, uint64_t* hashes);
void check_correctness(const std::string& input_file_path);
