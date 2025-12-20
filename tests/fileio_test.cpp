#include "gtest/gtest.h"
#include "fileio/csv.hpp"
#include "fileio/fileio.hpp"
#include "consts/consts.hpp"

TEST(FileIO, ParseCSVData) {
    std::vector<std::string> file_data = {
        { "34200.01399412,3,16085616,100,310400,-1" },
        { "34200.01399412,1,16116348,100,310500,-1" },
        { "34200.015247805,1,16116658,100,310400,-1" },
        { "34200.015442111,1,16116704,100,310500,1" },
    };

    auto columns = nanofill::fileio::parse_csv_data<nanofill::consts::TradingDataCSVFormat>(file_data);

    ASSERT_EQ(4U, columns.size());

    ASSERT_FLOAT_EQ(std::get<0>(columns[0]), 34200.01399412);
    ASSERT_FLOAT_EQ(std::get<0>(columns[1]), 34200.01399412);
    ASSERT_FLOAT_EQ(std::get<0>(columns[2]), 34200.015247805);
    ASSERT_FLOAT_EQ(std::get<0>(columns[3]), 34200.015442111);

    ASSERT_EQ(std::get<1>(columns[0]), 3U);
    ASSERT_EQ(std::get<1>(columns[1]), 1U);
    ASSERT_EQ(std::get<1>(columns[2]), 1U);
    ASSERT_EQ(std::get<1>(columns[3]), 1U);

    ASSERT_EQ(std::get<2>(columns[0]), 16085616U);
    ASSERT_EQ(std::get<2>(columns[1]), 16116348U);
    ASSERT_EQ(std::get<2>(columns[2]), 16116658U);
    ASSERT_EQ(std::get<2>(columns[3]), 16116704U);

    ASSERT_EQ(std::get<3>(columns[0]), 100U);
    ASSERT_EQ(std::get<3>(columns[1]), 100U);
    ASSERT_EQ(std::get<3>(columns[2]), 100U);
    ASSERT_EQ(std::get<3>(columns[3]), 100U);

    ASSERT_EQ(std::get<4>(columns[0]), 310400U);
    ASSERT_EQ(std::get<4>(columns[1]), 310500U);
    ASSERT_EQ(std::get<4>(columns[2]), 310400U);
    ASSERT_EQ(std::get<4>(columns[3]), 310500U);

    ASSERT_EQ(std::get<5>(columns[0]), -1);
    ASSERT_EQ(std::get<5>(columns[1]), -1);
    ASSERT_EQ(std::get<5>(columns[2]), -1);
    ASSERT_EQ(std::get<5>(columns[3]), 1);
}