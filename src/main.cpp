
#include "inputparser.h"
#include "node.h"
#include "optionparser.h"

#include <limits.h>
#include <deque>
#include <iostream>
#include <queue>
#include <thread>
#include <unordered_map>

#define SIM_COUNT 10000
#define THREAD_COUNT 1
#define P 0.9f

inline double randp()
{
    return std::rand() / (RAND_MAX * 1.0f);
}

std::vector<node> create_nodes(const neighbour_pair_map ne_pairs) {
    std::vector<node> nodes(ne_pairs.size());
    auto ne_it = ne_pairs.begin();
    for (int i = 0; i < nodes.size(); i++) {
        nodes[i].id = ne_it->first;
        for (const auto& ne : ne_it->second) {
            if (ne < nodes[i].id) { // neighbor is already in vector
                int j = 0;
                for (;j < nodes.size() && nodes[j].id < ne; j++);
                nodes[i].neighbors.push_back(&nodes[j]);
                nodes[j].neighbors.push_back(&nodes[i]);
            }
        }
        ne_it++;
    }
    return nodes;
}


struct simulation_thread {
    std::thread m_thread;
    std::vector<node> m_nodes;
    std::unordered_map<node_id_t, node*> m_id_map;

    simulation_thread(const neighbour_pair_map ne_pairs)
        : m_nodes(create_nodes(ne_pairs))
    {
        for (int i = 0; i < m_nodes.size(); i++) {
            m_id_map.insert(std::make_pair(m_nodes[i].id, &m_nodes[i]));
        }
    }

    double simulate(node_id_t u_id, std::vector<node_id_t> s_id) {
        std::vector<node*> s;
        for (const auto& id : s_id) {
            s.push_back(m_id_map[id]);
        }
        node* u = m_id_map[u_id];
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

struct simulation_pool {
    std::vector<simulation_thread> threads;
    std::deque<simulation_thread*> active_queue;

    simulation_pool(neighbour_pair_map ne_pairs)
        : threads()
        , active_queue()
    {
        for (int i = 0; i < THREAD_COUNT; i++) {
            threads.push_back(simulation_thread(ne_pairs));
            active_queue.push_back(&threads[i]);
        }
    }

    double simulate(node_id_t u_id, std::vector<node_id_t> s_id) {
        auto t = active_queue.front();
        active_queue.pop_front();
        auto gain = t->simulate(u_id, s_id);
        active_queue.push_back(t);
        return gain;
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
    simulation_pool p(ne_pairs);
    auto nodes = create_nodes(ne_pairs);

    node* last_seed = nullptr;
    node* cur_best = nullptr;
    std::vector<node_id_t> s;
    marg_queue q;
    double tev = 0.0f; // total expected value
    int i = 0;
    for (auto& n : nodes) {
        std::vector<node_id_t> sn;
        n.mg1 = p.simulate(n.id, sn);
        n.prev_best = cur_best;
        if (cur_best != nullptr) {
            sn.push_back(cur_best->id);
            n.mg2 = p.simulate(n.id, sn);
        } else {
            n.mg2 = n.mg1;
        }
        q.push(&n);
        if (cur_best == nullptr || n.mg1 > cur_best->mg1) {
            cur_best = &n;
        }
        std::cout << ++i << " nodes ready" << std::endl;
        if (i == 10) {
            break;
        }
    }
    return 0;
    int k = 30;
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
            u->mg1 = p.simulate(u->id, s);
            u->prev_best = cur_best;
            if (cur_best != nullptr) {
                auto bs = s;
                bs.push_back(cur_best->id);
                u->mg2 = p.simulate(u->id, bs);
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