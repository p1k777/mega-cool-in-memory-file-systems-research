#include "filesystem_b.hpp"

#include <algorithm>
#include <gtest/gtest.h>

TEST(FileSystemB, WriteReadMkdir) {
    filesystem::FileSystemB fs;

    fs.op_mkdir("/a");

    filesystem::IFileSystem::bytes_type data = {'h', 'e', 'l', 'l', 'o'};
    fs.op_write("/a/file.txt", data);

    EXPECT_EQ(fs.op_read("/a/file.txt"), data);
}

TEST(FileSystemB, LsReturnsChildren) {
    filesystem::FileSystemB fs;

    fs.op_mkdir("/a");
    fs.op_mkdir("/a/b");

    filesystem::IFileSystem::bytes_type data = {'h', 'i'};
    fs.op_write("/a/file.txt", data);

    auto list = fs.op_ls("/a");

    EXPECT_NE(std::find(list.begin(), list.end(), "/a/b"), list.end());
    EXPECT_NE(std::find(list.begin(), list.end(), "/a/file.txt"), list.end());
}