
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

std::pair<uint64_t, uint64_t> bfs(unsigned int* seed, node* u, node* b, std::vector<node*> s) {
    std::deque<node*> q;
    for (auto& n : s) {
        n->set(ACTIVE);
        q.push_back(n);
    }
    while (!q.empty()) {
        auto curr = q.front();
        q.pop_front();

        for (auto& n : curr->neighbors) {
            if (n->is(ACTIVE)) {
                continue;
            }
            auto p = randp(seed);
            if (p >= P) {
                n->set(ACTIVE);
                q.push_back(n);
            }
        }
    }
    if (b != nullptr && !(b->is(ACTIVE))) {
        q.clear();
        b->set(ACTIVE_B);
        q.push_back(b);
        while (!q.empty()) {
            auto curr = q.front();
            q.pop_front();

            for (auto& n : curr->neighbors) {
                if (n->is(ACTIVE | ACTIVE_B)) {
                    continue;
                }
                auto p = randp(seed);
                if (p >= P) {
                    n->set(ACTIVE_B);
                    q.push_back(n);
                }
            }
        }
    }
    uint64_t covered_u = 0;
    uint64_t covered_b_u = 0;
    if (!u->is(ACTIVE)) {
        q.clear();
        u->set(ACTIVE_U);
        q.push_back(u);
        while (!q.empty()) {
            auto curr = q.front();
            q.pop_front();
            bool active_b = curr->is(ACTIVE_B | ACTIVE_BU);

            for (auto& n : curr->neighbors) {
                if (n->is(ACTIVE | ACTIVE_U)) {
                    continue;
                }
                auto p = randp(seed);
                if (p >= P) {
                    n->set(ACTIVE_U);
                    q.push_back(n);
                    covered_u++;
                    // the node we are coming from is the
                    // extension of the b region, mark it
                    if (active_b) {
                        n->set(ACTIVE_BU);
                    // otherwise we only need to make sure
                    // that we have not reached the original
                    // b region
                    } else if (!n->is(ACTIVE_B)) {
                        covered_b_u++;
                    }
                }
            }
        }
    }
    return std::make_pair(covered_u, covered_b_u);
}

std::pair<uint64_t, uint64_t> simulate(
        unsigned int* seed,
        std::vector<node>& nodes,
        std::unordered_map<node_id_t, node*>& id_map,
        node_id_t u_id,
        node_id_t b_id,
        std::vector<node_id_t> s_id) {
    std::vector<node*> s;
    for (const auto& id : s_id) {
        s.push_back(id_map[id]);
    }
    node* u = id_map[u_id];
    node* b = b_id == NULL_NODE_ID ? nullptr : id_map[b_id];
    uint64_t gain_sum = 0;
    uint64_t gain_sum_b = 0;
    auto count = SIM_COUNT/THREAD_COUNT;
    for (int i = 0; i < count; i++) {
        for (auto& n : nodes) {
            n.reset();
        }
        auto pair = bfs(seed, u, b, s);
        gain_sum += pair.first;
        gain_sum_b += pair.second;
    }
    return std::make_pair(gain_sum, gain_sum_b);
}

struct simulation_thread {
    std::thread m_thread;
    std::optional<std::tuple<node_id_t, node_id_t, std::vector<node_id_t>>> m_job;
    std::condition_variable cond;
    std::mutex lock;
    std::atomic<bool> m_running;
    std::promise<std::pair<uint64_t, uint64_t>> m_result;

    simulation_thread(const neighbour_pair_map ne_pairs)
        : m_thread()
        , m_job()
        , m_running(true)
    {
        m_thread = std::thread([this, ne_pairs = std::as_const(ne_pairs)](){
            unsigned int seed = std::rand();
            std::vector<node> nodes = create_nodes(ne_pairs);
            std::unordered_map<node_id_t, node*> id_map;
            for (int i = 0; i < nodes.size(); i++) {
                id_map.insert(std::make_pair(nodes[i].id, &nodes[i]));
            }

            while (true) {
                node_id_t u_id;
                node_id_t b_id;
                std::vector<node_id_t> s_id;
                {
                    std::unique_lock<std::mutex> l(lock);
                    if (m_running && !m_job.has_value()) {
                        cond.wait(l);
                    }
                    if (!m_job.has_value()) {
                        return;
                    }
                    u_id = std::get<0>(m_job.value());
                    b_id = std::get<1>(m_job.value());
                    s_id = std::get<2>(m_job.value());
                    m_job.reset();
                }
                auto res = simulate(&seed, nodes, id_map, u_id, b_id, s_id);
                m_result.set_value(res);
            }
        });
    }

    ~simulation_thread() {
        m_running = false;
        cond.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    std::future<std::pair<uint64_t, uint64_t>> add_job(node_id_t u_id, node_id_t b_id, std::vector<node_id_t> s_id) {
        m_result = std::promise<std::pair<uint64_t, uint64_t>>();
        {
            std::lock_guard<std::mutex> l(lock);
            m_job = std::make_optional(std::make_tuple(u_id, b_id, s_id));
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
            threads.push_back(new simulation_thread(ne_pairs));
        }
    }

    ~simulation_pool() {
        for (int i = 0; i < THREAD_COUNT; i++) {
            delete threads[i];
        }
    }

    std::pair<double, double> simulate(node_id_t u_id, node_id_t b_id, std::vector<node_id_t> s_id) {
        std::vector<std::future<std::pair<uint64_t, uint64_t>>> fs;
        uint64_t gain_sum = 0;
        uint64_t gain_sum_b = 0;
        for (int i = 0; i < THREAD_COUNT; i++) {
            fs.push_back(threads[i]->add_job(u_id, b_id, s_id));
        }
        for (auto& f : fs) {
            auto pair = f.get();
            gain_sum += pair.first;
            gain_sum_b += pair.second;
        }
        return std::make_pair(
            (1.0f*gain_sum)/SIM_COUNT,
            (1.0f*gain_sum_b)/SIM_COUNT
        );
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
        auto b_id = (cur_best == nullptr)
            ? NULL_NODE_ID
            : cur_best->id;
        auto pair = p.simulate(n.id, b_id, sn);
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
            auto b_id = (cur_best == nullptr)
                ? NULL_NODE_ID
                : cur_best->id;
            auto pair = p.simulate(u->id, b_id, s);
            u->mg1 = pair.first;
            u->prev_best = cur_best;
            u->mg2 = pair.second;
            q.push(u);
        }
        u->flag = s.size();
        cur_best = q.top(); // double check this
    }

    return 0;
}