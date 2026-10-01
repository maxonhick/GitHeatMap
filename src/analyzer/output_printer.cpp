#include "analyzer/output_printer.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <map>

namespace {

std::string escape_html(std::string_view data) {
    std::string buffer;
    buffer.reserve(data.size());
    for (char c : data) {
        switch (c) {
            case '&':  buffer.append("&amp;");  break;
            case '\"': buffer.append("&quot;"); break;
            case '\'': buffer.append("&apos;"); break;
            case '<':  buffer.append("&lt;");   break;
            case '>':  buffer.append("&gt;");   break;
            default:   buffer.push_back(c);     break;
        }
    }
    return buffer;
}

std::string get_key_for_activity_type(ActivityType type, int value) {
    switch (type) {
        case ActivityType::HOUR:
            return std::to_string(value);
        case ActivityType::DAY: {
            static const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            return days[value];
        }
        case ActivityType::MONTH:
            return std::to_string(value);
        case ActivityType::YEAR: {
            static const char* months[] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };
            return months[value - 1];
        }
        default:
            return "";
    }
}

}

void OutputPrinter::to_json(json& j, const FileStat& stat) {
    j = json {
        {"path", stat.path},
        {"commit_count", stat.commit_count},
        {"last_commit_time", format_timestamp(stat.last_commit_time)},
        {"last_author", stat.last_author},
        {"last_hash", stat.last_hash}
    };
}

OutputPrinter::OutputPrinter(OutputOptions options)
    : options_(std::move(options)) {}

void OutputPrinter::output() {
    if (options_.top == 0 && options_.print_type != PrintType::TABLE) {
        options_.top = options_.stats.size();
    } else if (options_.top == 0 && options_.print_type == PrintType::TABLE) {
        options_.top = 10;
    }

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

void OutputPrinter::output_activity_table(std::ostream& out) {
    const auto& act = options_.activity;

    if (act.total_commits == 0) {
        out << "No commits found for activity chart.\n";
        return;
    }

    uint64_t max_val = 1;
    for (const auto& [_, count] : act.buckets) {
        if (count > max_val) max_val = count;
    }

    const int max_bar_width = 40;

    out << "\nCommit Activity Histogram (" << act.total_commits << " commits total)\n";
    out << std::string(60, '=') << "\n";

    switch (act.type) {
        case ActivityType::HOUR: {
            for (int h = 0; h < 24; ++h) {
                uint64_t count = act.buckets.count(h) ? act.buckets.at(h) : 0;
                int bar_len = static_cast<int>((static_cast<double>(count) / max_val) * max_bar_width);

                out << std::right << std::setw(2) << std::setfill('0') << h << ":00 " << std::setfill(' ')
                    << "| " << std::string(bar_len, '#') << std::string(max_bar_width - bar_len, ' ')
                    << " | " << count << "\n";
            }
            break;
        }

        case ActivityType::DAY: {
            static const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            for (int d = 0; d < 7; ++d) {
                uint64_t count = act.buckets.count(d) ? act.buckets.at(d) : 0;
                int bar_len = static_cast<int>((static_cast<double>(count) / max_val) * max_bar_width);

                out << std::left << std::setw(5) << days[d] << "| "
                    << std::string(bar_len, '#') << std::string(max_bar_width - bar_len, ' ')
                    << " | " << count << "\n";
            }
            break;
        }

        case ActivityType::MONTH: {
            for (int day = 1; day <= 31; ++day) {
                uint64_t count = act.buckets.count(day) ? act.buckets.at(day) : 0;
                int bar_len = static_cast<int>((static_cast<double>(count) / max_val) * max_bar_width);

                out << "Day " << std::right << std::setw(2) << day << " | "
                    << std::string(bar_len, '#') << std::string(max_bar_width - bar_len, ' ')
                    << " | " << count << "\n";
            }
            break;
        }

        case ActivityType::YEAR: {
            static const char* months[] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };
            for (int m = 1; m <= 12; ++m) {
                uint64_t count = act.buckets.count(m) ? act.buckets.at(m) : 0;
                int bar_len = static_cast<int>((static_cast<double>(count) / max_val) * max_bar_width);

                out << std::left << std::setw(5) << months[m - 1] << "| "
                    << std::string(bar_len, '#') << std::string(max_bar_width - bar_len, ' ')
                    << " | " << count << "\n";
            }
            break;
        }

        case ActivityType::NONE:
            break;
    }

    out << std::string(60, '=') << "\n\n";
}

