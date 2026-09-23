#include "analyzer/repo_analyzer.hpp"
#include "analyzer/output_printer.hpp"
#include "libs/CLI11.hpp"

#include <chrono>
#include <iostream>

namespace {

int64_t parse_date_to_timestamp(const std::string& date_str) {
    if (date_str.empty()) {
        return 0;
    }
    std::tm tm{};
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        throw CLI::ValidationError("Date", "Invalid date format. Expected YYYY-MM-DD");
    }
#if defined(_WIN32)
    return static_cast<int64_t>(_mkgmtime(&tm));
#else
    return static_cast<int64_t>(timegm(&tm));
#endif
}

}

int main(int argc, char* argv[]) {
    CLI::App app{"GitHeatMap Repo Analyzer"};

    FilterOptions opts;
    OutputOptions out_opts;
    std::string since_str, until_str;

    app.add_option("repo", opts.repo_path, "The path to the git repository to analyze")
        ->default_val(".")
        ->check(CLI::ExistingDirectory);
    app.add_option("-b,--branch", opts.branch, "The branch to analyze");
    app.add_flag("--no-merges", opts.no_merges, "Do not include merge commits");
    app.add_option("-e,--extensions", opts.extensions, "The file extensions to include (e.g. '.md', '.py')")
        ->delimiter(',');
    app.add_option("--since", since_str, "The timestamp to start from");
    app.add_option("--until", until_str, "The timestamp to stop at");

    app.add_option("--exclude-patterns", opts.exclude_patterns, "The file patterns to exclude")
        ->delimiter(',');
    
    const std::map<std::string, SortBy> sort_map = {
        {"count", SortBy::COMMIT_COUNT},
        {"lastchange", SortBy::LAST_CHANGE},
        {"file", SortBy::FILE_NAME}
    };
    app.add_option("--sort", opts.sort_by, "Sort results by")
        ->transform(CLI::CheckedTransformer(sort_map, CLI::ignore_case))
        ->default_val(SortBy::COMMIT_COUNT);

    app.add_option("-n,--top", out_opts.top, "The number of results to return")
        ->default_val(10);

    const std::map<std::string, PrintType> print_map = {
        {"csv", PrintType::CSV},
        {"json", PrintType::JSON},
        {"html", PrintType::HTML},
        {"table", PrintType::TABLE}
    };
    app.add_option("--format", out_opts.print_type, "The output format")
        ->transform(CLI::CheckedTransformer(print_map, CLI::ignore_case))
        ->default_val(PrintType::TABLE);
    app.add_option("--output", out_opts.output_file, "The output file")->default_val("");

    CLI11_PARSE(app, argc, argv);

    try {
        opts.since_timestamp = parse_date_to_timestamp(since_str);
        opts.until_timestamp = parse_date_to_timestamp(until_str);
    } catch (const CLI::ValidationError& e) {
        std::cerr << "Argument error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Analyzing repository at: " << opts.repo_path << " ...\n";

    RepoAnalyzer analyzer(opts);
    out_opts.stats = analyzer.analyze();

    if (out_opts.stats.empty()) {
        std::cout << "No commits or files found matching criteria.\n";
        return 0;
    }

    OutputPrinter printer(out_opts);
    printer.output();
    
    return 0;
}
