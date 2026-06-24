#include "B_fs.hpp"

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

TEST(FileSystemB, FindByMask) {
    filesystem::FileSystemB fs;

    fs.op_mkdir("/a");
    fs.op_mkdir("/a/b");

    filesystem::IFileSystem::bytes_type data = {'h'};

    fs.op_write("/a/file.txt", data);
    fs.op_write("/a/b/test.txt", data);
    fs.op_write("/a/b/main.cpp", data);
    fs.op_write("/a/b/file1.cpp", data);

    auto txt_files = fs.op_find("/", "*.txt");

    EXPECT_NE(std::find(txt_files.begin(), txt_files.end(), "/a/file.txt"), txt_files.end());
    EXPECT_NE(std::find(txt_files.begin(), txt_files.end(), "/a/b/test.txt"), txt_files.end());

    auto cpp_files = fs.op_find("/", "*.cpp");

    EXPECT_NE(std::find(cpp_files.begin(), cpp_files.end(), "/a/b/main.cpp"), cpp_files.end());
    EXPECT_NE(std::find(cpp_files.begin(), cpp_files.end(), "/a/b/file1.cpp"), cpp_files.end());

    auto one_char = fs.op_find("/", "file?.cpp");

    EXPECT_NE(std::find(one_char.begin(), one_char.end(), "/a/b/file1.cpp"), one_char.end());
    EXPECT_EQ(std::find(one_char.begin(), one_char.end(), "/a/b/main.cpp"), one_char.end());
}

TEST(FileSystemB, MoveFile) {
    filesystem::FileSystemB fs;

    fs.op_mkdir("/a");
    fs.op_mkdir("/b");

    filesystem::IFileSystem::bytes_type data = {'h', 'i'};

    fs.op_write("/a/file.txt", data);
    fs.op_mv("/a/file.txt", "/b/moved.txt");

    EXPECT_EQ(fs.op_read("/b/moved.txt"), data);

    auto list_a = fs.op_ls("/a");
    EXPECT_EQ(std::find(list_a.begin(), list_a.end(), "/a/file.txt"), list_a.end());

    auto list_b = fs.op_ls("/b");
    EXPECT_NE(std::find(list_b.begin(), list_b.end(), "/b/moved.txt"), list_b.end());
}

TEST(FileSystemB, MoveDirectory) {
    filesystem::FileSystemB fs;

    fs.op_mkdir("/a");
    fs.op_mkdir("/a/sub");
    fs.op_mkdir("/target");

    filesystem::IFileSystem::bytes_type data = {'o', 'k'};

    fs.op_write("/a/sub/file.txt", data);

    fs.op_mv("/a", "/target/new_a");

    EXPECT_EQ(fs.op_read("/target/new_a/sub/file.txt"), data);

    auto found = fs.op_find("/target", "*.txt");
    EXPECT_NE(
        std::find(found.begin(), found.end(), "/target/new_a/sub/file.txt"),
        found.end()
    );
}
