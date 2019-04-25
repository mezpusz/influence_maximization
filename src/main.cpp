
#include "optionparser.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <limits.h>
#include <map>
#include <deque>
#include <iostream>

#define SIM_COUNT 1

inline double randp()
{
    return std::rand() / (RAND_MAX * 1.0f);
}

struct node {
    uint32_t id;
    std::vector<node*> neighbors;
    bool active = false;
    bool temp_active = false;
    double marg_gain;
};

std::map<uint32_t, std::vector<uint32_t>> parse(const std::string& filename) {
    std::cout << __FUNCTION__ << std::endl;
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

std::vector<node> create_nodes(std::map<uint32_t, std::vector<uint32_t>> ne_pairs) {
    std::cout << __FUNCTION__ << std::endl;
    std::vector<node> nodes(ne_pairs.size());
    auto ne_it = ne_pairs.begin();
    for (int i = 0; i < nodes.size(); i++) {
        nodes[i].id = ne_it->first;
        //std::cout << "node " << nodes[i].id << std::endl;
        for (const auto& ne : ne_it->second) {
            //std::cout << ne << " ";
            if (ne < nodes[i].id) { // neighbor is already in vector
                int j = 0;
                for (;j < nodes.size() && nodes[j].id < ne; j++);
                nodes[i].neighbors.push_back(&nodes[j]);
                nodes[j].neighbors.push_back(&nodes[i]);
            }
        }
        //std::cout << std::endl;
        ne_it++;
    }
    return nodes;
}

void simulate(node* seed, std::vector<node>& nodes) {
    std::cout << __FUNCTION__ << std::endl;
    uint64_t gain_sum = 1;
    for (int i = 0; i < SIM_COUNT; i++) {
        for (auto& n : nodes) {
            if (!n.active) {
                n.temp_active = false;
            }
        }
        seed->temp_active = true;
        std::deque<node*> q;
        q.push_back(seed);
        while (!q.empty()) {
            auto curr = q.front();
            q.pop_front();

            for (auto& n : curr->neighbors) {
                if (n->active || n->temp_active) {
                    continue;
                }
                auto thres = randp(); // threshold
                auto p = randp();
                if (p >= thres) {
                    n->temp_active = true;
                    q.push_back(n);
                    gain_sum++;
                }
            }
        }
    }
    seed->marg_gain = gain_sum / (SIM_COUNT * 1.0f);
}

void init_nodes(std::vector<node>& nodes) {
    std::cout << __FUNCTION__ << std::endl;
    for (auto& n : nodes) {
        simulate(&n, nodes);
        std::cout << n.id << " " << n.marg_gain << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::srand(0);
    OptionParser opt(argc, argv);
    std::string it_str, infile_str, outfile_str;
    //size_t iter = opt.GetOption("--it", it_str) ? stoi(it_str) : 1;
    if (!opt.GetOption("-i", infile_str)) {
        infile_str = "input.txt";
    }
    if (!opt.GetOption("-o", outfile_str)) {
        outfile_str = "output.txt";
    }

    auto ne_pairs = parse(infile_str);
    auto nodes = create_nodes(ne_pairs);
    init_nodes(nodes);

    return 0;
}