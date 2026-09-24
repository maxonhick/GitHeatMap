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