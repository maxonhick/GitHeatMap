#pragma once
#include "analyzer/types.hpp"

class OutputPrinter {
public:
    explicit OutputPrinter(OutputOptions options);
    
    void output();

private:
    OutputOptions options_;

    std::string format_timestamp(int64_t timestamp);

    void output_table();

    void output_json();

    void output_csv();

    void output_html();
};