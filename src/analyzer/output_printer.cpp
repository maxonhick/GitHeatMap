#include "analyzer/output_printer.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>

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
    std::ofstream file_out;
    if (!options_.output_file.empty()) {
        file_out.open(options_.output_file);
    }

    std::ostream& out = options_.output_file.empty() ? std::cout : file_out;

    out << "\n"
              << std::left
              << std::setw(8)  << "COUNT"
              << std::setw(20) << "LAST CHANGE"
              << std::setw(12) << "HASH"
              << "FILE\n";
    out << std::string(70, '-') << "\n";

    size_t limit = std::min<size_t>(options_.stats.size(), options_.top);
    for (size_t i = 0; i < limit; ++i) {
        const auto& item = options_.stats[i];
        out << std::left
                  << std::setw(8)  << item.commit_count
                  << std::setw(20) << format_timestamp(item.last_commit_time)
                  << std::setw(12) << item.last_hash
                  << item.path << "\n";
    }

    out << "\nTotal files analyzed: " << options_.stats.size() << "\n";
}

void OutputPrinter::output_json() {
    std::ofstream file_out;
    if (!options_.output_file.empty()) {
        file_out.open(options_.output_file);
    }

    std::ostream& out = options_.output_file.empty() ? std::cout : file_out;
    // TODO
    out << "TODO\n";
}

void OutputPrinter::output_csv() {
    std::ofstream file_out;
    if (!options_.output_file.empty()) {
        file_out.open(options_.output_file);
    }

    std::ostream& out = options_.output_file.empty() ? std::cout : file_out;
    out << "COUNT,FILE,LAST_CHANGE,HASH\n";
    size_t limit = std::min<size_t>(options_.stats.size(), options_.top);
    for (size_t i = 0; i < limit; ++i) {
        out << options_.stats[i].commit_count << ","
            << options_.stats[i].path << ","
            << format_timestamp(options_.stats[i].last_commit_time) << ","
            << options_.stats[i].last_hash << "\n";
    }
}

void OutputPrinter::output_html() {
    std::ofstream file_out;
    if (!options_.output_file.empty()) {
        file_out.open(options_.output_file);
    }

    std::ostream& out = options_.output_file.empty() ? std::cout : file_out;
    // TODO
    out << "TODO\n";
}