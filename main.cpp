#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <map>
#include "git2.h"
#include <iomanip>
#include <ctime>
#include <cctype>
#include <fstream>
#include <sstream>

struct FileStat {
    int commit_count = 0;
    time_t last_change_time = 0;
    std::string last_commit_hash;
    std::string last_commit_author;
};

enum class OutPutFormat {
    CSV,
    JSON,
    TABLE,
    HTML
};

std::string format_time(time_t t) {
    std::tm* tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

int diff_callback(
    const git_diff_delta* delta,
    float progress,
    void* payload
) {
    auto* file_stats = static_cast<std::map<std::string, FileStat>*>(payload);

    const char *path = delta->new_file.path;
    if (!path) path = delta->old_file.path;
    if (!path) return 0;

    (*file_stats)[path].commit_count++;

    return 0;
}



/// @brief Checks whether the author fits the filter
/// @param author The introduced filter
/// @param email Email address of the commit author
/// @param name Name of the commit author
/// @return True if author is valid
bool check_author(const std::string &author, const std::string &email, const std::string &name) {
    std::string email_lower = email;
    std::string name_lower = name;
    std::string author_lower = author;
    
    std::transform(email_lower.begin(), email_lower.end(), email_lower.begin(), [](unsigned char c) { return std::tolower(c); });
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), [](unsigned char c) { return std::tolower(c); });
    std::transform(author_lower.begin(), author_lower.end(), author_lower.begin(), [](unsigned char c) { return std::tolower(c); });

    return (email_lower.find(author_lower) != std::string::npos || name_lower.find(author_lower) != std::string::npos);
}

void PrintStats(const std::vector<std::pair<std::string, FileStat>> &file_stats, int top, OutPutFormat format) {
    switch (format) {
        case OutPutFormat::TABLE: {
            std::cout << "=== Топ-" << top << " самых часто изменяемых файлов ===\n";
            std::cout << std::left << std::setw(20) << "Коммиты"
                    << std::setw(60) << "Файл"
                    << std::setw(20) << "Последнее изменение"
                    << std::endl;
            std::cout << std::string(88, '-') << std::endl;
            
            for (size_t i = 0; i < top; ++i) {
                const auto& [path, fs] = file_stats[i];
                std::cout << std::left << std::setw(8) << fs.commit_count
                        << std::setw(60) << (path.length() > 57 ? path.substr(0, 54) + "..." : path)
                        << std::setw(20) << format_time(fs.last_change_time)
                        << std::endl;
            }
            break;
        }
        case OutPutFormat::CSV: {
            std::ofstream file("stats.csv");
            file << "Коммиты,Файл,Последнее изменение\n";
            for (size_t i = 0; i < top; ++i) {
                const auto& [path, fs] = file_stats[i];
                file << fs.commit_count << "," << path << "," << format_time(fs.last_change_time) << "\n";
            }
            std::cout << "=== Статистика сохранена в stats.csv ===\n";
            file.close();
            break;
        }
        case OutPutFormat::JSON: {
            std::ofstream file("stats.json");
            file << "[\n";
            for (size_t i = 0; i < top; ++i) {
                const auto& [path, fs] = file_stats[i];
                file << "    {\n";
                file << "        \"commit_count\": " << fs.commit_count << ",\n";
                file << "        \"path\": \"" << path << "\",\n";
                file << "        \"last_change_time\": \"" << format_time(fs.last_change_time) << "\"\n";
                if (i == top - 1) {
                    file << "    }\n";
                    continue;
                }
                file << "    },\n";
            }
            file << "]\n";
            std::cout << "=== Статистика сохранена в stats.json ===\n";
            file.close();
            break;
        }
        case OutPutFormat::HTML: {
            std::ofstream file("stats.html");
            file << "<table>\n";
            file << "    <tr>\n";
            file << "        <th>Коммиты</th>\n";
            file << "        <th>Файл</th>\n";
            file << "        <th>Последнее изменение</th>\n";
            file << "    </tr>\n";
            for (size_t i = 0; i < top; ++i) {
                const auto& [path, fs] = file_stats[i];
                file << "    <tr>\n";
                file << "        <td>" << fs.commit_count << "</td>\n";
                file << "        <td>" << (path.length() > 57 ? path.substr(0, 54) + "..." : path) << "</td>\n";
                file << "        <td>" << format_time(fs.last_change_time) << "</td>\n";
                file << "    </tr>\n";
            }
            file << "</table>\n";
            std::cout << "=== Статистика сохранена в stats.html ===\n";
            file.close();
            break;
        }
    }
}

