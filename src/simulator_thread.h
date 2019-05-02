#pragma once

#include "node.h"
#include "inputparser.h"
#include "simulator.h"

#include <thread>
#include <optional>
#include <future>

class simulator_thread {
public:
    simulator_thread(
        const neighbour_pair_map& ne_pairs,
        size_t iter,
        size_t threads,
        double p)
        : m_thread()
        , m_job()
        , m_running(true)
    {
        m_thread = std::thread([this, ne_pairs, iter, threads, p](){
            simulator sim(p, iter/threads, ne_pairs);
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
                auto res = sim.run(u_id, b_id, s_id);
                m_result.set_value(res);
            }
        });
    }

    ~simulator_thread() {
        m_running = false;
        cond.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    std::future<std::pair<uint64_t, uint64_t>> add_job(node_id_t u_id, node_id_t b_id, std::vector<node_id_t> s_id);

private:
    std::thread m_thread;
    std::optional<std::tuple<node_id_t, node_id_t, std::vector<node_id_t>>> m_job;
    std::condition_variable cond;
    std::mutex lock;
    std::atomic<bool> m_running;
    std::promise<std::pair<uint64_t, uint64_t>> m_result;
};

struct simulator_pool {
    std::vector<simulator_thread*> threads;
    size_t thread_count;
    size_t iterations;

    simulator_pool(
        const neighbour_pair_map& ne_pairs,
        size_t iter,
        size_t thread_count,
        double p)
        : threads()
        , thread_count(thread_count)
        , iterations(iter)
    {
        for (int i = 0; i < thread_count; i++) {
            threads.push_back(new simulator_thread(ne_pairs, iter, thread_count, p));
        }
    }

    ~simulator_pool() {
        for (auto& t : threads) {
            delete t;
        }
    }

    std::pair<double, double> run(node_id_t u_id, node_id_t b_id, std::vector<node_id_t> s_id);
};
