#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <regex>
#include <optional>
#include <unordered_map>
#include <stdexcept>

namespace filter {

inline bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool wildcard_match(std::string_view text, std::string_view pattern) {
    size_t t = 0, p = 0;
    size_t star_idx = std::string_view::npos;
    size_t match_idx = 0;

    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
            ++t;
            ++p;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star_idx = p;
            match_idx = t;
            ++p;
        } else if (star_idx != std::string_view::npos) {
            p = star_idx + 1;
            ++match_idx;
            t = match_idx;
        } else {
            return false;
        }
    }

    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }

    return p == pattern.size();
}

inline bool is_excluded(std::string_view path, const std::vector<std::string>& patterns) {
    if (patterns.empty()) {
        return false;
    }

    for (const auto& pattern : patterns) {
        if (wildcard_match(path, pattern)) {
            return true;
        }

        if (!pattern.empty() && pattern.back() == '/' && starts_with(path, pattern)) {
            return true;
        }

        std::string dir_pattern = pattern + "/";
        if (starts_with(path, dir_pattern)) {
            return true;
        }
    }

    return false;
}

inline std::string to_lower(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

inline bool matches_author(std::string_view author, std::string_view pattern) {
    if (pattern.empty()) {
        return true;
    }

    std::string lower_author = to_lower(author);
    std::string lower_pattern = to_lower(pattern);

    if (lower_pattern.find_first_of("*?") != std::string_view::npos) {
        return wildcard_match(lower_author, lower_pattern);
    }

    return lower_author.find(lower_pattern) != std::string::npos;
}

inline std::optional<int64_t> parse_number_or_word(const std::string& token) {
    static const std::unordered_map<std::string, int64_t> word_to_num = {
        {"a", 1}, {"an", 1}, {"one", 1}, {"last", 1},
        {"two", 2}, {"three", 3}, {"four", 4}, {"five", 5},
        {"six", 6}, {"seven", 7}, {"eight", 8}, {"nine", 9}, {"ten", 10}
    };

    auto it = word_to_num.find(token);
    if (it != word_to_num.end()) {
        return it->second;
    }

    if (!token.empty() && std::all_of(token.begin(), token.end(), ::isdigit)) {
        return std::stoll(token);
    }

    return std::nullopt;
}

inline int64_t parse_date_to_timestamp(std::string date_str) {
    if (date_str.empty()) {
        return 0;
    }

    std::transform(date_str.begin(), date_str.end(), date_str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    std::time_t now = std::time(nullptr);

    // Quick key words
    if (date_str == "now") {
        return static_cast<int64_t>(now);
    }
    if (date_str == "today") {
        std::tm tm_buf{};
#if defined(_WIN32)
        gmtime_s(&tm_buf, &now);
#else
        gmtime_r(&now, &tm_buf);
#endif
        tm_buf.tm_hour = 0;
        tm_buf.tm_min = 0;
        tm_buf.tm_sec = 0;
#if defined(_WIN32)
        return static_cast<int64_t>(_mkgmtime(&tm_buf));
#else
        return static_cast<int64_t>(timegm(&tm_buf));
#endif
    }
    if (date_str == "yesterday") {
        return parse_date_to_timestamp("today") - 86400;
    }

    // Absolute format: YYYY-MM-DD
    std::tm tm{};
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (!ss.fail()) {
#if defined(_WIN32)
        return static_cast<int64_t>(_mkgmtime(&tm));
#else
        return static_cast<int64_t>(timegm(&tm));
#endif
    }
    
    // Relative format: "2.weeks", "1.day", "three weeks ago", "10 days"
    // The regular expression captures the number, the delimiter (either a dot or a space), and the time unit.
    static const std::regex rel_regex(R"(^([a-z0-9]+)[.\s]+([a-z]+)(?:\s+ago)?$)");
    std::smatch match;

    if (std::regex_match(date_str, match, rel_regex)) {
        auto maybe_count = parse_number_or_word(match[1].str());
        if (!maybe_count.has_value()) {
            throw std::runtime_error("Unknown number in date: " + match[1].str());
        }

        int64_t count = maybe_count.value();
        std::string unit = match[2].str();

        int64_t multiplier = 0;
        if (unit == "hour" || unit == "hours" || unit == "h") {
            multiplier = 3600;
        } else if (unit == "day" || unit == "days" || unit == "d") {
            multiplier = 86400;
        } else if (unit == "week" || unit == "weeks" || unit == "w") {
            multiplier = 7 * 86400;
        } else if (unit == "month" || unit == "months" || unit == "m") {
            multiplier = 30 * 86400;
        } else if (unit == "year" || unit == "years" || unit == "y") {
            multiplier = 365 * 86400;
        }

        if (multiplier > 0) {
            return static_cast<int64_t>(now) - (count * multiplier);
        }
    }

    throw std::runtime_error("Invalid date format: '" + date_str + "'. Expected YYYY-MM-DD or relative like '2.weeks', 'yesterday'");
}

}