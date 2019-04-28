#pragma once

#include <vector>
#include <cstdint>

struct node {
    uint32_t id;
    std::vector<node*> neighbors;
    bool active = false;
    double mg1 = 0.0f;
    double mg2 = 0.0f;
    int flag = 0;
    node* prev_best = nullptr;
};
