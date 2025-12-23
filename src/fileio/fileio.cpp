#include "fileio.hpp"
#include <fstream>

namespace nanofill::fileio {

// Open a text file, returning the data, which is just a vector of file lines.
// テキストファイルを開けて、ファイルの行のvectorとしてデータを返す。
std::vector<std::string>
open_text_file(const char* filename) {
    // There are faster ways to do this but let's not over-complicate things.
    // We are only doing this so that we have some data to work with.
    // もっと速い方法があるが、複雑にしないようにしよう。
    // ただデータがあるようにするためにこれをしてる。

    std::fstream file(filename, std::ios::in);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + std::string(filename));
    }

    std::vector<std::string> file_data;
    
    for (std::string line; std::getline(file, line); ) {
        file_data.push_back(line);
    }

    return file_data;
}

}