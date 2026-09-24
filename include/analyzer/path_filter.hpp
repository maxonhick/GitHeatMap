#pragma once
#include <string>
#include <string_view>
#include <vector>

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

}