#pragma once
#include "analyzer/types.hpp"
#include "libs/json.hpp"
using json = nlohmann::ordered_json;

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

    void output_activity_table(std::ostream& out);

    void output_activity_csv(std::ostream& out);

    void output_activity_json(json& j);

    void to_json(json& j, const FileStat& stat);
};