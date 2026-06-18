#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "../../fs_generator/generated_fs.hpp"
#include "../../path_generator/path_generator.hpp"
#include "../zipf_distribution.hpp"

using GeneratedFs = fsgenerator::GeneratedFs;
using TreeNode = GeneratedFs::tree_node_type;
using path_type = GeneratedFs::path_type;

using pathgen::UniformPathGenerator;
using pathgen::ZipfPathGenerator;

GeneratedFs make_small_fs() {
    std::vector<TreeNode> nodes;

    nodes.push_back(TreeNode{
        .name = "",
        .parent = 0,
        .depth = 0,
        .children = 3,
        .is_file = false,
        .is_root = true,
    });

    nodes.push_back(TreeNode{
        .name = "a",
        .parent = 0,
        .depth = 1,
        .children = 2,
        .is_file = false,
        .is_root = false,
    });

    nodes.push_back(TreeNode{
        .name = "b",
        .parent = 0,
        .depth = 1,
        .children = 1,
        .is_file = false,
        .is_root = false,
    });

    nodes.push_back(TreeNode{
        .name = "root.txt",
        .parent = 0,
        .depth = 1,
        .children = 0,
        .is_file = true,
        .is_root = false,
    });

    nodes.push_back(TreeNode{
        .name = "a1.txt",
        .parent = 1,
        .depth = 2,
        .children = 0,
        .is_file = true,
        .is_root = false,
    });

    nodes.push_back(TreeNode{
        .name = "a2.txt",
        .parent = 1,
        .depth = 2,
        .children = 0,
        .is_file = true,
        .is_root = false,
    });

    nodes.push_back(TreeNode{
        .name = "b1.txt",
        .parent = 2,
        .depth = 2,
        .children = 0,
        .is_file = true,
        .is_root = false,
    });

    return GeneratedFs{std::move(nodes)};
}

GeneratedFs make_no_files_fs() {
    std::vector<TreeNode> nodes;

    nodes.push_back(TreeNode{
        .name = "",
        .parent = 0,
        .depth = 0,
        .children = 2,
        .is_file = false,
        .is_root = true,
    });

    nodes.push_back(TreeNode{
        .name = "a",
        .parent = 0,
        .depth = 1,
        .children = 1,
        .is_file = false,
        .is_root = false,
    });

    nodes.push_back(TreeNode{
        .name = "b",
        .parent = 1,
        .depth = 2,
        .children = 0,
        .is_file = false,
        .is_root = false,
    });

    return GeneratedFs{std::move(nodes)};
}

GeneratedFs make_only_root_fs() {
    std::vector<TreeNode> nodes;

    nodes.push_back(TreeNode{
        .name = "",
        .parent = 0,
        .depth = 0,
        .children = 0,
        .is_file = false,
        .is_root = true,
    });

    return GeneratedFs{std::move(nodes)};
}

std::unordered_set<path_type> collect_file_paths(const GeneratedFs& fs) {
    std::unordered_set<path_type> result;

    for (size_t id : fs.get_files()) {
        result.insert(fs.get_path(id));
    }

    return result;
}

std::unordered_set<path_type> collect_dir_paths(const GeneratedFs& fs) {
    std::unordered_set<path_type> result;

    for (size_t id : fs.get_dirs()) {
        result.insert(fs.get_path(id));
    }

    return result;
}

bool contains(const std::unordered_set<path_type>& set, const path_type& path) {
    return set.find(path) != set.end();
}

bool is_valid_absolute_path(std::string_view path) {
    if (path.empty() || path.front() != '/') {
        return false;
    }

    if (path.size() > 1 && path.back() == '/') {
        return false;
    }

    return path.find("//") == std::string_view::npos;
}

bool ends_with(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size()
        && str.substr(str.size() - suffix.size()) == suffix;
}

path_type parent_path(path_type path) {
    if (path == "/") {
        return "/";
    }

    const size_t pos = path.find_last_of('/');
    if (pos == 0) {
        return "/";
    }

    return path.substr(0, pos);
}

