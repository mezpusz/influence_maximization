
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

// This method is just for control but not yet working
std::map<node_id_t, double> maximize_influence_max_degree(
        const neighbour_pair_map& ne_pairs,
        size_t iter,
        size_t k,
        size_t threads,
        double p) {
    simulator_pool pool(ne_pairs, iter, threads, p);
    // store own copy for accessing nodes
    auto nodes = create_nodes(ne_pairs);

    std::cout << "Finding max degree nodes" << std::endl;
    std::vector<std::pair<node_id_t, size_t>> max_degrees;
    max_degrees.reserve(k+1);
    for (const auto& [id, neighbors] : ne_pairs) {
        if (max_degrees.empty()) {
            max_degrees.push_back(std::make_pair(id, neighbors.size()));
        } else {
            bool placed = false;
            for (int i = 0; i < max_degrees.size(); i++) {
                if (neighbors.size() > max_degrees[i].second) {
                    max_degrees.push_back(std::make_pair(0,0));
                    for (int j = max_degrees.size()-1; j > i; j--) {
                        max_degrees[j] = max_degrees[j-1];
                    }
                    max_degrees[i] = std::make_pair(id, neighbors.size());
                    if (max_degrees.size() > k) {
                        max_degrees.resize(k);
                    }
                    placed = true;
                    break;
                }
            }
            if (!placed && max_degrees.size() < k) {
                max_degrees.push_back(std::make_pair(id, neighbors.size()));
            }
        }
    }

    std::map<node_id_t, double> res;
    std::vector<node_id_t> sn;
    double tev = 0.0f; // total expected value
    for (const auto& [id, n] : max_degrees) {
        std::cout << "Node " << res.size() << " has " << n << " neighbors" << std::endl;
        auto pair = pool.run(id, NULL_NODE_ID, sn);
        tev += pair.second;
        res.insert(std::make_pair(id, pair.second));
        sn.push_back(id);
        std::cout << "k = " << res.size() << std::endl;
        std::cout << "Total expected value: " << tev << std::endl;
        std::cout << "Ids (score):" << std::endl;
        for (const auto& [id, score] : res) {
            std::cout << id << " " << score << std::endl;
        }
    }
    return res;
}

// This method implements the CELF++ version
std::map<node_id_t, double> maximize_influence(
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
    std::map<node_id_t, double> res;
    marg_queue q;
    double tev = 0.0f; // total expected value
    int i = 0;
    std::cout << "Initializing nodes..." << std::endl;
    for (auto& n : nodes) {
        // This vector is just a placeholder here because
        // we don't have any nodes already added to the result 
        std::vector<node_id_t> sn;
        // b_id means the current best in the whole project
        auto b_id = (cur_best == nullptr)
            ? NULL_NODE_ID
            : cur_best->id;
        // The simulation is run for the already added set,
        // with the current best and without a current best also
        auto pair = pool.run(n.id, b_id, sn);
        n.mg1 = pair.first; // First contains run without current best
        n.prev_best = cur_best;
        n.mg2 = pair.second; // Second contains run with current best
        std::cout << ++i << " nodes ready " << n.id << " " << n.mg1 << " " << n.mg2 << std::endl;
        q.push(&n);
        if (cur_best == nullptr || n.mg1 > cur_best->mg1) {
            cur_best = &n;
        }
    }
    while (res.size() < k) {
        auto u = q.top();
        q.pop(); // pop u here, so we can update q properly if needed
        if (u->flag == res.size()) {
            res.insert(std::make_pair(u->id, u->mg1));
            tev += u->mg1;
            last_seed = u;
            cur_best = nullptr;
            std::cout << "k = " << res.size() << std::endl;
            std::cout << "Total expected value: " << tev << std::endl;
            std::cout << "Ids (score):" << std::endl;
            for (const auto& [id, score] : res) {
                std::cout << id << " " << score << std::endl;
            }
            continue;
        } else if (u->prev_best == last_seed && u->flag == res.size()-1) {
            u->mg1 = u->mg2;
            q.push(u);
        } else {
            auto b_id = (cur_best == nullptr)
                ? NULL_NODE_ID
                : cur_best->id;
            std::vector<node_id_t> s;
            for (const auto& [id, score] : res) {
                s.push_back(id);
            }
            auto pair = pool.run(u->id, b_id, s);
            u->mg1 = pair.first;
            u->prev_best = cur_best;
            u->mg2 = pair.second;
            q.push(u);
        }
        u->flag = res.size();
        cur_best = q.top(); // double check this
    }
    return res;
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
        infile_str = "facebook/0.edges";
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
    std::map<node_id_t, double> res;
    if (opt.GetBoolOption("--max_degree")) {
        res = maximize_influence_max_degree(ne_pairs, iter, k, threads, p);
    } else {
        res = maximize_influence(ne_pairs, iter, k, threads, p);
    }
    std::ofstream out(outfile_str);
    if (out.is_open()) {
        double sum = 0.0f;
        for (const auto& [id, score] : res) {
            out << id << " " << score << std::endl;
            sum += score;
        }
        out << std::endl;
        out << "Expected value: " << sum << std::endl;
        out.close();
    } else {
        return -1;
    }

    return 0;
}