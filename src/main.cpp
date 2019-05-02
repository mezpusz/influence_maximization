
#include "inputparser.h"
#include "node.h"
#include "optionparser.h"
#include "simulator_thread.h"

#include <iostream>
#include <fstream>
#include <queue>

class marg_gain_compare {
public:
    bool operator()(const node* const lhs, const node* const rhs) const {
        return lhs->mg1 < rhs->mg1;
    }
};

using marg_queue = std::priority_queue<node*, std::vector<node*>, marg_gain_compare>;

std::vector<node_id_t> maximize_influence(
        const neighbour_pair_map& ne_pairs,
        size_t iter,
        size_t k,
        size_t threads,
        double p) {
    simulator_pool pool(ne_pairs, iter, threads, p);
    // store own copy for accessing nodes
    auto nodes = create_nodes(ne_pairs);

    node* last_seed = nullptr;
    node* cur_best = nullptr;
    std::vector<node_id_t> s;
    marg_queue q;
    double tev = 0.0f; // total expected value
    int i = 0;
    std::cout << "Initializing nodes..." << std::endl;
    for (auto& n : nodes) {
        std::vector<node_id_t> sn;
        auto b_id = (cur_best == nullptr)
            ? NULL_NODE_ID
            : cur_best->id;
        auto pair = pool.run(n.id, b_id, sn);
        n.mg1 = pair.first;
        n.prev_best = cur_best;
        n.mg2 = pair.second;
        std::cout << ++i << " nodes ready " << n.id << " " << n.mg1 << " " << n.mg2 << std::endl;
        q.push(&n);
        if (cur_best == nullptr || n.mg1 > cur_best->mg1) {
            cur_best = &n;
        }
        if (i == 60) {
            break;
        }
    }
    while (s.size() < k) {
        auto u = q.top();
        q.pop(); // pop u here, so we can update q properly if needed
        if (u->flag == s.size()) {
            s.push_back(u->id);
            tev += u->mg1;
            last_seed = u;
            cur_best = nullptr;
            std::cout << "k = " << s.size() << std::endl;
            std::cout << "Total expected value: " << tev << std::endl;
            std::cout << "Ids:" << std::endl;
            for (const auto& n : s) {
                std::cout << n << std::endl;
            }
            continue;
        } else if (u->prev_best == last_seed && u->flag == s.size()-1) {
            u->mg1 = u->mg2;
            q.push(u);
        } else {
            auto b_id = (cur_best == nullptr)
                ? NULL_NODE_ID
                : cur_best->id;
            auto pair = pool.run(u->id, b_id, s);
            u->mg1 = pair.first;
            u->prev_best = cur_best;
            u->mg2 = pair.second;
            q.push(u);
        }
        u->flag = s.size();
        cur_best = q.top(); // double check this
    }
    return s;
}

int main(int argc, char* argv[]) {
    std::srand(0);
    OptionParser opt(argc, argv);
    std::string infile_str, outfile_str;
    size_t iter, k, threads;
    if (!opt.GetOption("--it", iter)) {
        iter = 10000;
    }
    if (!opt.GetOption("-k", k)) {
        k = 10;
    }
    if (!opt.GetOption("--threads", threads)) {
        threads = std::thread::hardware_concurrency();
    }
    double p;
    if (!opt.GetOption("-p", p)) {
        p = 0.1f;
    }
    if (!opt.GetOption("-i", infile_str)) {
        infile_str = "CA-HepTh.txt";
    }
    if (!opt.GetOption("-o", outfile_str)) {
        outfile_str = "output.txt";
    }
    std::cout << "Starting influence maximization with parameters:" << std::endl
              << "* input file: " << infile_str << std::endl
              << "* output file: " << outfile_str << std::endl
              << "* k=" << k << std::endl << "* threads used: " << threads << std::endl
              << "* probability of influence: " << p << std::endl
              << "* monte carlo iterations: " << iter << std::endl << std::endl;

    std::cout << "Parsing input file..." << std::endl;
    auto ne_pairs = parse(infile_str);

    auto s = maximize_influence(ne_pairs, iter, k, threads, p);
    std::ofstream out(outfile_str);
    if (out.is_open()) {
        for (const auto& e : s) {
            out << e << std::endl;
        }
        out.close();
    } else {
        return -1;
    }

    return 0;
}