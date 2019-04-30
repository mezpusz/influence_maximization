
#include "inputparser.h"
#include "node.h"
#include "optionparser.h"

#include <limits.h>
#include <deque>
#include <iostream>
#include <queue>
#include <thread>
#include <optional>
#include <future>
#include <unordered_map>
#include <condition_variable>

#define SIM_COUNT 10000
#define THREAD_COUNT std::thread::hardware_concurrency()
#define P 0.9f

inline double randp(unsigned int* seed)
{
    return rand_r(seed) / (RAND_MAX * 1.0f);
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

int bfs(unsigned int* seed, std::vector<node*> s) {
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
            auto p = randp(seed);
            if (p >= P) {
                n->active = true;
                q.push_back(n);
                covered_nodes++;
            }
        }
    }
    return covered_nodes;
}

double simulate(
        std::vector<node>& nodes,
        std::unordered_map<node_id_t, node*>& id_map,
        node_id_t u_id, std::vector<node_id_t> s_id) {
    std::vector<node*> s;
    for (const auto& id : s_id) {
        s.push_back(id_map[id]);
    }
    node* u = id_map[u_id];
    uint64_t gain_sum = s.size();
    auto count = SIM_COUNT/THREAD_COUNT;
    unsigned int seed(time(0));
    for (int i = 0; i < count; i++) {
        for (auto& n : nodes) {
            n.active = false;
        }
        auto cs = bfs(&seed, s);
        if (!u->active) {
            std::vector<node*> us;
            us.push_back(u);
            auto cu = bfs(&seed, us);
            gain_sum += cu - cs;
        }
    }
    return gain_sum;
}

struct simulation_thread {
    std::thread m_thread;
    std::optional<std::pair<node_id_t, std::vector<node_id_t>>> m_job;
    std::condition_variable cond;
    std::mutex lock;
    std::atomic<bool> m_running;
    std::promise<double> m_result;

    simulation_thread(const neighbour_pair_map ne_pairs, bool create_thread)
        : m_thread()
        , m_job()
        , m_running(create_thread)
    {
        if (create_thread) {
            m_thread = std::thread([this, ne_pairs = std::as_const(ne_pairs)](){
                std::vector<node> nodes = create_nodes(ne_pairs);
                std::unordered_map<node_id_t, node*> id_map;
                for (int i = 0; i < nodes.size(); i++) {
                    id_map.insert(std::make_pair(nodes[i].id, &nodes[i]));
                }

                while (true) {
                    node_id_t u_id;
                    std::vector<node_id_t> s_id;
                    {
                        std::unique_lock<std::mutex> l(lock);
                        if (m_running && !m_job.has_value()) {
                            cond.wait(l);
                        }
                        if (!m_job.has_value()) {
                            return;
                        }
                        u_id = m_job.value().first;
                        s_id = m_job.value().second;
                        m_job.reset();
                    }
                    auto res = simulate(nodes, id_map, u_id, s_id);
                    m_result.set_value(res);
                }
            });
        }
    }

    ~simulation_thread() {
        m_running = false;
        cond.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    std::future<double> add_job(node_id_t u_id, std::vector<node_id_t> s_id) {
        m_result = std::promise<double>();
        {
            std::lock_guard<std::mutex> l(lock);
            m_job = std::make_optional(std::make_pair(u_id, s_id));
        }
        cond.notify_one();
        return m_result.get_future();
    }
};

struct simulation_pool {
    std::vector<simulation_thread*> threads;

    simulation_pool(neighbour_pair_map ne_pairs)
        : threads()
    {
        threads.reserve(THREAD_COUNT);
        for (int i = 0; i < THREAD_COUNT; i++) {
            threads.push_back(new simulation_thread(ne_pairs, true));
        }
    }

    ~simulation_pool() {
        for (int i = 0; i < THREAD_COUNT; i++) {
            delete threads[i];
        }
    }

    double simulate(node_id_t u_id, std::vector<node_id_t> s_id) {
        std::vector<std::future<double>> fs;
        double gain = 0.0f;
        for (int i = 0; i < THREAD_COUNT; i++) {
            fs.push_back(threads[i]->add_job(u_id, s_id));
        }
        for (auto& f : fs) {
            gain += f.get();
        }
        return gain/SIM_COUNT;
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
        std::cout << ++i << " nodes ready " << n.id << " " << n.mg1 << " " << n.mg2 << std::endl;
        q.push(&n);
        if (cur_best == nullptr || n.mg1 > cur_best->mg1) {
            cur_best = &n;
        }
        if (i == 60) {
            break;
        }
    }
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