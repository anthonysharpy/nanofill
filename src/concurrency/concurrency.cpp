#include <pthread.h>
#include "concurrency.hpp"
#include <cstdint>
#include <iostream>
#include <thread>

namespace nanofill::concurrency {

// Pins the current thread to the given core ID.
void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    if (ret != 0) {
        std::cout << "Pinning thread failed with error code " << ret << std::endl;
        std::abort();
    }
}

}