#pragma once

#include <map>
#include <vector>

std::map<uint32_t, std::vector<uint32_t>> parse(const std::string& filename);