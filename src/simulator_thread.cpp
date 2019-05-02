
#include "simulator_thread.h"

std::future<std::pair<uint64_t, uint64_t>> simulator_thread::add_job(node_id_t u_id, node_id_t b_id, std::vector<node_id_t> s_id) {
    m_result = std::promise<std::pair<uint64_t, uint64_t>>();
    {
        std::lock_guard<std::mutex> l(lock);
        m_job = std::make_optional(std::make_tuple(u_id, b_id, s_id));
    }
    cond.notify_one();
    return m_result.get_future();
}

std::pair<double, double> simulator_pool::run(node_id_t u_id, node_id_t b_id, std::vector<node_id_t> s_id) {
    std::vector<std::future<std::pair<uint64_t, uint64_t>>> fs;
    uint64_t gain_sum = 0;
    uint64_t gain_sum_b = 0;
    for (int i = 0; i < thread_count; i++) {
        fs.push_back(threads[i]->add_job(u_id, b_id, s_id));
    }
    for (auto& f : fs) {
        auto pair = f.get();
        gain_sum += pair.first;
        gain_sum_b += pair.second;
    }
    return std::make_pair(
        (1.0f*gain_sum)/iterations,
        (1.0f*gain_sum_b)/iterations
    );
}
