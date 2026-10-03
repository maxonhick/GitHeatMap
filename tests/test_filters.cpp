#include <gtest/gtest.h>
#include "analyzer/filters.hpp"

TEST(FiltersTest, StartsWith) {
    EXPECT_TRUE(filter::starts_with("src/main.cpp", "src/"));
    EXPECT_TRUE(filter::starts_with("src/main.cpp", ""));
    EXPECT_TRUE(filter::starts_with("test", "test"));
    EXPECT_FALSE(filter::starts_with("src/main.cpp", "include/"));
    EXPECT_FALSE(filter::starts_with("short", "longer_prefix"));
}

TEST(FiltersTest, WildcardMatchExactAndSimple) {
    EXPECT_TRUE(filter::wildcard_match("file.txt", "file.txt"));
    EXPECT_FALSE(filter::wildcard_match("file.txt", "file.cpp"));
    EXPECT_TRUE(filter::wildcard_match("file.txt", "file.?xt"));
    EXPECT_FALSE(filter::wildcard_match("file.txt", "file.?x"));
}

TEST(FiltersTest, WildcardMatchAsterisk) {
    EXPECT_TRUE(filter::wildcard_match("main.cpp", "*.cpp"));
    EXPECT_TRUE(filter::wildcard_match("src/module/test.cpp", "*.cpp"));
    EXPECT_TRUE(filter::wildcard_match("src/module/test.cpp", "src/*"));
    EXPECT_TRUE(filter::wildcard_match("abc_123_xyz.log", "abc*xyz.log"));
    EXPECT_TRUE(filter::wildcard_match("anything", "*"));
    EXPECT_TRUE(filter::wildcard_match("", "*"));
    EXPECT_FALSE(filter::wildcard_match("file.h", "*.cpp"));
    EXPECT_FALSE(filter::wildcard_match("dir/file.txt", "file*"));
}

TEST(FiltersTest, IsExcluded) {
    std::vector<std::string> patterns = {
        "build/",
        "docs",
        "*.tmp",
        "vendor/*"
    };

    EXPECT_FALSE(filter::is_excluded("src/main.cpp", {}));
    EXPECT_TRUE(filter::is_excluded("build/out.bin", patterns));
    EXPECT_TRUE(filter::is_excluded("docs/readme.md", patterns));
    EXPECT_TRUE(filter::is_excluded("temp.tmp", patterns));
    EXPECT_TRUE(filter::is_excluded("vendor/lib/header.h", patterns));
    EXPECT_FALSE(filter::is_excluded("src/build_helper.cpp", patterns));
}

TEST(FiltersTest, ToLower) {
    EXPECT_EQ(filter::to_lower("HeLLo_WoRLD 123!"), "hello_world 123!");
    EXPECT_EQ(filter::to_lower(""), "");
}

TEST(FiltersTest, MatchesAuthor) {
    EXPECT_TRUE(filter::matches_author("John Doe", ""));

    EXPECT_TRUE(filter::matches_author("John Doe", "john"));
    EXPECT_TRUE(filter::matches_author("Alice Smith", "SMITH"));
    EXPECT_FALSE(filter::matches_author("Alice Smith", "Bob"));

    EXPECT_TRUE(filter::matches_author("dependabot[bot]", "*bot*"));
    EXPECT_TRUE(filter::matches_author("Developer", "Dev*"));
    EXPECT_FALSE(filter::matches_author("Developer", "*bot"));
}

TEST(DateParserTest, ParsesAbsoluteDate) {
    EXPECT_EQ(filter::parse_date_to_timestamp("2026-01-01"), 1767225600);
}

TEST(DateParserTest, ParsesRelativeUnits) {
    int64_t now = std::time(nullptr);

    EXPECT_NEAR(filter::parse_date_to_timestamp("now"), now, 2);
    EXPECT_NEAR(filter::parse_date_to_timestamp("2.weeks"), now - (14 * 86400), 2);
    EXPECT_NEAR(filter::parse_date_to_timestamp("1.day"), now - 86400, 2);
    EXPECT_NEAR(filter::parse_date_to_timestamp("3 hours ago"), now - (3 * 3600), 2);
    EXPECT_NEAR(filter::parse_date_to_timestamp("a year ago"), now - (365 * 86400), 2);
    EXPECT_NEAR(filter::parse_date_to_timestamp("last month"), now - (30 * 86400), 2);
}

TEST(DateParserTest, ThrowsOnInvalidInput) {
    EXPECT_THROW(filter::parse_date_to_timestamp("not-a-date"), std::runtime_error);
    EXPECT_THROW(filter::parse_date_to_timestamp("2026/05/10"), std::runtime_error);
}

TEST(DateParserTest, ParsesKeywordsTodayAndYesterday) {
    int64_t today = filter::parse_date_to_timestamp("today");
    int64_t yesterday = filter::parse_date_to_timestamp("yesterday");

    EXPECT_GT(today, 0);
    EXPECT_EQ(today - yesterday, 86400);
}

TEST(DateParserTest, ParsesWordsAsNumbers) {
    int64_t now = std::time(nullptr);
    EXPECT_NEAR(filter::parse_date_to_timestamp("two.weeks"), now - (14 * 86400), 2);
    EXPECT_NEAR(filter::parse_date_to_timestamp("three.days"), now - (3 * 86400), 2);
    EXPECT_NEAR(filter::parse_date_to_timestamp("a.month"), now - (30 * 86400), 2);
}
