#pragma once
#include "analyzer/types.hpp"
#include <vector>

struct AnalysisResult {
    std::vector<FileStat> files;
    ActivityStats activity;
};

class RepoAnalyzer {
public:
    explicit RepoAnalyzer(FilterOptions options);
    
    AnalysisResult analyze();

private:
    FilterOptions options_;
};