path_type filename(path_type path) {
    const size_t pos = path.find_last_of('/');
    return path.substr(pos + 1);
}

size_t expected_result_size(const std::vector<OperationType>& ops) {
    size_t result = 0;

    for (OperationType op : ops) {
        result += (op == OperationType::Move ? 2 : 1);
    }

    return result;
}

template <typename Generator>
std::vector<path_type> generate_many(Generator& gen, OperationType op, size_t count) {
    return gen.generate(std::vector<OperationType>(count, op));
}



TEST(ZipfDistribution, ThrowsOnInvalidRange) {
    EXPECT_THROW(pathgen::ZipfDistribution<int>(10, 5, 1.5), std::invalid_argument);
}

TEST(ZipfDistribution, ThrowsOnNonPositiveS) {
    EXPECT_THROW(pathgen::ZipfDistribution<int>(1, 10, 0.0), std::invalid_argument);
    EXPECT_THROW(pathgen::ZipfDistribution<int>(1, 10, -1.0), std::invalid_argument);
}

TEST(ZipfDistribution, SingleValueRangeAlwaysReturnsThatValue) {
    std::mt19937 gen(42);
    pathgen::ZipfDistribution<int> dist(7, 7, 1.5);

    for (int i = 0; i < 10000; ++i) {
        EXPECT_EQ(dist(gen), 7);
    }
}

TEST(ZipfDistribution, ValuesAreInsideRange) {
    std::mt19937 gen(42);
    pathgen::ZipfDistribution<int> dist(5, 20, 1.5);

    for (int i = 0; i < 100000; ++i) {
        const int value = dist(gen);
        EXPECT_GE(value, 5);
        EXPECT_LE(value, 20);
    }
}

TEST(ZipfDistribution, LowerRanksAreMoreFrequentThanUpperRanks) {
    std::mt19937 gen(42);
    pathgen::ZipfDistribution<int> dist(1, 100, 1.5);

    int first_ten = 0;
    int last_ten = 0;

    for (int i = 0; i < 200000; ++i) {
        const int value = dist(gen);
        if (value <= 10) {
            ++first_ten;
        }
        if (value > 90) {
            ++last_ten;
        }
    }

    EXPECT_GT(first_ten, last_ten * 5);
}

TEST(ZipfDistribution, LargerSIsMoreConcentratedAtBeginning) {
    constexpr int samples = 200000;

    std::mt19937 gen1(42);
    std::mt19937 gen2(42);

    pathgen::ZipfDistribution<int> weak_zipf(1, 100, 0.7);
    pathgen::ZipfDistribution<int> strong_zipf(1, 100, 2.0);

    int weak_first_ten = 0;
    int strong_first_ten = 0;

    for (int i = 0; i < samples; ++i) {
        if (weak_zipf(gen1) <= 10) {
            ++weak_first_ten;
        }

        if (strong_zipf(gen2) <= 10) {
            ++strong_first_ten;
        }
    }

    EXPECT_GT(strong_first_ten, weak_first_ten);
}

TEST(PathGenerator, UniformThrowsWhenGeneratedFsHasNoFiles) {
    auto fs = make_no_files_fs();
    EXPECT_THROW(UniformPathGenerator gen(fs, 0.0), std::invalid_argument);
}

TEST(PathGenerator, UniformThrowsWhenGeneratedFsHasOnlyRoot) {
    auto fs = make_only_root_fs();
    EXPECT_THROW(UniformPathGenerator gen(fs, 0.0), std::invalid_argument);
}

TEST(PathGenerator, UniformThrowsOnNegativeLocality) {
    auto fs = make_small_fs();
    EXPECT_THROW(UniformPathGenerator gen(fs, -0.1), std::invalid_argument);
}

TEST(PathGenerator, UniformThrowsOnLocalityOneOrGreater) {
    auto fs = make_small_fs();

    EXPECT_THROW(UniformPathGenerator gen(fs, 1.0), std::invalid_argument);
    EXPECT_THROW(UniformPathGenerator gen(fs, 2.0), std::invalid_argument);
}