int analis(const char *repo_path, time_t since, time_t until, const char *commit_author, bool no_merges, int top, OutPutFormat format) {
    git_repository *repo = nullptr;
    int error = git_repository_open(&repo, repo_path);
    if (error < 0) {
        std::cerr << "Could not open repository: " << repo_path << std::endl;
        return 1;
    }

    git_revwalk *revwalk = nullptr;
    git_commit *commit = nullptr;
    git_revwalk_new(&revwalk, repo);
    git_revwalk_push_head(revwalk);
    git_revwalk_sorting(revwalk, GIT_SORT_TIME);

    std::map<std::string, FileStat> file_stats;

    std::cout << "Анализ коммитов...\n" << std::endl;

    git_oid oid;
    int commit_total_count = 0;

    while (!git_revwalk_next(&oid, revwalk)) {
        error = git_commit_lookup(&commit, repo, &oid);
        if (error < 0) {
            continue;
        }

        const git_signature *author = git_commit_author(commit);
        time_t commit_time = git_commit_time(commit);
        std::string commit_hash = git_oid_tostr_s(&oid);

        if (until != 0 && git_commit_time(commit) > until) {
            continue;
        }

        if (since != 0 && commit_time < since) {
            git_commit_free(commit);
            break;
        }

        if (commit_author != nullptr && !check_author(commit_author, author->email, author->name)) {
            continue;
        }

        commit_total_count++;

        git_tree *tree = nullptr;
        git_tree *parent_tree = nullptr;

        error = git_commit_tree(&tree, commit);
        if (error < 0) {
            git_commit_free(commit);
            continue;
        }

        unsigned int parent_count = git_commit_parentcount(commit);
        git_diff *diff = nullptr;

        if (no_merges && parent_count > 1) {
            git_commit_free(commit);
            continue;
        }

        if (parent_count > 0) {
            git_commit *parent_commit = nullptr;
            error = git_commit_parent(&parent_commit, commit, 0);
            if (error == 0) {
                error = git_commit_tree(&parent_tree, parent_commit);
                if (error == 0) {
                    git_diff_tree_to_tree(&diff, repo, parent_tree, tree, nullptr);
                }
                git_commit_free(parent_commit);
            }
        } else {
            git_diff_tree_to_tree(&diff, repo, nullptr, tree, nullptr);
        }

        if (diff) {
            git_diff_foreach(
                diff,
                diff_callback,
                nullptr,
                nullptr,
                nullptr,
                &file_stats
            );

            size_t num_deltas = git_diff_num_deltas(diff);
            for (size_t i = 0; i < num_deltas; i++) {
                const git_diff_delta* delta = git_diff_get_delta(diff, i);
                const char *path = delta->new_file.path;
                if (!path) path = delta->old_file.path;
                if (!path) continue;

                auto &stat = file_stats[path];
                if (commit_time > stat.last_change_time) {
                    stat.last_change_time = commit_time;
                    stat.last_commit_hash = commit_hash;
                    stat.last_commit_author = author->name;
                }
            }

            git_diff_free(diff);
        }

        if (parent_tree) git_tree_free(parent_tree);
        git_tree_free(tree);
        git_commit_free(commit);
    }

    std::cout << "Всего коммитов обработано: " << commit_total_count << std::endl;
    std::cout << "Уникальных файлов затронуто: " << file_stats.size() << "\n" << std::endl;
    
    std::vector<std::pair<std::string, FileStat>> sorted_stats(file_stats.begin(), file_stats.end());
    std::sort(sorted_stats.begin(), sorted_stats.end(),
        [](const auto& a, const auto& b) {
            return a.second.commit_count > b.second.commit_count;
        });
    
    top = std::min(top, static_cast<int>(sorted_stats.size()));
    PrintStats(sorted_stats, top, format);
    
    git_revwalk_free(revwalk);
    git_repository_free(repo);
    return 0;
}

