#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>

enum SortBy {
    COMMIT_COUNT,
    LAST_CHANGE,
    FILE_NAME
};

enum ActivityType{
    NONE,
    HOUR, // hour of day: 0-23
    DAY, // day of week: 0-6
    MONTH, // day of month: 1-31
    YEAR, // month of year: 1-12
};

struct ActivityStats {
    ActivityType type = ActivityType::NONE;
    std::map<int, uint64_t> buckets;
    uint64_t total_commits = 0;
};

struct FilterOptions {
    std::string repo_path = ".";
    std::string branch = "";
    bool no_merges = false;
    std::vector<std::string> extensions;
    int64_t since_timestamp = 0;
    int64_t until_timestamp = 0;
    std::vector<std::string> exclude_patterns;
    std::string author_pattern = "";
    SortBy sort_by = SortBy::COMMIT_COUNT;
    ActivityType activity_type = ActivityType::NONE;
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
    ActivityStats activity;
};
