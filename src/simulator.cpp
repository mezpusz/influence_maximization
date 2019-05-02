
#include "simulator.h"

#include <deque>
#include <iostream>

inline bool randp(double p, unsigned int* seed)
{
    return (rand_r(seed) / (RAND_MAX * 1.0f)) <= p;
}

std::pair<uint64_t, uint64_t> simulator::run(
        node_id_t u_id,
        node_id_t b_id,
        std::vector<node_id_t> s_id) {
        // std::cout << "Starting simulation" << std::endl;
    std::vector<node*> s;
    for (const auto& id : s_id) {
        s.push_back(m_id_map[id]);
    }
    node* u = m_id_map[u_id];
    node* b = b_id == NULL_NODE_ID ? nullptr : m_id_map[b_id];
    uint64_t gain_sum = 0;
    uint64_t gain_sum_b = 0;
    auto count = m_count;
    for (int i = 0; i < count; i++) {
        for (auto& n : m_nodes) {
            n.reset();
        }
        auto pair = bfs(u, b, s);
        gain_sum += pair.first;
        gain_sum_b += pair.second;
    }
    return std::make_pair(gain_sum, gain_sum_b);
}

std::pair<uint64_t, uint64_t> simulator::bfs(node* u, node* b, std::vector<node*> s) {
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
            if (randp(m_p, &m_seed)) {
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
                if (randp(m_p, &m_seed)) {
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
                if (randp(m_p, &m_seed)) {
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
