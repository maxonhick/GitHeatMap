#pragma once
#include "analyzer/types.hpp"
#include <vector>

class RepoAnalyzer {
public:
    explicit RepoAnalyzer(FilterOptions options);
    
    std::vector<FileStat> analyze();

private:
    FilterOptions options_;
};