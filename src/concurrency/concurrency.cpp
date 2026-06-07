#include <pthread.h>
#include "concurrency.hpp"
#include <cstdint>
#include <thread>
#include <print>

namespace nanofill::concurrency {

// Pins the current thread to the given core ID.
// 現在のスレッドを指定されたコアに固定する。
void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    if (ret != 0) {
        std::println("Pinning thread failed with error code {}", ret);
        std::abort();
    }
}

}