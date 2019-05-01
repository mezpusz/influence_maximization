#pragma once

#include <vector>
#include <cstdint>

using node_id_t = int64_t;

#define NULL_NODE_ID INT64_MAX
#define ACTIVE    1
#define ACTIVE_U  (ACTIVE << 1)
#define ACTIVE_B  (ACTIVE_U << 1)
#define ACTIVE_BU (ACTIVE_B << 1)

struct node {
    node_id_t id;
    std::vector<node*> neighbors;
    int active_flag = 0;
    double mg1 = 0.0f;
    double mg2 = 0.0f;
    int flag = 0;
    node* prev_best = nullptr;

    bool is(int x) {
        return (active_flag & x) > 0;
    }

    void set(int x) {
        active_flag |= x;
    }

    void reset() {
        active_flag = 0;
    }
};
