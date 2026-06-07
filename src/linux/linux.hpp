#pragma once

#include <fstream>
#include <unistd.h>

namespace nanofill::linux {

double get_ram_usage_mb() {
    long rss_pages = 0, dummy;
    std::ifstream statm("/proc/self/statm");
    statm >> dummy >> rss_pages;  // 1st = total size, 2nd = resident
    long bytes = rss_pages * sysconf(_SC_PAGESIZE);
    return bytes / (1024.0 * 1024.0);
}

}