TEST(PathGenerator, UniformAcceptsValidLocalityBounds) {
    auto fs = make_small_fs();

    EXPECT_NO_THROW(UniformPathGenerator gen(fs, 0.0));
    EXPECT_NO_THROW(UniformPathGenerator gen(fs, 0.999));
}

TEST(PathGenerator, ZipfThrowsOnNonPositiveS) {
    auto fs = make_small_fs();

    EXPECT_THROW(ZipfPathGenerator gen(fs, 0.0, 0.0), std::invalid_argument);
    EXPECT_THROW(ZipfPathGenerator gen(fs, -1.0, 0.0), std::invalid_argument);
}

TEST(PathGenerator, ZipfAcceptsPositiveS) {
    auto fs = make_small_fs();
    EXPECT_NO_THROW(ZipfPathGenerator gen(fs, 1.5, 0.0));
}

TEST(PathGenerator, EmptyOperationSequenceProducesEmptyPathSequence) {
    auto fs = make_small_fs();
    UniformPathGenerator gen(fs, 0.0);

    const auto paths = gen.generate({});

    EXPECT_TRUE(paths.empty());
}

TEST(PathGenerator, ReadGeneratesExistingFilePaths) {
    auto fs = make_small_fs();
    const auto valid_files = collect_file_paths(fs);
    UniformPathGenerator gen(fs, 0.0);

    const auto paths = generate_many(gen, OperationType::Read, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& path : paths) {
        EXPECT_TRUE(is_valid_absolute_path(path)) << path;
        EXPECT_TRUE(contains(valid_files, path)) << path;
    }
}

TEST(PathGenerator, WriteGeneratesExistingFilePaths) {
    auto fs = make_small_fs();
    const auto valid_files = collect_file_paths(fs);
    UniformPathGenerator gen(fs, 0.0);

    const auto paths = generate_many(gen, OperationType::Write, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& path : paths) {
        EXPECT_TRUE(is_valid_absolute_path(path)) << path;
        EXPECT_TRUE(contains(valid_files, path)) << path;
    }
}

TEST(PathGenerator, LsGeneratesExistingDirectoryPaths) {
    auto fs = make_small_fs();
    const auto valid_dirs = collect_dir_paths(fs);
    UniformPathGenerator gen(fs, 0.0);

    const auto paths = generate_many(gen, OperationType::Ls, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& path : paths) {
        EXPECT_TRUE(is_valid_absolute_path(path)) << path;
        EXPECT_TRUE(contains(valid_dirs, path)) << path;
    }
}

TEST(PathGenerator, MkdirGeneratesFreshDirectoryNamesUnderExistingDirectories) {
    auto fs = make_small_fs();
    const auto valid_dirs = collect_dir_paths(fs);
    UniformPathGenerator gen(fs, 0.0);

    const auto paths = generate_many(gen, OperationType::Mkdir, 100);

    ASSERT_EQ(paths.size(), 100u);
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& path = paths[i];

        EXPECT_TRUE(is_valid_absolute_path(path)) << path;
        EXPECT_TRUE(contains(valid_dirs, parent_path(path))) << path;
        EXPECT_EQ(filename(path), "new_dir" + std::to_string(i)) << path;
    }
}

TEST(PathGenerator, MoveGeneratesSourceFileAndDestinationUnderExistingDirectory) {
    auto fs = make_small_fs();
    const auto valid_files = collect_file_paths(fs);
    const auto valid_dirs = collect_dir_paths(fs);
    UniformPathGenerator gen(fs, 0.0);

    const auto paths = generate_many(gen, OperationType::Move, 500);

    ASSERT_EQ(paths.size(), 1000u);
    for (size_t i = 0; i < paths.size(); i += 2) {
        const path_type& source = paths[i];
        const path_type& destination = paths[i + 1];

        EXPECT_TRUE(contains(valid_files, source)) << source;
        EXPECT_TRUE(is_valid_absolute_path(destination)) << destination;
        EXPECT_TRUE(contains(valid_dirs, parent_path(destination))) << destination;
        EXPECT_TRUE(ends_with(filename(destination), "_new")) << destination;
    }
}