void OutputPrinter::output_activity_csv(std::ostream& out) {
    const auto& act = options_.activity;

    if (act.total_commits == 0) {
        out << "No commits found for activity chart.\n";
        return;
    }

    out << "PERIOD,COUNT\n";
    switch (act.type) {
        case ActivityType::HOUR: {
            for (int h = 0; h < 24; ++h) {
                uint64_t count = act.buckets.count(h) ? act.buckets.at(h) : 0;
                out << h << "," << count << "\n";
            }
            break;
        }

        case ActivityType::DAY: {
            static const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            for (int d = 0; d < 7; ++d) {
                uint64_t count = act.buckets.count(d) ? act.buckets.at(d) : 0;
                out << days[d] << "," << count << "\n";
            }
            break;
        }

        case ActivityType::MONTH: {
            for (int day = 1; day <= 31; ++day) {
                uint64_t count = act.buckets.count(day) ? act.buckets.at(day) : 0;
                out << day << "," << count << "\n";
            }
            break;
        }

        case ActivityType::YEAR: {
            static const char* months[] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };
            for (int m = 1; m <= 12; ++m) {
                uint64_t count = act.buckets.count(m) ? act.buckets.at(m) : 0;
                out << months[m - 1] << "," << count << "\n";
            }
            break;
        }

        case ActivityType::NONE:
            break;
    }

}

void OutputPrinter::output_activity_json(json& activity_obj) {
    const auto& act = options_.activity;
    std::map<ActivityType, std::string> types = {
        {ActivityType::HOUR, "hour"},
        {ActivityType::DAY, "wday"},
        {ActivityType::MONTH, "mday"},
        {ActivityType::YEAR, "month"}
    };

    activity_obj["type"] = types[act.type];
    activity_obj["total_commits"] = act.total_commits;
    json buckets_json = json::object();
    for (const auto& [key, count] : act.buckets) {
        std::string key_str = get_key_for_activity_type(act.type, key);
        buckets_json[key_str] = count;
    }
    activity_obj["buckets"] = buckets_json;
}

void OutputPrinter::output_table() {
    std::ofstream file_out;
    if (!options_.output_file.empty()) {
        file_out.open(options_.output_file);
    }

    std::ostream& out = options_.output_file.empty() ? std::cout : file_out;

    if (options_.activity.type != ActivityType::NONE) {
        output_activity_table(out);
    }

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
    json result = json::object();

    json activity_json = json::object();
    if (options_.activity.type != ActivityType::NONE) {
        output_activity_json(activity_json);
        result["activity"] = activity_json;
    }

    size_t limit = std::min<size_t>(options_.stats.size(), options_.top);
    json files = json::array();
    for (size_t i = 0; i < limit; ++i) {
        json j = json::object();
        to_json(j, options_.stats[i]);
        files.push_back(j);
    }
    result["files"] = files;

    out << result.dump(4) << "\n";
}

