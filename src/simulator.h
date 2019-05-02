#pragma once

#include "node.h"
#include "inputparser.h"

#include <unordered_map>

class simulator {
public:
    simulator(double p, size_t sim_count, const neighbour_pair_map ne_pairs)
        : m_p(p)
        , m_count(sim_count)
        , m_seed(std::rand())
        , m_nodes(create_nodes(ne_pairs))
    {
        for (int i = 0; i < m_nodes.size(); i++) {
            m_id_map.insert(std::make_pair(m_nodes[i].id, &m_nodes[i]));
        }
    }

    std::pair<uint64_t, uint64_t> run(node_id_t u_id, node_id_t b_id, std::vector<node_id_t> s_id);
private:
    std::pair<uint64_t, uint64_t> bfs(node* u, node* b, std::vector<node*> s);

    double m_p;
    size_t m_count;
    unsigned int m_seed;
    std::vector<node> m_nodes;
    std::unordered_map<node_id_t, node*> m_id_map;
};