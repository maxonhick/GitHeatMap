#include "analyzer/repo_analyzer.hpp"
#include "git/git_handle.hpp"

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <string_view>

namespace {

bool matches_extension(std::string_view path, const std::vector<std::string>& extensions) {
    if (extensions.empty()) {
        return true;
    }
    for (const auto& ext : extensions) {
        if (path.size() >= ext.size() &&
            path.compare(path.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

std::string get_short_oid(const git_oid* oid) {
    char buf[GIT_OID_HEXSZ + 1] = {0};
    git_oid_tostr(buf, 8, oid);
    return std::string(buf);
}

}

RepoAnalyzer::RepoAnalyzer(FilterOptions options)
    : options_(std::move(options)) {}

std::vector<FileStat> RepoAnalyzer::analyze() {
    git::GlobalContext git_ctx;

    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, options_.repo_path.c_str()) != 0) {
        const git_error* err = git_error_last();
        std::cerr << "Error opening repository: " << (err ? err->message : "unknown error") << "\n";
        return {};
    }
    git::Repository repo(raw_repo);

    git_revwalk* raw_walker = nullptr;
    if (git_revwalk_new(&raw_walker, repo.get()) != 0) {
        return {};
    }
    git::Revwalk walker(raw_walker);

    git_revwalk_sorting(walker.get(), GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);

    if (options_.branch.empty()) {
        if (git_revwalk_push_head(walker.get()) != 0) {
            std::cerr << "Failed to push HEAD to revwalk\n";
            return {};
        }
    } else {
        git_object* raw_target = nullptr;
        if (git_revparse_single(&raw_target, repo.get(), options_.branch.c_str()) != 0) {
            const git_error* err = git_error_last();
            std::cerr << "Cannot resolve revision or branch '" << options_.branch 
                    << "': " << (err ? err->message : "unknown error") << "\n";
            return {};
        }

        git::Object target_obj(raw_target);

        const git_oid* target_oid = git_object_id(target_obj.get());
        if (git_revwalk_push(walker.get(), target_oid) != 0) {
            std::cerr << "Failed to push target commit to revwalk\n";
            return {};
        }
    }

    std::unordered_map<std::string, FileStat> stats_map;
    git_oid oid;

    while (git_revwalk_next(&oid, walker.get()) == 0) {
        git_commit* raw_commit = nullptr;
        if (git_commit_lookup(&raw_commit, repo.get(), &oid) != 0) {
            continue;
        }
        git::Commit commit(raw_commit);

        unsigned int parent_count = git_commit_parentcount(commit.get());
        if (options_.no_merges && parent_count > 1) {
            continue;
        }

        const git_signature* author = git_commit_author(commit.get());
        std::string author_name = author && author->name ? author->name : "Unknown";
        int64_t commit_time = static_cast<int64_t>(git_commit_time(commit.get()));
        std::string short_hash = get_short_oid(&oid);

        if (options_.since_timestamp > 0 && commit_time < options_.since_timestamp) {
            continue;
        }
        if (options_.until_timestamp > 0 && commit_time > options_.until_timestamp) {
            continue;
        }

        git_tree* raw_tree = nullptr;
        if (git_commit_tree(&raw_tree, commit.get()) != 0) {
            continue;
        }
        git::Tree current_tree(raw_tree);

        git_tree* raw_parent_tree = nullptr;
        git::Tree parent_tree(nullptr);

        if (parent_count > 0) {
            git_commit* raw_parent = nullptr;
            if (git_commit_parent(&raw_parent, commit.get(), 0) == 0) {
                git::Commit parent_commit(raw_parent);
                if (git_commit_tree(&raw_parent_tree, parent_commit.get()) == 0) {
                    parent_tree.reset(raw_parent_tree);
                }
            }
        }

        git_diff* raw_diff = nullptr;
        if (git_diff_tree_to_tree(&raw_diff, repo.get(), parent_tree.get(), current_tree.get(), nullptr) != 0) {
            continue;
        }
        git::Diff diff(raw_diff);

        size_t num_deltas = git_diff_num_deltas(diff.get());
        for (size_t i = 0; i < num_deltas; ++i) {
            const git_diff_delta* delta = git_diff_get_delta(diff.get(), i);
            if (!delta || !delta->new_file.path) {
                continue;
            }

            std::string file_path = delta->new_file.path;

            if (!matches_extension(file_path, options_.extensions)) {
                continue;
            }

            auto& entry = stats_map[file_path];
            if (entry.commit_count == 0) {
                entry.path = file_path;
                entry.last_commit_time = commit_time;
                entry.last_author = author_name;
                entry.last_hash = short_hash;
            }
            entry.commit_count++;
        }
    }

    std::vector<FileStat> results;
    results.reserve(stats_map.size());
    for (auto& [_, stat] : stats_map) {
        results.push_back(std::move(stat));
    }

    switch (options_.sort_by) {
        case SortBy::COMMIT_COUNT:
            std::sort(results.begin(), results.end(), [](const FileStat& a, const FileStat& b) {
                return a.commit_count > b.commit_count;
            });
            break;
        case SortBy::LAST_CHANGE:
            std::sort(results.begin(), results.end(), [](const FileStat& a, const FileStat& b) {
                return a.last_commit_time > b.last_commit_time;
            });
            break;
        case SortBy::FILE_NAME:
            std::sort(results.begin(), results.end(), [](const FileStat& a, const FileStat& b) {
                return a.path < b.path;
            });
            break;
    }
    

    return results;
}