void OutputPrinter::output_csv() {
    std::ofstream file_out;
    if (!options_.output_file.empty()) {
        file_out.open(options_.output_file);
    }

    if (options_.activity.type != ActivityType::NONE) {
        std::ofstream file_activity_out;
        if (!options_.output_file.empty()) {
            file_activity_out.open(options_.output_file + ".activity.csv");
        }
        std::ostream& out = options_.output_file.empty() ? std::cout : file_activity_out;
        output_activity_csv(out);
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
    size_t limit = std::min<size_t>(options_.stats.size(), options_.top);
    uint64_t max_commits = 1;
    for (size_t i = 0; i < limit; ++i) {
        if (options_.stats[i].commit_count > max_commits) {
            max_commits = options_.stats[i].commit_count;
        }
    }

    out << "<!DOCTYPE html>\n"
        << "<html lang=\"en\">\n"
        << "<head>\n"
        << "  <meta charset=\"UTF-8\">\n"
        << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        << "  <title>GitHeatMap Report</title>\n"
        << "  <style>\n"
        << "    :root {\n"
        << "      --bg: #0d1117; --card-bg: #161b22; --border: #30363d;\n"
        << "      --text: #c9d1d9; --text-muted: #8b949e; --accent: #58a6ff;\n"
        << "      --bar-start: #238636; --bar-end: #da3633;\n"
        << "    }\n"
        << "    body {\n"
        << "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif;\n"
        << "      background-color: var(--bg); color: var(--text);\n"
        << "      margin: 0; padding: 2rem;\n"
        << "    }\n"
        << "    .container { max-width: 1200px; margin: 0 auto; }\n"
        << "    h1 { margin-bottom: 0.5rem; font-size: 1.8rem; }\n"
        << "    .meta { color: var(--text-muted); margin-bottom: 1.5rem; font-size: 0.95rem; }\n"
        << "    table {\n"
        << "      width: 100%; border-collapse: collapse;\n"
        << "      background-color: var(--card-bg); border-radius: 6px;\n"
        << "      overflow: hidden; border: 1px solid var(--border);\n"
        << "    }\n"
        << "    th, td { padding: 10px 14px; text-align: left; border-bottom: 1px solid var(--border); }\n"
        << "    th { background-color: #21262d; font-size: 0.85rem; color: var(--text-muted); text-transform: uppercase; }\n"
        << "    tr:last-child td { border-bottom: none; }\n"
        << "    tr:hover td { background-color: rgba(110, 118, 129, 0.08); }\n"
        << "    .path { font-family: monospace; font-size: 0.9rem; word-break: break-all; }\n"
        << "    .hash { font-family: monospace; font-size: 0.85rem; color: var(--accent); }\n"
        << "    .bar-cell { min-width: 180px; width: 25%; }\n"
        << "    .bar-track {\n"
        << "      background-color: #21262d; border-radius: 4px;\n"
        << "      height: 14px; width: 100%; overflow: hidden; display: flex;\n"
        << "    }\n"
        << "    .bar-fill {\n"
        << "      height: 100%; border-radius: 4px;\n"
        << "      background: linear-gradient(90deg, var(--bar-start), var(--bar-end));\n"
        << "    }\n"
        << "  </style>\n"
        << "</head>\n"
        << "<body>\n"
        << "  <div class=\"container\">\n"
        << "    <h1>GitHeatMap Activity Report</h1>\n"
        << "    <div class=\"meta\">Analyzed " << options_.stats.size() << " files (showing top " << limit << ")</div>\n"
        << "    <table>\n"
        << "      <thead>\n"
        << "        <tr>\n"
        << "          <th style=\"width: 80px;\">Commits</th>\n"
        << "          <th class=\"bar-cell\">Activity</th>\n"
        << "          <th>File Path</th>\n"
        << "          <th>Last Change</th>\n"
        << "          <th>Author</th>\n"
        << "          <th>Commit</th>\n"
        << "        </tr>\n"
        << "      </thead>\n"
        << "      <tbody>\n";

    for (size_t i = 0; i < limit; ++i) {
        const auto& item = options_.stats[i];
        double pct = (static_cast<double>(item.commit_count) / max_commits) * 100.0;

        out << "        <tr>\n"
            << "          <td><strong>" << item.commit_count << "</strong></td>\n"
            << "          <td class=\"bar-cell\">\n"
            << "            <div class=\"bar-track\">\n"
            << "              <div class=\"bar-fill\" style=\"width: " << std::fixed << std::setprecision(1) << pct << "%;\"></div>\n"
            << "            </div>\n"
            << "          </td>\n"
            << "          <td class=\"path\">" << escape_html(item.path) << "</td>\n"
            << "          <td>" << format_timestamp(item.last_commit_time) << "</td>\n"
            << "          <td>" << escape_html(item.last_author) << "</td>\n"
            << "          <td class=\"hash\">" << escape_html(item.last_hash) << "</td>\n"
            << "        </tr>\n";
    }

    out << "      </tbody>\n"
        << "    </table>\n"
        << "  </div>\n"
        << "</body>\n"
        << "</html>\n";
}