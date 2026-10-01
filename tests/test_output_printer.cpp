#include <gtest/gtest.h>
#include "analyzer/output_printer.hpp"
#include "analyzer/types.hpp"
#include "libs/json.hpp"
#include <sstream>
#include <iostream>

class OutputPrinterTest : public ::testing::Test {
protected:
    std::vector<FileStat> sample_stats;

    void SetUp() override {
        sample_stats = {
            {"src/main.cpp", 15, 1700000000, "Alice", "a1b2c3d4"},
            {"src/utils.cpp", 5, 1690000000, "Bob", "e5f6g7h8"},
            {"README.md", 2, 1680000000, "Charlie", "11223344"}
        };
    }
};

TEST_F(OutputPrinterTest, OutputJsonFormat) {
    OutputOptions opts;
    opts.print_type = PrintType::JSON;
    opts.stats = sample_stats;
    opts.top = 2;

    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    OutputPrinter printer(opts);
    printer.output();

    std::cout.rdbuf(old_cout);

    std::string out = buffer.str();
    nlohmann::json parsed;
    ASSERT_NO_THROW(parsed = nlohmann::json::parse(out));
    ASSERT_TRUE(parsed.is_array());
    EXPECT_EQ(parsed.size(), 2);
    EXPECT_EQ(parsed[0]["path"], "src/main.cpp");
    EXPECT_EQ(parsed[0]["commit_count"], 15);
    EXPECT_EQ(parsed[0]["last_author"], "Alice");
    EXPECT_EQ(parsed[0]["last_hash"], "a1b2c3d4");
}

TEST_F(OutputPrinterTest, OutputCsvFormat) {
    OutputOptions opts;
    opts.print_type = PrintType::CSV;
    opts.stats = sample_stats;
    opts.top = 0;

    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    OutputPrinter printer(opts);
    printer.output();

    std::cout.rdbuf(old_cout);

    std::string out = buffer.str();
    EXPECT_NE(out.find("COUNT,FILE,LAST_CHANGE,HASH"), std::string::npos);
    EXPECT_NE(out.find("15,src/main.cpp,"), std::string::npos);
    EXPECT_NE(out.find("5,src/utils.cpp,"), std::string::npos);
    EXPECT_NE(out.find("2,README.md,"), std::string::npos);
}

TEST_F(OutputPrinterTest, OutputTableFormat) {
    OutputOptions opts;
    opts.print_type = PrintType::TABLE;
    opts.stats = sample_stats;
    opts.top = 10;

    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    OutputPrinter printer(opts);
    printer.output();

    std::cout.rdbuf(old_cout);

    std::string out = buffer.str();
    EXPECT_NE(out.find("COUNT   LAST CHANGE"), std::string::npos);
    EXPECT_NE(out.find("Total files analyzed: 3"), std::string::npos);
    EXPECT_NE(out.find("src/main.cpp"), std::string::npos);
}

TEST_F(OutputPrinterTest, OutputHtmlFormat) {
    OutputOptions opts;
    opts.print_type = PrintType::HTML;
    opts.stats = sample_stats;
    opts.top = 2;

    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    OutputPrinter printer(opts);
    printer.output();

    std::cout.rdbuf(old_cout);

    std::string out = buffer.str();
    EXPECT_NE(out.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(out.find("GitHeatMap Activity Report"), std::string::npos);
    EXPECT_NE(out.find("src/main.cpp"), std::string::npos);
    EXPECT_NE(out.find("class=\"bar-fill\""), std::string::npos);
}

TEST_F(OutputPrinterTest, OutputActivityHistogram) {
    OutputOptions opts;
    opts.activity.type = ActivityType::DAY;
    opts.activity.total_commits = 10;
    opts.activity.buckets[0] = 6; // Mon
    opts.activity.buckets[1] = 4; // Tue

    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    OutputPrinter printer(opts);
    printer.output();

    std::cout.rdbuf(old_cout);

    std::string out = buffer.str();
    EXPECT_NE(out.find("Commit Activity Histogram (10 commits total)"), std::string::npos);
    EXPECT_NE(out.find("Mon"), std::string::npos);
    EXPECT_NE(out.find("Tue"), std::string::npos);
    EXPECT_NE(out.find("####"), std::string::npos);
}