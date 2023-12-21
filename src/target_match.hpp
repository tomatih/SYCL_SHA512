#pragma once

#include <array>
#include <cstdint>
#include <set>
#include <string>

std::set<std::array<uint64_t, 512/64>> read_in_targets(const std::string& file_path);
void target_cracking(const std::string& dictionary_path, const std::string& target_path);
