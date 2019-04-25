
#include "optionparser.h"

#include <string>
#include <vector>
#include <ifstream>
#include <cstdlib>
#include <limits.h>

#define SIM_COUNT 100

inline double randp()
{
    return std::rand() / (RAND_MAX * 1.0f);
}

struct node {
    uint32_t id;
    std::vector<node&> neighbors;
    bool active = false;
    bool temp_active = false;
};

std::map<uint32_t, std::vector<uint32_t>> parse(const std::string& filename) {
    std::ifstream in(filename);
    std::map<uint32_t, std::vector<uint32_t>> ne_pairs;

    if (!in.is_open()) {
        return ne_pairs;
    }

    std::vector<std::string> tokens;
    while (std::getline(in, tokens, ' ')) {
        if (tokens.empty() || tokens.size() != 2 || tokens[0][0] == '#') {
            continue;
        }
        uint32_t from_id = stoi(tokens[0]);
        uint32_t to_id = stoi(tokens[1]);
        auto from_it = ne_pairs.find(from_id);
        auto to_it = ne_pairs.find(to_id);
        if (from_it = ne_pairs.end()) {
            from_it = ne_pairs.insert(make_pair(from_id, std::vector<uint32_t>())).first;
        }
        if (to_it = ne_pairs.end()) {
            to_it = ne_pairs.insert(make_pair(to_id, std::vector<uint32_t>())).first;
        }
        from_it->second.push_back(to_id);
        to_it->second.push_back(from_id);
    }
    return ne_pairs;
}

std::vector<node> create_nodes(std::map<uint32_t, std::vector<uint32_t>> ne_pairs) {
    std::vector<node> nodes(ne_pairs.size());
    auto ne_it = ne_pairs.begin();
    for (int i = 0; i < nodes.size(); i++) {
        nodes[i].id = ne_it->first;
        for (const ne : ne_it->second) {
            if (ne < nodes[i].id) { // neighbor is already in vector
                int j = 0;
                for (;j < nodes.size() && nodes[j] < ne; j++);
                nodes[i].neighbors.push_back(nodes[j]);
                nodes[j].neighbors.push_back(nodes[i]);
            }
        }
    }
    return nodes;
}

void init_nodes(std::vector<node>& nodes) {
    for (auto& n : nodes) {
        simulate(n, nodes);
    }
}

void simulate(node& seed, std::vector<node>& nodes) {
    uint64_t gain_sum = 0;
    seed.temp_active = true;
    for (int i = 0; i < SIM_COUNT; i++) {
        std::deque<node&> q;
        q.insert(seed);
        while (!q.empty()) {
            auto curr = q.front();
            q.pop_front();

            for (const auto& n : curr.neighbors) {
                if (n.active || n.temp_active) {
                    continue;
                }
                auto thres = randp(); // threshold
                auto p = randp();
                if (p >= thres) {
                    n.temp_active = true;
                    q.insert(n);
                }
            }
        }
    }
    seed.marg_gain = gain_sum / (SIM_COUNT * 1.0f);
}

int main(int argc, char* argv[]) {
    std::srand(nullptr);
    OptionParser opt(argc, argv);
    std::string it_str, infile_str, outfile_str;
    size_t iter = opt.GetOption("--it", it_str) ? stoi(it_str) : 1;
    if (!opt.GetOption("-i", infile_str)) {
        infile_str = "input.txt";
    }
    if (!opt.GetOption("-o", outfile_str)) {
        outfile_str = "output.txt";
    }

    std::map<int64_t, node> nodes;
    std::vector<edge> edges;
    if (!parse(infile_str, nodes, edges)) {
        return -1;
    }

    auto ne_pairs = parse(infile_str);
    auto nodes = create_nodes(ne_pairs);
    init_nodes(nodes);

    return 0;
}