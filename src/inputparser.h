#pragma once

#include "node.h"

#include <map>
#include <vector>

using neighbour_pair_map = std::map<node_id_t, std::vector<node_id_t>>;

neighbour_pair_map parse(const std::string& filename);