#pragma once
#include <git2.h>
#include <memory>

namespace git {

template <typename T, void (*Deleter)(T*)>
struct DeleterWrapper {
    void operator()(T* ptr) const {
        if (ptr) Deleter(ptr);
    }
};

template <typename T, void (*Deleter)(T*)>
using Handle = std::unique_ptr<T, DeleterWrapper<T, Deleter>>;

using Repository = Handle<git_repository, git_repository_free>;
using Commit     = Handle<git_commit, git_commit_free>;
using Tree       = Handle<git_tree, git_tree_free>;
using Diff       = Handle<git_diff, git_diff_free>;
using Revwalk    = Handle<git_revwalk, git_revwalk_free>;

struct GlobalContext {
    GlobalContext() { git_libgit2_init(); }
    ~GlobalContext() { git_libgit2_shutdown(); }
};

}