std::pair<int, time_t> GetTime(std::vector<std::string> &date) {
    switch (date.size()) {
    case 1: // yesterday/today/now or date format YYYY-MM-DD
        {
            if (date[0] == "yesterday") {
                return std::pair(0, (time(nullptr) / 86400 - 1) * 86400);
            }
            if (date[0] == "today") {
                return std::pair(0, time(nullptr) / 86400 * 86400);
            }
            if (date[0] == "now") {
                return std::pair(0, time(nullptr));
            }
            std::tm tmStruct = {};
            std::istringstream ss(date[0]);
            ss >> std::get_time(&tmStruct, "%Y-%m-%d");
            if (ss.fail())
                return std::pair(1, 0);
            return std::pair(0, mktime(&tmStruct));
        }
        break;
    case 2: // last hour, day, week, month, year or date format YYYY-MM-DD HH:MM(:SS)
        {
            if (date[0] == "last") {
                if (date[1] == "hour") {
                    return std::pair(0, time(nullptr) - 3600);
                }
                if (date[1] == "day") {
                    return std::pair(0, time(nullptr) - 86400);
                }
                if (date[1] == "week") {
                    return std::pair(0, time(nullptr) - 604800);
                }
                if (date[1] == "month") {
                    return std::pair(0, time(nullptr) - 2592000);
                }
                if (date[1] == "year") {
                    return std::pair(0, time(nullptr) - 31536000);
                }
                return std::pair(1, 0);
            }
            std::tm tmStruct = {};
            std::istringstream ss(date[0] + " " + date[1]);
            if (std::count(date[1].begin(), date[1].end(), ':') == 1) // date format YYYY-MM-DD HH:MM
                ss >> std::get_time(&tmStruct, "%Y-%m-%d %H:%M");
            else
                ss >> std::get_time(&tmStruct, "%Y-%m-%d %H:%M:%S");
            if (ss.fail())
                return std::pair(1, 0);
            return std::pair(0, mktime(&tmStruct));
        }
        break;
    case 3: // X (hours, days, weeks, months or years) ago
        {
            try {
                if (date[1] == "hour") {
                    return std::pair(0, time(nullptr) - std::stoi(date[0]) * 3600);
                }
                if (date[1] == "day") {
                    return std::pair(0, time(nullptr) / 86400 * 86400 - std::stoi(date[0]) * 86400);
                }
                if (date[1] == "week") {
                    return std::pair(0, time(nullptr) / 86400 * 86400 - std::stoi(date[0]) * 604800);
                }
                if (date[1] == "month") {
                    return std::pair(0, time(nullptr) / 86400 * 86400 - std::stoi(date[0]) * 2592000);
                }
                if (date[1] == "year") {
                    return std::pair(0, time(nullptr) / 86400 * 86400 - std::stoi(date[0]) * 31536000);
                }
                return std::pair(1, 0);
            } catch (...) {
                return std::pair(1, 0);
            }
        }
    }
    return std::pair(1, 0);
}

