#include "inputparser.h"

#include <fstream>
#include <iostream>
#include <sstream>

neighbour_pair_map parse(const std::string& filename) {
    //std::cout << __FUNCTION__ << std::endl;
    std::ifstream in(filename);
    neighbour_pair_map ne_pairs;

    if (!in.is_open()) {
        std::cout << "Cannot open input file " << filename << std::endl;
        return ne_pairs;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line[0] == '#') {
            continue;
        }
        std::istringstream is(line);
        node_id_t from_id;
        node_id_t to_id;
        is >> from_id >> to_id;
        auto from_it = ne_pairs.find(from_id);
        auto to_it = ne_pairs.find(to_id);
        if (from_it == ne_pairs.end()) {
            from_it = ne_pairs.insert(make_pair(from_id, std::vector<node_id_t>())).first;
        }
        if (to_it == ne_pairs.end()) {
            to_it = ne_pairs.insert(make_pair(to_id, std::vector<node_id_t>())).first;
        }
        from_it->second.push_back(to_id);
        to_it->second.push_back(from_id);
    }
    return ne_pairs;
}