#pragma once

#include <fstream>
#include <unistd.h>

namespace nanofill::linux {

double get_ram_usage_mb() {
    long rss_pages = 0;
    long dummy = 0;

    std::ifstream statm("/proc/self/statm");
    statm >> dummy >> rss_pages;
    long bytes = rss_pages * sysconf(_SC_PAGESIZE);
    
    return bytes / (1024.0 * 1024.0);
}

}