int main(int argc, char *argv[]) {
    git_libgit2_init();
    int error;

    const char *repo_path = ".";
    time_t since = 0;
    time_t until = 0;
    const char *author = nullptr;
    bool no_merges = false;
    int top = 10;
    OutPutFormat format = OutPutFormat::TABLE;
    if (argc > 1) {
        repo_path = argv[1];
        for (int i = 2; i < argc; i++) {
            if (std::string(argv[i]) == "--since") {
                if (since != 0) {
                    std::cerr << "Since date specified more than once" << std::endl;
                    return 1;
                }
                if (i + 1 >= argc || std::string(argv[i + 1]).substr(0, 2) == "--") {
                    std::cerr << "Missing since date" << std::endl;
                    return 1;
                }
                std::vector<std::string> date;
                while (i + 1 < argc && std::string(argv[i + 1]).substr(0, 2) != "--") {
                    date.push_back(argv[i + 1]);
                    i++;
                }
                std::pair<int, time_t> result = GetTime(date);
                if (result.first != 0) {
                    std::cerr << "Invalid since date" << std::endl;
                    return 1;
                }
                since = result.second;
                std::cout << "Since: " << format_time(since) << std::endl;
                if (since != 0 && until != 0 && until < since) {
                    std::cerr << "Until date must be greater than since date" << std::endl;
                    return 1;
                }
                continue;
            }

            if (std::string(argv[i]) == "--until") {
                if (until != 0) {
                    std::cerr << "Until date specified more than once" << std::endl;
                    return 1;
                }
                if (i + 1 >= argc || std::string(argv[i + 1]).substr(0, 2) == "--") {
                    std::cerr << "Missing until date" << std::endl;
                    return 1;
                }
                std::vector<std::string> date;
                while (i + 1 < argc && std::string(argv[i + 1]).substr(0, 2) != "--") {
                    date.push_back(argv[i + 1]);
                    i++;
                }
                std::pair<int, time_t> result = GetTime(date);
                if (result.first != 0) {
                    std::cerr << "Invalid since date" << std::endl;
                    return 1;
                }
                until = result.second;
                std::cout << "Until: " << format_time(until) << std::endl;
                if (since != 0 && until != 0 && until < since) {
                    std::cerr << "Until date must be greater than since date" << std::endl;
                    return 1;
                }
                continue;
            }

            if (std::string(argv[i]) == "--author") {
                if (author != nullptr) {
                    std::cerr << "Author: Firstname Lastname specified more than once" << std::endl;
                    return 1;
                }
                if (i + 1 >= argc || std::string(argv[i + 1]).substr(0, 2) == "--") {
                    std::cerr << "Missing author" << std::endl;
                    return 1;
                }
                author = argv[i + 1];
                std::cout << "Author: " << author << std::endl;
                i++;
                continue;
            }

            if (std::string(argv[i]) == "--no-merges") {
                no_merges = true;
                std::cout << "No merges" << std::endl;
                continue;
            }

            if (std::string(argv[i]) == "--top") {
                if (i + 1 >= argc || std::string(argv[i + 1]).substr(0, 2) == "--") {
                    std::cerr << "Missing top number" << std::endl;
                    return 1;
                }
                try {
                    top = std::stoi(argv[i + 1]);
                    if (top <= 0) {
                        std::cerr << "Top number must be greater than zero" << std::endl;
                        return 1;
                    }
                } catch (...) {
                    std::cerr << "Invalid top number" << std::endl;
                    return 1;
                }
                std::cout << "Top: " << top << std::endl;
                i++;
                continue;
            }

            if (std::string(argv[i]).size() > 9 && std::string(argv[i]).substr(0, 9) == "--format=") {
                std::string form = std::string(argv[i]).substr(9);
                std::transform(form.begin(), form.end(), form.begin(), ::tolower);

                if (form == "json") {
                    format = OutPutFormat::JSON;
                } else if (form == "csv") {
                    format = OutPutFormat::CSV;
                } else if (form == "table") {
                    format = OutPutFormat::TABLE;
                } else if (form == "html") {
                    format = OutPutFormat::HTML;
                } else {
                    std::cerr << "Invalid format" << std::endl;
                    return 1;
                }
                std::cout << "Format: " << form << std::endl;
                continue;
            }

            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            return 1;
        }
    }

    std::cout << "\n\nРепозиторий: " << repo_path << std::endl;
    
    error = analis(repo_path, since, until, author, no_merges, top, format);
    if (error != 0) {
        return error;
    }

    git_libgit2_shutdown();
    return 0;
}