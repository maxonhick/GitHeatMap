#include "analyzer/output_printer.hpp"

#include <iostream>
#include <iomanip>

OutputPrinter::OutputPrinter(OutputOptions options)
    : options_(std::move(options)) {}

void OutputPrinter::output() {
    switch (options_.print_type) {
        case PrintType::TABLE:
            output_table();
            break;
        case PrintType::JSON:
            output_json();
            break;
        case PrintType::CSV:
            output_csv();
            break;
        case PrintType::HTML:
            output_html();
            break;
    }
}

std::string OutputPrinter::format_timestamp(int64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &time);
#else
    gmtime_r(&time, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
    return std::string(buf);
}

void OutputPrinter::output_table() {
    std::cout << "\n"
              << std::left
              << std::setw(8)  << "COUNT"
              << std::setw(20) << "LAST CHANGE"
              << std::setw(12) << "HASH"
              << "FILE\n";
    std::cout << std::string(70, '-') << "\n";

    size_t limit = std::min<size_t>(options_.stats.size(), options_.top);
    for (size_t i = 0; i < limit; ++i) {
        const auto& item = options_.stats[i];
        std::cout << std::left
                  << std::setw(8)  << item.commit_count
                  << std::setw(20) << format_timestamp(item.last_commit_time)
                  << std::setw(12) << item.last_hash
                  << item.path << "\n";
    }

    std::cout << "\nTotal files analyzed: " << options_.stats.size() << "\n";
}

void OutputPrinter::output_json() {
    // TODO
    std::cout << "TODO\n";
}

void OutputPrinter::output_csv() {
    // TODO
    std::cout << "TODO\n";
}

void OutputPrinter::output_html() {
    // TODO
    std::cout << "TODO\n";
}