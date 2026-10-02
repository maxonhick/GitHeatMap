#include <gtest/gtest.h>
#include "analyzer/output_printer.hpp"
#include "analyzer/types.hpp"
#include "libs/json.hpp"
#include <sstream>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

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

    std::string capture_output(OutputPrinter& printer) {
        std::stringstream buffer;
        std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());
        printer.output();
        std::cout.rdbuf(old_cout);
        return buffer.str();
    }
};

// -------------------------------------------------------------
// Tests JSON
// -------------------------------------------------------------

TEST_F(OutputPrinterTest, OutputJsonWithoutActivity) {
    OutputOptions opts;
    opts.print_type = PrintType::JSON;
    opts.stats = sample_stats;
    opts.top = 2;

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    nlohmann::json parsed;
    ASSERT_NO_THROW(parsed = nlohmann::json::parse(out));
    ASSERT_TRUE(parsed.contains("files"));
    EXPECT_FALSE(parsed.contains("activity"));
    EXPECT_EQ(parsed["files"].size(), 2);
    EXPECT_EQ(parsed["files"][0]["path"], "src/main.cpp");
    EXPECT_EQ(parsed["files"][0]["commit_count"], 15);
}

TEST_F(OutputPrinterTest, OutputJsonWithActivity) {
    OutputOptions opts;
    opts.print_type = PrintType::JSON;
    opts.stats = sample_stats;
    opts.top = 1;
    opts.activity.type = ActivityType::DAY;
    opts.activity.total_commits = 20;
    opts.activity.buckets[0] = 12; // Mon
    opts.activity.buckets[4] = 8;  // Fri

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    nlohmann::json parsed;
    ASSERT_NO_THROW(parsed = nlohmann::json::parse(out));
    ASSERT_TRUE(parsed.contains("activity"));
    ASSERT_TRUE(parsed.contains("files"));

    EXPECT_EQ(parsed["activity"]["type"], "wday");
    EXPECT_EQ(parsed["activity"]["total_commits"], 20);
    EXPECT_EQ(parsed["activity"]["buckets"]["Mon"], 12);
    EXPECT_EQ(parsed["activity"]["buckets"]["Fri"], 8);
    EXPECT_EQ(parsed["files"].size(), 1);
}

// -------------------------------------------------------------
// Tests CSV
// -------------------------------------------------------------

TEST_F(OutputPrinterTest, OutputCsvFormatFilesOnly) {
    OutputOptions opts;
    opts.print_type = PrintType::CSV;
    opts.stats = sample_stats;
    opts.top = 0;

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("COUNT,FILE,LAST_CHANGE,HASH"), std::string::npos);
    EXPECT_NE(out.find("15,src/main.cpp,"), std::string::npos);
    EXPECT_NE(out.find("5,src/utils.cpp,"), std::string::npos);
    EXPECT_NE(out.find("2,README.md,"), std::string::npos);
}

TEST_F(OutputPrinterTest, OutputCsvActivityStdout) {
    OutputOptions opts;
    opts.print_type = PrintType::CSV;
    opts.activity.type = ActivityType::YEAR; // Month of year
    opts.activity.total_commits = 10;
    opts.activity.buckets[1] = 4; // Jan
    opts.activity.buckets[5] = 6; // May

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("PERIOD,COUNT"), std::string::npos);
    EXPECT_NE(out.find("Jan,4"), std::string::npos);
    EXPECT_NE(out.find("May,6"), std::string::npos);
}

// -------------------------------------------------------------
// Tests TABLE (Console output)
// -------------------------------------------------------------

TEST_F(OutputPrinterTest, OutputTableFormatFilesOnly) {
    OutputOptions opts;
    opts.print_type = PrintType::TABLE;
    opts.stats = sample_stats;
    opts.top = 10;

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("COUNT   LAST CHANGE"), std::string::npos);
    EXPECT_NE(out.find("Total files analyzed: 3"), std::string::npos);
    EXPECT_NE(out.find("src/main.cpp"), std::string::npos);
}

TEST_F(OutputPrinterTest, OutputTableWithActivityHistogram) {
    OutputOptions opts;
    opts.print_type = PrintType::TABLE;
    opts.stats = sample_stats;
    opts.activity.type = ActivityType::HOUR;
    opts.activity.total_commits = 15;
    opts.activity.buckets[9] = 10;  // 09:00
    opts.activity.buckets[18] = 5;  // 18:00

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("Commit Activity Histogram (15 commits total)"), std::string::npos);
    EXPECT_NE(out.find("09:00 | ######################################## | 10"), std::string::npos);
    EXPECT_NE(out.find("18:00 | ####################                     | 5"), std::string::npos);
    EXPECT_NE(out.find("COUNT   LAST CHANGE"), std::string::npos);
    EXPECT_NE(out.find("src/main.cpp"), std::string::npos);
}

TEST_F(OutputPrinterTest, OutputTableEmptyActivity) {
    OutputOptions opts;
    opts.print_type = PrintType::TABLE;
    opts.activity.type = ActivityType::DAY;
    opts.activity.total_commits = 0;

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("No commits found for activity chart."), std::string::npos);
}

