
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
#include <queue>

#define SIM_COUNT 10000
#define P 0.9f

inline double randp()
{
    return std::rand() / (RAND_MAX * 1.0f);
}

struct node {
    uint32_t id;
    std::vector<node*> neighbors;
    bool active = false;
    double mg1 = 0.0f;
    double mg2 = 0.0f;
    int flag = 0;
    node* prev_best = nullptr;
};

class marg_gain_compare {
public:
    bool operator()(const node* const lhs, const node* const rhs) const {
        return lhs->mg1 < rhs->mg1;
    }
};

using marg_queue = std::priority_queue<node*, std::vector<node*>, marg_gain_compare>;

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

std::vector<node> create_nodes(std::map<uint32_t, std::vector<uint32_t>> ne_pairs) {
    //std::cout << __FUNCTION__ << std::endl;
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

int bfs(std::vector<node*> s) {
    //std::cout << __FUNCTION__ << std::endl;
    uint64_t covered_nodes = s.size();
    std::deque<node*> q;
    for (auto& n : s) {
        n->active = true;
        q.push_back(n);
    }
    while (!q.empty()) {
        auto curr = q.front();
        q.pop_front();

        for (auto& n : curr->neighbors) {
            if (n->active) {
                continue;
            }
            auto p = randp();
            if (p >= P) {
                n->active = true;
                q.push_back(n);
                covered_nodes++;
            }
        }
    }
    return covered_nodes;
}

double simulate(node* u, std::vector<node*> s, std::vector<node>& nodes) {
    //std::cout << __FUNCTION__ << std::endl;
    uint64_t gain_sum = s.size();
    for (int i = 0; i < SIM_COUNT; i++) {
        for (auto& n : nodes) {
            n.active = false;
        }
        auto cs = bfs(s);
        if (!u->active) {
            std::vector<node*> us;
            us.push_back(u);
            auto cu = bfs(us);
            gain_sum += cu - cs;
        }
    }
    return gain_sum / (SIM_COUNT * 1.0f);
}

int main(int argc, char* argv[]) {
    std::srand(0);
    OptionParser opt(argc, argv);
    std::string it_str, infile_str, outfile_str;
    //size_t iter = opt.GetOption("--it", it_str) ? stoi(it_str) : 1;
    if (!opt.GetOption("-i", infile_str)) {
        infile_str = "CA-HepTh.txt";
    }
    if (!opt.GetOption("-o", outfile_str)) {
        outfile_str = "output.txt";
    }

    auto ne_pairs = parse(infile_str);
    auto nodes = create_nodes(ne_pairs);

    node* last_seed = nullptr;
    node* cur_best = nullptr;
    std::vector<node*> s;
    marg_queue q;
    double tev = 0.0f; // total expected value
    int i = 0;
    for (auto& n : nodes) {
        std::vector<node*> sn;
        n.mg1 = simulate(&n, sn, nodes);
        n.prev_best = cur_best;
        if (cur_best != nullptr) {
            sn.push_back(cur_best);
            n.mg2 = simulate(&n, sn, nodes);
        } else {
            n.mg2 = n.mg1;
        }
        q.push(&n);
        if (cur_best == nullptr || n.mg1 > cur_best->mg1) {
            cur_best = &n;
        }
        std::cout << ++i << " nodes ready" << std::endl;
    }
    int k = 30;
    while (s.size() < k) {
        auto u = q.top();
        q.pop(); // pop u here, so we can update q properly if needed
        if (u->flag == s.size()) {
            s.push_back(u);
            tev += u->mg1;
            last_seed = u;
            cur_best = nullptr;
            std::cout << "k = " << s.size() << std::endl;
            std::cout << "Total expected value: " << tev << std::endl;
            std::cout << "Ids:" << std::endl;
            for (const auto& n : s) {
                std::cout << n->id << std::endl;
            }
            continue;
        } else if (u->prev_best == last_seed && u->flag == s.size()-1) {
            u->mg1 = u->mg2;
            q.push(u);
        } else {
            u->mg1 = simulate(u, s, nodes);
            u->prev_best = cur_best;
            if (cur_best != nullptr) {
                auto bs = s;
                bs.push_back(cur_best);
                u->mg2 = simulate(u, bs, nodes);
            } else {
                std::cout << "3" << std::endl;
                u->mg2 = u->mg1;
            }
            q.push(u);
        }
        u->flag = s.size();
        cur_best = q.top(); // double check this
    }

    return 0;
}