#include "../C_fs.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace filesystem {
namespace {

class FlatHashFileSystemTest : public ::testing::Test {
protected:
    std::unique_ptr<IFileSystem> fs = std::make_unique<FlatHashFileSystem>();
};

TEST_F(FlatHashFileSystemTest, NewFileSystemHasEmptyRootDirectory) {
    EXPECT_TRUE(fs->op_ls("/").empty());
    EXPECT_GT(fs->get_memory_usage(), 0U);
}

} // namespace
} // namespace filesystem
