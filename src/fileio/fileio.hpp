#pragma once

#include <vector>
#include <string>

namespace nanofill::fileio {

std::vector<std::string>
open_text_file(const char* filename);

}