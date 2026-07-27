#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <map>
#include "git2.h"
#include <iomanip>
#include <ctime>
#include <sstream>

struct FileStat {
    int commit_count = 0;
    time_t last_change_time = 0;
    std::string last_commit_hash;
    std::string last_commit_author;
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

int analis(const char *repo_path, time_t since, time_t until, const char *commit_author, bool no_merges) {
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

        if (until != 0 && git_commit_time(commit) > until) {
            continue;
        }

        commit_total_count++;

        const git_signature *author = git_commit_author(commit);
        time_t commit_time = git_commit_time(commit);
        std::string commit_hash = git_oid_tostr_s(&oid);

        if (since != 0 && commit_time < since) {
            git_commit_free(commit);
            break;
        }

        if (commit_author != nullptr && 
            strcmp(commit_author, author->name) != 0 && 
            strcmp(commit_author, author->email) != 0) {
            git_commit_free(commit);
            continue;
        }

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
    commit_total_count--;

    std::cout << "Всего коммитов обработано: " << commit_total_count << std::endl;
    std::cout << "Уникальных файлов затронуто: " << file_stats.size() << "\n" << std::endl;
    
    std::vector<std::pair<std::string, FileStat>> sorted_stats(file_stats.begin(), file_stats.end());
    std::sort(sorted_stats.begin(), sorted_stats.end(),
        [](const auto& a, const auto& b) {
            return a.second.commit_count > b.second.commit_count;
        });
    
    std::cout << "=== Топ-10 самых часто изменяемых файлов ===\n";
    std::cout << std::left << std::setw(9) << "Коммиты"
              << std::setw(60) << "Файл"
              << std::setw(20) << "Последнее изменение"
              << std::endl;
    std::cout << std::string(88, '-') << std::endl;
    
    for (size_t i = 0; i < std::min<size_t>(10, sorted_stats.size()); ++i) {
        const auto& [path, fs] = sorted_stats[i];
        std::cout << std::left << std::setw(8) << fs.commit_count
                  << std::setw(60) << (path.length() > 57 ? path.substr(0, 54) + "..." : path)
                  << std::setw(20) << format_time(fs.last_change_time)
                  << std::endl;
    }
    
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

            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            return 1;
        }
    }

    std::cout << "\n\nРепозиторий: " << repo_path << std::endl;
    
    error = analis(repo_path, since, until, author, no_merges);
    if (error != 0) {
        return error;
    }

    git_libgit2_shutdown();
    return 0;
}