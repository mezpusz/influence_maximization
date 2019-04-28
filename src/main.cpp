
#include "inputparser.h"
#include "node.h"
#include "optionparser.h"

#include <limits.h>
#include <deque>
#include <iostream>
#include <queue>
#include <thread>

#define SIM_COUNT 10000
#define P 0.9f

inline double randp()
{
    return std::rand() / (RAND_MAX * 1.0f);
}

struct simulation_thread {
    std::thread m_thread;
    std::vector<node> m_nodes;

    simulation_thread(std::map<uint32_t, std::vector<uint32_t>> ne_pairs)
        : m_nodes(ne_pairs.size())
    {
        auto ne_it = ne_pairs.begin();
        for (int i = 0; i < m_nodes.size(); i++) {
            m_nodes[i].id = ne_it->first;
            for (const auto& ne : ne_it->second) {
                if (ne < m_nodes[i].id) { // neighbor is already in vector
                    int j = 0;
                    for (;j < m_nodes.size() && m_nodes[j].id < ne; j++);
                    m_nodes[i].neighbors.push_back(&m_nodes[j]);
                    m_nodes[j].neighbors.push_back(&m_nodes[i]);
                }
            }
            ne_it++;
        }
    }

    double simulate(node* u, std::vector<node*> s) {
        uint64_t gain_sum = s.size();
        for (int i = 0; i < SIM_COUNT; i++) {
            for (auto& n : m_nodes) {
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

private:
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
};

class marg_gain_compare {
public:
    bool operator()(const node* const lhs, const node* const rhs) const {
        return lhs->mg1 < rhs->mg1;
    }
};

using marg_queue = std::priority_queue<node*, std::vector<node*>, marg_gain_compare>;

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
    simulation_thread t(ne_pairs);

    node* last_seed = nullptr;
    node* cur_best = nullptr;
    std::vector<node*> s;
    marg_queue q;
    double tev = 0.0f; // total expected value
    int i = 0;
    for (auto& n : t.m_nodes) {
        std::vector<node*> sn;
        n.mg1 = t.simulate(&n, sn);
        n.prev_best = cur_best;
        if (cur_best != nullptr) {
            sn.push_back(cur_best);
            n.mg2 = t.simulate(&n, sn);
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
            u->mg1 = t.simulate(u, s);
            u->prev_best = cur_best;
            if (cur_best != nullptr) {
                auto bs = s;
                bs.push_back(cur_best);
                u->mg2 = t.simulate(u, bs);
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