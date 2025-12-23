#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <charconv>
#include <sstream>

namespace nanofill::fileio {

template<typename T>
T parse_csv_column(const std::string& column);

template<>
std::uint32_t parse_csv_column<std::uint32_t>(const std::string& column) {
    int output;
    std::from_chars(column.data(), column.data()+column.size(), output);

    // Since we know the data is well-formed, we won't check for errors.
    // Obviously, this is probably a bad idea in the real world.
    // 今回のデータは絶対に形式が正しいので、エラーをチェックしない。もちろん、これは
    // 普通に望ましくないアイデアだ。
    return output;
}

template<>
std::uint16_t parse_csv_column<std::uint16_t>(const std::string& column) {
    int output;
    std::from_chars(column.data(), column.data()+column.size(), output);

    // Since we know the data is well-formed, we won't check for errors.
    // Obviously, this is probably a bad idea in the real world.
    // 今回のデータは絶対に形式が正しいので、エラーをチェックしない。もちろん、これは
    // 普通に望ましくないアイデアだ。
    return output;
}

template<>
std::uint8_t parse_csv_column<std::uint8_t>(const std::string& column) {
    int output;
    std::from_chars(column.data(), column.data()+column.size(), output);

    // Since we know the data is well-formed, we won't check for errors.
    // Obviously, this is probably a bad idea in the real world.
    // 今回のデータは絶対に形式が正しいので、エラーをチェックしない。もちろん、これは
    // 普通に望ましくないアイデアだ。
    return output;
}

template<>
std::int8_t parse_csv_column<std::int8_t>(const std::string& column) {
    int output;
    std::from_chars(column.data(), column.data()+column.size(), output);

    // Since we know the data is well-formed, we won't check for errors.
    // Obviously, this is probably a bad idea in the real world.
    // 今回のデータは絶対に形式が正しいので、エラーをチェックしない。もちろん、これは
    // 普通に望ましくないアイデアだ。
    return output;
}

template<>
double parse_csv_column<double>(const std::string& column) {
    double output;
    std::from_chars(column.data(), column.data()+column.size(), output);

    // Since we know the data is well-formed, we won't check for errors.
    // Obviously, this is probably a bad idea in the real world.
    // 今回のデータは絶対に形式が正しいので、エラーをチェックしない。もちろん、これは
    // 普通に望ましくないアイデアだ。
    return output;
}

template<typename TypesTuple>
TypesTuple
parse_csv_line(const std::string& line) {
    TypesTuple output;
    std::istringstream string_stream(line);
    std::string column;

    auto process_column = [&](auto& field) {
        if (!std::getline(string_stream, column, ',')) {
            throw std::runtime_error("Not enough columns in the provided CSV data");
        }

        field = parse_csv_column<std::decay_t<decltype(field)>>(column);
    };

    std::apply([&](auto&... fields) {
        (process_column(fields), ...);
    }, output);

    return output;
}

template<typename TypeTuple>
std::vector<TypeTuple>
parse_csv_data(const std::vector<std::string>& file_lines) {
    // In reality we would use a library for this, but for parsing a single CSV
    // I felt like it was simpler to create something myself.
    // This is not supposed to be the most optimised approach by any means.
    // 実はライブラリを使うのほうがいいけど、この一つのCSVを解析するために、簡単な自家製の
    // ソリューションのうほうが楽しそうだった。
    // これはどうしても最適解ではない。

    std::vector<TypeTuple> output;

    output.reserve(file_lines.size());

    for (const std::string& line : file_lines) {
        output.push_back(parse_csv_line<TypeTuple>(line));
    }

    return output;
}

}