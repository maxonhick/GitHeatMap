#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum SortBy {
    COMMIT_COUNT,
    LAST_CHANGE,
    FILE_NAME
};

struct FilterOptions {
    std::string repo_path = ".";
    std::string branch = "";
    bool no_merges = false;
    std::vector<std::string> extensions;
    int64_t since_timestamp = 0;
    int64_t until_timestamp = 0;
    std::vector<std::string> exclude_patterns;
    SortBy sort_by = SortBy::COMMIT_COUNT;
};

struct FileStat {
    std::string path;
    uint64_t commit_count = 0;
    int64_t last_commit_time = 0;
    std::string last_author;
    std::string last_hash;
};

enum PrintType {
    CSV,
    JSON,
    HTML,
    TABLE
};

struct OutputOptions {
    PrintType print_type = PrintType::TABLE;
    std::string output_file = "";
    int top = 10;
    std::vector<FileStat> stats;
};
