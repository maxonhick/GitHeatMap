#include <gtest/gtest.h>
#include <git2.h>
#include <filesystem>
#include <fstream>
#include "analyzer/repo_analyzer.hpp"
#include "git/git_handle.hpp"

namespace fs = std::filesystem;

class RepoAnalyzerTest : public ::testing::Test {
protected:
    fs::path repo_dir;
    git::GlobalContext ctx;

    void SetUp() override {
        repo_dir = fs::temp_directory_path() / ("githeatmap_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(repo_dir);

        git_repository* repo_ptr = nullptr;
        ASSERT_EQ(git_repository_init(&repo_ptr, repo_dir.string().c_str(), 0), 0);
        git::Repository repo(repo_ptr);

        writeFile("file1.cpp", "int main() { return 0; }");
        writeFile("file2.md", "# Hello");
        auto c1 = createCommit(repo.get(), {"file1.cpp", "file2.md"}, "Alice", "alice@example.com", "Initial commit", 1600000000, {});

        writeFile("file1.cpp", "int main() { return 1; }");
        writeFile("doc.txt", "Some docs");
        auto c2 = createCommit(repo.get(), {"file1.cpp", "doc.txt"}, "Bob", "bob@example.com", "Bob commit", 1650000000, {c1});

        writeFile("file1.cpp", "int main() { return 2; }");
        createCommit(repo.get(), {"file1.cpp"}, "Charlie", "charlie@example.com", "Charlie commit", 1700000000, {c2});
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(repo_dir, ec);
    }

    void writeFile(const std::string& rel_path, const std::string& content) {
        fs::path p = repo_dir / rel_path;
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }
        std::ofstream ofs(p);
        ofs << content;
    }

    git_oid createCommit(git_repository* repo, const std::vector<std::string>& files,
                         const char* author_name, const char* author_email,
                         const char* message, git_time_t commit_time,
                         const std::vector<git_oid>& parents) {
        git_index* index_ptr = nullptr;
        git_repository_index(&index_ptr, repo);
        git::Handle<git_index, git_index_free> index(index_ptr);

        for (const auto& f : files) {
            git_index_add_bypath(index.get(), f.c_str());
        }
        git_index_write(index.get());

        git_oid tree_id;
        git_index_write_tree(&tree_id, index.get());

        git_tree* tree_ptr = nullptr;
        git_tree_lookup(&tree_ptr, repo, &tree_id);
        git::Tree tree(tree_ptr);

        git_signature* sig = nullptr;
        git_signature_new(&sig, author_name, author_email, commit_time, 0);
        git::Handle<git_signature, git_signature_free> signature(sig);

        std::vector<const git_commit*> parent_commits;
        for (const auto& pid : parents) {
            git_commit* pc = nullptr;
            git_commit_lookup(&pc, repo, &pid);
            parent_commits.push_back(pc);
        }

        git_oid commit_id;
        git_commit_create_v(&commit_id, repo, "HEAD", signature.get(), signature.get(),
                            nullptr, message, tree.get(), parent_commits.size(),
                            parent_commits.empty() ? nullptr : parent_commits[0]);

        for (auto pc : parent_commits) {
            git_commit_free(const_cast<git_commit*>(pc));
        }

        return commit_id;
    }
};

TEST_F(RepoAnalyzerTest, CountsCommitsAndIdentifiesLastCommit) {
    FilterOptions opts;
    opts.repo_path = repo_dir.string();
    opts.sort_by = SortBy::COMMIT_COUNT;

    RepoAnalyzer analyzer(opts);
    auto stats = analyzer.analyze();

    ASSERT_EQ(stats.size(), 3);
    EXPECT_EQ(stats[0].path, "file1.cpp");
    EXPECT_EQ(stats[0].commit_count, 3);
    EXPECT_EQ(stats[0].last_author, "Charlie");
    EXPECT_EQ(stats[0].last_commit_time, 1700000000);
}

TEST_F(RepoAnalyzerTest, FilterByExtension) {
    FilterOptions opts;
    opts.repo_path = repo_dir.string();
    opts.extensions = {".md"};

    RepoAnalyzer analyzer(opts);
    auto stats = analyzer.analyze();

    ASSERT_EQ(stats.size(), 1);
    EXPECT_EQ(stats[0].path, "file2.md");
}

TEST_F(RepoAnalyzerTest, FilterByAuthor) {
    FilterOptions opts;
    opts.repo_path = repo_dir.string();
    opts.author_pattern = "Bob";

    RepoAnalyzer analyzer(opts);
    auto stats = analyzer.analyze();

    ASSERT_EQ(stats.size(), 2);
    for (const auto& item : stats) {
        EXPECT_EQ(item.last_author, "Bob");
    }
}

TEST_F(RepoAnalyzerTest, FilterByTimestampSinceUntil) {
    FilterOptions opts;
    opts.repo_path = repo_dir.string();
    opts.since_timestamp = 1640000000;
    opts.until_timestamp = 1660000000;

    RepoAnalyzer analyzer(opts);
    auto stats = analyzer.analyze();

    ASSERT_EQ(stats.size(), 2);
    for (const auto& item : stats) {
        EXPECT_EQ(item.last_commit_time, 1650000000);
    }
}

TEST_F(RepoAnalyzerTest, ExcludePatterns) {
    FilterOptions opts;
    opts.repo_path = repo_dir.string();
    opts.exclude_patterns = {"*.cpp", "*.txt"};

    RepoAnalyzer analyzer(opts);
    auto stats = analyzer.analyze();

    ASSERT_EQ(stats.size(), 1);
    EXPECT_EQ(stats[0].path, "file2.md");
}