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

struct ActivityBarItem {
    std::string label;
    uint64_t count;
};

std::vector<ActivityBarItem> get_ordered_activity_items(const ActivityStats& act) {
    std::vector<ActivityBarItem> items;
    if (act.type == ActivityType::NONE) return items;

    switch (act.type) {
        case ActivityType::HOUR: {
            for (int h = 0; h < 24; ++h) {
                uint64_t count = act.buckets.count(h) ? act.buckets.at(h) : 0;
                char buf[8];
                std::snprintf(buf, sizeof(buf), "%02d", h);
                items.push_back({buf, count});
            }
            break;
        }
        case ActivityType::DAY: {
            static const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            for (int d = 0; d < 7; ++d) {
                uint64_t count = act.buckets.count(d) ? act.buckets.at(d) : 0;
                items.push_back({days[d], count});
            }
            break;
        }
        case ActivityType::MONTH: {
            for (int d = 1; d <= 31; ++d) {
                uint64_t count = act.buckets.count(d) ? act.buckets.at(d) : 0;
                items.push_back({std::to_string(d), count});
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
                items.push_back({months[m - 1], count});
            }
            break;
        }
        default:
            break;
    }
    return items;
}

std::string get_activity_title(ActivityType type) {
    switch (type) {
        case ActivityType::HOUR:  return "Activity by Hour of Day (00:00 - 23:00)";
        case ActivityType::DAY:   return "Activity by Day of Week";
        case ActivityType::MONTH: return "Activity by Day of Month (1 - 31)";
        case ActivityType::YEAR:  return "Activity by Month of Year";
        default:                  return "Commit Activity";
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
        << "      --bar-start: #238636; --bar-end: #388bfd;\n"
        << "    }\n"
        << "    body {\n"
        << "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif;\n"
        << "      background-color: var(--bg); color: var(--text);\n"
        << "      margin: 0; padding: 2rem;\n"
        << "    }\n"
        << "    .container { max-width: 1200px; margin: 0 auto; }\n"
        << "    h1 { margin-bottom: 0.5rem; font-size: 1.8rem; }\n"
        << "    .meta { color: var(--text-muted); margin-bottom: 1.5rem; font-size: 0.95rem; }\n"
        << "\n"
        << "    /* Стили для Activity Chart */\n"
        << "    .card {\n"
        << "      background-color: var(--card-bg); border: 1px solid var(--border);\n"
        << "      border-radius: 6px; padding: 1.5rem; margin-bottom: 2rem;\n"
        << "    }\n"
        << "    .card-title { font-size: 1.1rem; font-weight: 600; margin-bottom: 1.2rem; display: flex; justify-content: space-between; }\n"
        << "    .chart-container {\n"
        << "      display: flex; align-items: flex-end; gap: 6px; height: 160px;\n"
        << "      padding-top: 20px; border-bottom: 1px solid var(--border);\n"
        << "    }\n"
        << "    .chart-col {\n"
        << "      flex: 1; display: flex; flex-direction: column; align-items: center;\n"
        << "      height: 100%; justify-content: flex-end; position: relative;\n"
        << "    }\n"
        << "    .chart-bar {\n"
        << "      width: 100%; min-width: 8px; max-width: 32px;\n"
        << "      background: linear-gradient(180deg, var(--bar-end), var(--bar-start));\n"
        << "      border-radius: 3px 3px 0 0; transition: height 0.3s ease, filter 0.2s;\n"
        << "      position: relative;\n"
        << "    }\n"
        << "    .chart-col:hover .chart-bar { filter: brightness(1.25); cursor: pointer; }\n"
        << "    .chart-label {\n"
        << "      font-size: 0.75rem; color: var(--text-muted); margin-top: 6px;\n"
        << "      white-space: nowrap; overflow: hidden; text-overflow: ellipsis;\n"
        << "    }\n"
        << "    .chart-tooltip {\n"
        << "      visibility: hidden; opacity: 0; position: absolute; bottom: 100%;\n"
        << "      background-color: #21262d; color: var(--text); border: 1px solid var(--border);\n"
        << "      padding: 4px 8px; border-radius: 4px; font-size: 0.75rem; white-space: nowrap;\n"
        << "      transform: translateY(-4px); transition: opacity 0.2s;\n"
        << "      pointer-events: none; z-index: 10;\n"
        << "    }\n"
        << "    .chart-col:hover .chart-tooltip { visibility: visible; opacity: 1; }\n"
        << "\n"
        << "    /* Стили таблицы файлов */\n"
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
        << "    .bar-cell { min-width: 160px; width: 20%; }\n"
        << "    .bar-track {\n"
        << "      background-color: #21262d; border-radius: 4px;\n"
        << "      height: 12px; width: 100%; overflow: hidden; display: flex;\n"
        << "    }\n"
        << "    .bar-fill {\n"
        << "      height: 100%; border-radius: 4px;\n"
        << "      background: linear-gradient(90deg, #238636, #da3633);\n"
        << "    }\n"
        << "  </style>\n"
        << "</head>\n"
        << "<body>\n"
        << "  <div class=\"container\">\n"
        << "    <h1>GitHeatMap Activity Report</h1>\n"
        << "    <div class=\"meta\">Analyzed " << options_.stats.size() << " files"
        << (options_.top > 0 ? " (showing top " + std::to_string(limit) + ")" : "") << "</div>\n";

    if (options_.activity.type != ActivityType::NONE) {
        auto activity_items = get_ordered_activity_items(options_.activity);
        uint64_t max_act_commits = 1;
        for (const auto& item : activity_items) {
            if (item.count > max_act_commits) {
                max_act_commits = item.count;
            }
        }

        out << "    <div class=\"card\">\n"
            << "      <div class=\"card-title\">\n"
            << "        <span>" << get_activity_title(options_.activity.type) << "</span>\n"
            << "        <span style=\"color: var(--text-muted); font-size: 0.9rem;\">"
            << options_.activity.total_commits << " commits total</span>\n"
            << "      </div>\n"
            << "      <div class=\"chart-container\">\n";

        for (const auto& item : activity_items) {
            double height_pct = (static_cast<double>(item.count) / max_act_commits) * 100.0;
            if (item.count > 0 && height_pct < 4.0) {
                height_pct = 4.0;
            }

            out << "        <div class=\"chart-col\">\n"
                << "          <div class=\"chart-tooltip\">" << item.label << ": " << item.count << " commits</div>\n"
                << "          <div class=\"chart-bar\" style=\"height: " << std::fixed << std::setprecision(1) << height_pct << "%;\"></div>\n"
                << "          <span class=\"chart-label\">" << escape_html(item.label) << "</span>\n"
                << "        </div>\n";
        }

        out << "      </div>\n"
            << "    </div>\n";
    }

    out << "    <table>\n"
        << "      <thead>\n"
        << "        <tr>\n"
        << "          <th style=\"width: 70px;\">Commits</th>\n"
        << "          <th class=\"bar-cell\">Heat</th>\n"
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