TEST(PathGenerator, MixedOperationSequenceHasExpectedResultSize) {
    auto fs = make_small_fs();
    UniformPathGenerator gen(fs, 0.0);

    const std::vector<OperationType> ops = {
        OperationType::Read,
        OperationType::Write,
        OperationType::Mkdir,
        OperationType::Ls,
        OperationType::Move,
        OperationType::Find,
        OperationType::Move,
    };

    const auto paths = gen.generate(ops);

    EXPECT_EQ(paths.size(), expected_result_size(ops));
}

TEST(PathGenerator, HighLocalityStillGeneratesOnlyValidUniformPaths) {
    auto fs = make_small_fs();
    const auto valid_files = collect_file_paths(fs);
    UniformPathGenerator gen(fs, 0.95);

    const auto paths = generate_many(gen, OperationType::Read, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& path : paths) {
        EXPECT_TRUE(contains(valid_files, path)) << path;
    }
}

TEST(PathGenerator, ZipfReadGeneratesExistingFilePaths) {
    auto fs = make_small_fs();
    const auto valid_files = collect_file_paths(fs);
    ZipfPathGenerator gen(fs, 1.5, 0.0);

    const auto paths = generate_many(gen, OperationType::Read, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& path : paths) {
        EXPECT_TRUE(is_valid_absolute_path(path)) << path;
        EXPECT_TRUE(contains(valid_files, path)) << path;
    }
}

TEST(PathGenerator, ZipfLsGeneratesExistingDirectoryPaths) {
    auto fs = make_small_fs();
    const auto valid_dirs = collect_dir_paths(fs);
    ZipfPathGenerator gen(fs, 1.5, 0.0);

    const auto paths = generate_many(gen, OperationType::Ls, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& path : paths) {
        EXPECT_TRUE(is_valid_absolute_path(path)) << path;
        EXPECT_TRUE(contains(valid_dirs, path)) << path;
    }
}

TEST(PathGenerator, ZipfFindGeneratesPatternsInsideExistingDirectories) {
    auto fs = make_small_fs();
    const auto valid_dirs = collect_dir_paths(fs);
    ZipfPathGenerator gen(fs, 1.5, 0.0);

    const auto paths = generate_many(gen, OperationType::Find, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& pattern : paths) {
        ASSERT_TRUE(ends_with(pattern, "/*")) << pattern;

        const path_type dir = pattern.substr(0, pattern.size() - 2);
        EXPECT_TRUE(contains(valid_dirs, dir)) << pattern;
    }
}

TEST(PathGenerator, ZipfMoveGeneratesSourceFileAndDestinationUnderExistingDirectory) {
    auto fs = make_small_fs();
    const auto valid_files = collect_file_paths(fs);
    const auto valid_dirs = collect_dir_paths(fs);
    ZipfPathGenerator gen(fs, 1.5, 0.2);

    const auto paths = generate_many(gen, OperationType::Move, 500);

    ASSERT_EQ(paths.size(), 1000u);
    for (size_t i = 0; i < paths.size(); i += 2) {
        const path_type& source = paths[i];
        const path_type& destination = paths[i + 1];

        EXPECT_TRUE(contains(valid_files, source)) << source;
        EXPECT_TRUE(contains(valid_dirs, parent_path(destination))) << destination;
        EXPECT_TRUE(ends_with(filename(destination), "_new")) << destination;
    }
}

TEST(PathGenerator, ZipfHighLocalityStillGeneratesOnlyValidPaths) {
    auto fs = make_small_fs();
    const auto valid_files = collect_file_paths(fs);
    ZipfPathGenerator gen(fs, 1.5, 0.95);

    const auto paths = generate_many(gen, OperationType::Read, 1000);

    ASSERT_EQ(paths.size(), 1000u);
    for (const auto& path : paths) {
        EXPECT_TRUE(contains(valid_files, path)) << path;
    }
}

