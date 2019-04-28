#pragma once

#include <vector>
#include <cstdint>

using node_id_t = uint32_t;

struct node {
    node_id_t id;
    std::vector<node*> neighbors;
    bool active = false;
    double mg1 = 0.0f;
    double mg2 = 0.0f;
    int flag = 0;
    node* prev_best = nullptr;
};
