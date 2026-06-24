#include "../A_fs.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace filesystem {
namespace {

class TreeFileSystemTest : public ::testing::Test {
protected:
    std::unique_ptr<IFileSystem> fs = std::make_unique<TreeFileSystem>();
};

TEST_F(TreeFileSystemTest, NewFileSystemHasEmptyRootDirectory) {
    EXPECT_TRUE(fs->op_ls("/").empty());
    EXPECT_GT(fs->get_memory_usage(), 0U);
}

TEST_F(TreeFileSystemTest, CreatesNestedDirectoriesWithExistingParents) {
    fs->op_mkdir("/home");
    fs->op_mkdir("/home/user");

    EXPECT_EQ(fs->op_ls("/"), IFileSystem::units_list_type({"home"}));
    EXPECT_EQ(
        fs->op_ls("/home"),
        IFileSystem::units_list_type({"user"})
    );
}

TEST_F(TreeFileSystemTest, WriteCreatesFileAndReadReturnsItsBytes) {
    fs->op_mkdir("/data");
    const IFileSystem::bytes_type bytes = {'a', '\0', 'b'};

    fs->op_write("/data/file.bin", bytes);

    EXPECT_EQ(fs->op_read("/data/file.bin"), bytes);
}

TEST_F(TreeFileSystemTest, WriteOverwritesExistingFile) {
    fs->op_write("/file", {'o', 'l', 'd'});
    fs->op_write("/file", {'n', 'e', 'w'});

    EXPECT_EQ(
        fs->op_read("/file"),
        IFileSystem::bytes_type({'n', 'e', 'w'})
    );
    EXPECT_EQ(fs->op_ls("/"), IFileSystem::units_list_type({"file"}));
}

TEST_F(TreeFileSystemTest, ListReturnsImmediateChildrenOnly) {
    fs->op_mkdir("/src");
    fs->op_mkdir("/src/lib");
    fs->op_write("/src/main.cpp", {});
    fs->op_write("/src/lib/helper.cpp", {});

    auto children = fs->op_ls("/src");
    std::sort(children.begin(), children.end());

    EXPECT_EQ(
        children,
        IFileSystem::units_list_type({"lib", "main.cpp"})
    );
}

TEST_F(TreeFileSystemTest, FindMatchesStarGlobRecursively) {
    fs->op_mkdir("/src");
    fs->op_mkdir("/src/lib");
    fs->op_write("/src/main.cpp", {});
    fs->op_write("/src/main.hpp", {});
    fs->op_write("/src/lib/helper.cpp", {});

    auto matches = fs->op_find("/src", "*.cpp");
    std::sort(matches.begin(), matches.end());

    EXPECT_EQ(
        matches,
        IFileSystem::units_list_type({
            "/src/lib/helper.cpp",
            "/src/main.cpp"
        })
    );
}

TEST_F(TreeFileSystemTest, FindMatchesQuestionMarkAsOneCharacter) {
    fs->op_write("/a.cpp", {});
    fs->op_write("/ab.cpp", {});
    fs->op_write("/b.cpp", {});

    auto matches = fs->op_find("/", "?.cpp");
    std::sort(matches.begin(), matches.end());

    EXPECT_EQ(
        matches,
        IFileSystem::units_list_type({"/a.cpp", "/b.cpp"})
    );
}

TEST_F(TreeFileSystemTest, FindMatchesWholeNameAndTreatsDotLiterally) {
    fs->op_write("/main.cpp", {});
    fs->op_write("/mainXcpp", {});
    fs->op_write("/prefix-main.cpp-suffix", {});

    EXPECT_EQ(
        fs->op_find("/", "main.cpp"),
        IFileSystem::units_list_type({"/main.cpp"})
    );
}

TEST_F(TreeFileSystemTest, FindIsLimitedToRequestedSubtree) {
    fs->op_mkdir("/left");
    fs->op_mkdir("/right");
    fs->op_write("/left/file.txt", {});
    fs->op_write("/right/file.txt", {});

    EXPECT_EQ(
        fs->op_find("/left", "*.txt"),
        IFileSystem::units_list_type({"/left/file.txt"})
    );
}

TEST_F(TreeFileSystemTest, MoveRenamesFileAndPreservesContents) {
    fs->op_write("/old.txt", {'x'});

    fs->op_mv("/old.txt", "/new.txt");

    EXPECT_EQ(fs->op_read("/new.txt"), IFileSystem::bytes_type({'x'}));
    EXPECT_THROW(fs->op_read("/old.txt"), std::runtime_error);
}

TEST_F(TreeFileSystemTest, MoveDirectoryPreservesItsSubtree) {
    fs->op_mkdir("/source");
    fs->op_mkdir("/source/nested");
    fs->op_write("/source/nested/file", {'x'});
    fs->op_mkdir("/target");

    fs->op_mv("/source", "/target/moved");

    EXPECT_EQ(
        fs->op_read("/target/moved/nested/file"),
        IFileSystem::bytes_type({'x'})
    );
    EXPECT_THROW(fs->op_ls("/source"), std::runtime_error);
}

TEST_F(TreeFileSystemTest, MoveToSamePathDoesNothing) {
    fs->op_write("/file", {'x'});

    EXPECT_NO_THROW(fs->op_mv("/file", "/file"));
    EXPECT_EQ(fs->op_read("/file"), IFileSystem::bytes_type({'x'}));
}

TEST_F(TreeFileSystemTest, MoveRejectsRootCollisionAndCycles) {
    fs->op_mkdir("/dir");
    fs->op_mkdir("/dir/child");
    fs->op_write("/occupied", {});

    EXPECT_THROW(fs->op_mv("/", "/root"), std::runtime_error);
    EXPECT_THROW(fs->op_mv("/dir", "/dir/child/moved"), std::runtime_error);
    EXPECT_THROW(fs->op_mv("/dir", "/occupied"), std::runtime_error);

    EXPECT_NO_THROW(fs->op_ls("/dir"));
    EXPECT_NO_THROW(fs->op_ls("/dir/child"));
}

TEST_F(TreeFileSystemTest, OperationsRejectWrongNodeTypesAndMissingPaths) {
    fs->op_mkdir("/directory");
    fs->op_write("/file", {});

    EXPECT_THROW(fs->op_read("/directory"), std::runtime_error);
    EXPECT_THROW(fs->op_read("/missing"), std::runtime_error);
    EXPECT_THROW(fs->op_write("/directory", {}), std::runtime_error);
    EXPECT_THROW(fs->op_ls("/file"), std::runtime_error);
    EXPECT_THROW(fs->op_ls("/missing"), std::runtime_error);
    EXPECT_THROW(fs->op_find("/file", "*"), std::runtime_error);
}

TEST_F(TreeFileSystemTest, CreationRequiresExistingDirectoryParent) {
    fs->op_write("/file", {});

    EXPECT_THROW(fs->op_mkdir("/missing/child"), std::runtime_error);
    EXPECT_THROW(fs->op_write("/missing/file", {}), std::runtime_error);
    EXPECT_THROW(fs->op_mkdir("/file/child"), std::runtime_error);
    EXPECT_THROW(fs->op_write("/file/child", {}), std::runtime_error);
}

TEST_F(TreeFileSystemTest, CreationRejectsExistingPaths) {
    fs->op_mkdir("/directory");
    fs->op_write("/file", {});

    EXPECT_THROW(fs->op_mkdir("/directory"), std::runtime_error);
    EXPECT_THROW(fs->op_mkdir("/file"), std::runtime_error);
}

TEST_F(TreeFileSystemTest, OperationsRejectMalformedPaths) {
    EXPECT_THROW(fs->op_mkdir("relative"), std::invalid_argument);
    EXPECT_THROW(fs->op_mkdir("/trailing/"), std::invalid_argument);
    EXPECT_THROW(fs->op_mkdir("/double//slash"), std::invalid_argument);
    EXPECT_THROW(fs->op_mkdir("/."), std::invalid_argument);
    EXPECT_THROW(fs->op_mkdir("/.."), std::invalid_argument);
    EXPECT_THROW(fs->op_mkdir("/"), std::runtime_error);
}

TEST_F(TreeFileSystemTest, MemoryUsageIncreasesWhenNodesAndDataAreAdded) {
    const size_t empty_usage = fs->get_memory_usage();

    fs->op_mkdir("/directory");
    const size_t directory_usage = fs->get_memory_usage();

    fs->op_write("/directory/file", IFileSystem::bytes_type(1024, 'x'));
    const size_t file_usage = fs->get_memory_usage();

    EXPECT_GT(directory_usage, empty_usage);
    EXPECT_GT(file_usage, directory_usage);
}

} // namespace
} // namespace filesystem
