#include "inputparser.h"

#include <fstream>
#include <iostream>
#include <sstream>

std::map<uint32_t, std::vector<uint32_t>> parse(const std::string& filename) {
    //std::cout << __FUNCTION__ << std::endl;
    std::ifstream in(filename);
    std::map<uint32_t, std::vector<uint32_t>> ne_pairs;

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
        uint32_t from_id;
        uint32_t to_id;
        is >> from_id >> to_id;
        auto from_it = ne_pairs.find(from_id);
        auto to_it = ne_pairs.find(to_id);
        if (from_it == ne_pairs.end()) {
            from_it = ne_pairs.insert(make_pair(from_id, std::vector<uint32_t>())).first;
        }
        if (to_it == ne_pairs.end()) {
            to_it = ne_pairs.insert(make_pair(to_id, std::vector<uint32_t>())).first;
        }
        from_it->second.push_back(to_id);
        to_it->second.push_back(from_id);
    }
    return ne_pairs;
}