// -------------------------------------------------------------
// Tests HTML
// -------------------------------------------------------------

TEST_F(OutputPrinterTest, OutputHtmlWithoutActivity) {
    OutputOptions opts;
    opts.print_type = PrintType::HTML;
    opts.stats = sample_stats;
    opts.top = 2;

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(out.find("GitHeatMap Activity Report"), std::string::npos);
    EXPECT_NE(out.find("src/main.cpp"), std::string::npos);
    
    EXPECT_EQ(out.find("<div class=\"chart-container\">"), std::string::npos);
    EXPECT_EQ(out.find("class=\"card\""), std::string::npos);
}

TEST_F(OutputPrinterTest, OutputHtmlWithActivityChart) {
    OutputOptions opts;
    opts.print_type = PrintType::HTML;
    opts.stats = sample_stats;
    opts.top = 2;
    opts.activity.type = ActivityType::DAY;
    opts.activity.total_commits = 15;
    opts.activity.buckets[0] = 10; // Mon
    opts.activity.buckets[4] = 5;  // Fri

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("class=\"card\""), std::string::npos);
    EXPECT_NE(out.find("Activity by Day of Week"), std::string::npos);
    EXPECT_NE(out.find("15 commits total"), std::string::npos);
    EXPECT_NE(out.find("class=\"chart-container\""), std::string::npos);
    EXPECT_NE(out.find("Mon: 10 commits"), std::string::npos);
    EXPECT_NE(out.find("Fri: 5 commits"), std::string::npos);
    EXPECT_NE(out.find("src/main.cpp"), std::string::npos);
}

// Checking file save and .activity.csv generation
TEST_F(OutputPrinterTest, OutputCsvWritesToFileAndCreatesActivityFile) {
    fs::path temp_dir = fs::temp_directory_path() / "githeatmap_printer_test";
    fs::create_directories(temp_dir);
    fs::path main_csv = temp_dir / "report.csv";
    fs::path act_csv = temp_dir / "report.csv.activity.csv";

    OutputOptions opts;
    opts.print_type = PrintType::CSV;
    opts.output_file = main_csv.string();
    opts.stats = sample_stats;
    opts.activity.type = ActivityType::DAY;
    opts.activity.total_commits = 5;
    opts.activity.buckets[0] = 5;

    OutputPrinter printer(opts);
    printer.output();

    ASSERT_TRUE(fs::exists(main_csv));
    ASSERT_TRUE(fs::exists(act_csv));

    std::ifstream main_in(main_csv);
    std::string main_content((std::istreambuf_iterator<char>(main_in)), std::istreambuf_iterator<char>());
    EXPECT_NE(main_content.find("COUNT,FILE,LAST_CHANGE,HASH"), std::string::npos);
    EXPECT_NE(main_content.find("src/main.cpp"), std::string::npos);

    std::ifstream act_in(act_csv);
    std::string act_content((std::istreambuf_iterator<char>(act_in)), std::istreambuf_iterator<char>());
    EXPECT_NE(act_content.find("PERIOD,COUNT"), std::string::npos);
    EXPECT_NE(act_content.find("Mon,5"), std::string::npos);

    fs::remove_all(temp_dir);
}

// Checking escape of special characters in HTML
TEST_F(OutputPrinterTest, OutputHtmlEscapesSpecialCharacters) {
    OutputOptions opts;
    opts.print_type = PrintType::HTML;
    opts.stats = {
        {"src/<script>alert(1)</script>.cpp", 1, 1700000000, "Author & Co \"Hack\"", "hash123"}
    };

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_EQ(out.find("<script>"), std::string::npos);
    EXPECT_NE(out.find("&lt;script&gt;alert(1)&lt;/script&gt;.cpp"), std::string::npos);
    EXPECT_NE(out.find("Author &amp; Co &quot;Hack&quot;"), std::string::npos);
}

// Checking the MONTH activity mode (days of the month 1-31)
TEST_F(OutputPrinterTest, OutputActivityDayOfMonth) {
    OutputOptions opts;
    opts.print_type = PrintType::TABLE;
    opts.activity.type = ActivityType::MONTH; // mday: 1-31
    opts.activity.total_commits = 10;
    opts.activity.buckets[1] = 7;
    opts.activity.buckets[31] = 3;

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    EXPECT_NE(out.find("Day  1 |"), std::string::npos);
    EXPECT_NE(out.find("Day 31 |"), std::string::npos);
}

// Checking the behavior of top > list size
TEST_F(OutputPrinterTest, OutputRespectsTopLargerThanStats) {
    OutputOptions opts;
    opts.print_type = PrintType::JSON;
    opts.stats = sample_stats;
    opts.top = 100;

    OutputPrinter printer(opts);
    std::string out = capture_output(printer);

    auto parsed = nlohmann::json::parse(out);
    EXPECT_EQ(parsed["files"].size(), 3);
}
