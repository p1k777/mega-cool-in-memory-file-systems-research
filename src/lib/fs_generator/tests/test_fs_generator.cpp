#include <gtest/gtest.h>

#include <cstddef>
#include <vector>
#include <unordered_map>

#include "filesystem.hpp"
#include "fs_generator/generated_fs.hpp"
#include "fs_generator/fs_generator.hpp"

class FakeFileSystem : public filesystem::IFileSystem {
public:
    FakeFileSystem() : cur_depth_(0), cur_width_(0), files_count_(0) {}

    bytes_type op_read(const path_type&) const override { return {}; }
    units_list_type op_ls(const path_type&) const override { return {}; }
    void op_mv(const path_type&, const path_type&) override {}
    size_t get_memory_usage() const noexcept override { return 0; }
    units_list_type op_find(const path_type&, const path_type&) const noexcept override { return {}; }

    void op_mkdir(const path_type& p) override {
        if (get_idx_.contains(p)) { 
            throw std::runtime_error("directory already exists: " + p); 
        }

        if (p.empty() || p == "/") {
            get_idx_[p] = tree_.size();
            tree_.push_back({}); 
            depths_[p] = 0;
            return;
        }

        path_type parent = get_parent_path(p);

        if (!get_idx_.contains(parent)) { 
            throw std::runtime_error("parent directory does not exist for: " + p); 
        }

        get_idx_[p] = tree_.size();
        tree_.push_back({});

        children_counts_[parent]++;
        cur_width_ = std::max(cur_width_, children_counts_[parent]);

        size_t node_depth = depths_[parent] + 1;
        depths_[p] = node_depth;
        cur_depth_ = std::max(cur_depth_, node_depth);
    }

    void op_write(const path_type& p, const bytes_type& data) override {
        if (get_idx_.contains(p)) { 
            throw std::runtime_error("file already exists: " + p); 
        }

        path_type parent = get_parent_path(p);

        if (!get_idx_.contains(parent)) { 
            throw std::runtime_error("parent directory does not exist for: " + p); 
        }

        get_idx_[p] = tree_.size();
        tree_.push_back({});
        
        files_count_++;

        children_counts_[parent]++;
        cur_width_ = std::max(cur_width_, children_counts_[parent]);

        size_t node_depth = depths_[parent] + 1;
        depths_[p] = node_depth;
        cur_depth_ = std::max(cur_depth_, node_depth);
    }

    size_t get_max_depth() const { return cur_depth_; }
    size_t get_max_width() const { return cur_width_; }
    size_t get_files_count() const { return files_count_; }
    size_t get_total_nodes() const { return tree_.size(); }

private:
    path_type get_parent_path(const path_type& p) const {
        size_t last_slash = p.rfind('/');
        if (last_slash == path_type::npos) return ""; 
        if (last_slash == 0) return "/";              
        return p.substr(0, last_slash);
    }

    size_t cur_depth_;
    size_t cur_width_;
    size_t files_count_;

    std::vector<typename fsgenerator::GeneratedFs::tree_node_type> tree_;
    std::unordered_map<path_type, size_t> get_idx_;
    
    std::unordered_map<path_type, size_t> depths_;
    std::unordered_map<path_type, size_t> children_counts_;
};


TEST(FakeFileSystemTest, CalcDepth) {
    FakeFileSystem fs;
    
    fs.op_mkdir("/"); 
    
    fs.op_mkdir("/folder");
    fs.op_mkdir("/folder/subfolder");
    fs.op_write("/folder/subfolder/f.txt", {});

    EXPECT_EQ(fs.get_max_depth(), 3);
}

TEST(FakeFileSystemTest, CalcWidth) {
    FakeFileSystem fs;
    fs.op_mkdir("/");
    fs.op_mkdir("/parent");
    
    fs.op_mkdir("/parent/dir1");
    fs.op_mkdir("/parent/dir2");
    fs.op_write("/parent/file1.txt", {});
    fs.op_write("/parent/file2.txt", {});
    fs.op_write("/parent/file3.txt", {});

    EXPECT_EQ(fs.get_max_width(), 5);
    EXPECT_EQ(fs.get_files_count(), 3);
}

TEST(FakeFileSystemTest, MissingParentDir) {
    FakeFileSystem fs;
    fs.op_mkdir("/");
    
    EXPECT_THROW({
        fs.op_write("/missing_dir/file.txt", {});
    }, std::runtime_error);

    EXPECT_THROW({
        fs.op_mkdir("/missing_dir/new_dir");
    }, std::runtime_error);
}


using namespace fsgenerator;
using tree_node_type = GeneratedFs::tree_node_type;

TEST(GeneratedFsTest, RootOnly) {
    std::vector<tree_node_type> tree = {
        {"", 0, 0, 0, false, true}
    };
    
    GeneratedFs gen_fs(tree);
    FakeFileSystem fake_fs;
    
    gen_fs.fill(fake_fs);

    EXPECT_EQ(fake_fs.get_max_depth(), 0);
    EXPECT_EQ(fake_fs.get_max_width(), 0);
    EXPECT_EQ(fake_fs.get_files_count(), 0);
}

TEST(GeneratedFsTest, WideDir) {
    std::vector<tree_node_type> tree = {
        {"", 0, 0, 3, false, true},              
        {"file1.txt", 0, 1, 0, true, false},     
        {"file2.txt", 0, 1, 0, true, false},     
        {"dir1", 0, 1, 0, false, false}          
    };
    
    GeneratedFs gen_fs(tree);
    FakeFileSystem fake_fs;
    
    gen_fs.fill(fake_fs);

    EXPECT_EQ(fake_fs.get_max_depth(), 1);
    EXPECT_EQ(fake_fs.get_max_width(), 3);
    EXPECT_EQ(fake_fs.get_files_count(), 2);
}

TEST(GeneratedFsTest, DeepNested) {
    std::vector<tree_node_type> tree = {
        {"", 0, 0, 1, false, true},                 
        {"folder", 0, 1, 1, false, false},          
        {"subfolder", 1, 2, 1, false, false},       
        {"file.txt", 2, 3, 0, true, false}          
    };
    
    GeneratedFs gen_fs(tree);
    FakeFileSystem fake_fs;
    
    gen_fs.fill(fake_fs);

    EXPECT_EQ(fake_fs.get_max_depth(), 3);
    EXPECT_EQ(fake_fs.get_max_width(), 1);
    EXPECT_EQ(fake_fs.get_files_count(), 1);
}

TEST(GeneratedFsTest, Mixed) {
    std::vector<tree_node_type> tree = {
        {"", 0, 0, 2, false, true},                 
        {"dir1", 0, 1, 2, false, false},            
        {"fileA.txt", 1, 2, 0, true, false},        
        {"fileB.txt", 1, 2, 0, true, false},        
        {"fileC.txt", 0, 1, 0, true, false}         
    };
    
    GeneratedFs gen_fs(tree);
    FakeFileSystem fake_fs;
    
    gen_fs.fill(fake_fs);

    EXPECT_EQ(fake_fs.get_max_depth(), 2);
    EXPECT_EQ(fake_fs.get_max_width(), 2);
    EXPECT_EQ(fake_fs.get_files_count(), 3);
}


TEST(FsGeneratorTest, Params) {

    struct Suit {
        size_t D, W;
        double F, f_p;
    };
    auto suits = {
        Suit{3, 2, 0.7, 0.3},
        Suit{10, 2, 0.4, 0.3},
        Suit{3, 10, 0.7, 0.5},
        Suit{10, 1, 1., 0.},
        Suit{1, 100, 1., 0.5},
        Suit{3, 10, 0.8, 0.1},
    };

    double delta = 0.1;
    for (auto suit : suits) {
        FsGenerator g(suit.D, suit.W, suit.F, suit.f_p);

        FakeFileSystem fs;
        GeneratedFs(g.generate()).fill(fs);

        EXPECT_LE(fs.get_max_depth(), suit.D);
        EXPECT_LE(fs.get_max_width(), suit.W);

        EXPECT_TRUE(
            (1-delta) * suit.D <= fs.get_max_depth() &&
            fs.get_max_depth() <= (1+delta)*suit.D
        );

        EXPECT_TRUE(
            (1-delta) * suit.W <= fs.get_max_width() &&
            fs.get_max_width() <= (1+delta)*suit.W
        );
    }


}

TEST(GeneratedFsAccessorsTest, GetNodeAndPathReturnCorrectData) {
    std::vector<tree_node_type> mock_tree = {
        {"", 0, 0, 1, false, true},
        {"folder", 0, 1, 1, false, false},
        {"file.txt", 1, 2, 0, true, false}
    };
    
    GeneratedFs gen_fs(mock_tree);

    EXPECT_EQ(gen_fs.get_node(0).name, "");
    EXPECT_TRUE(gen_fs.get_node(0).is_root);
    
    EXPECT_EQ(gen_fs.get_node(2).name, "file.txt");
    EXPECT_TRUE(gen_fs.get_node(2).is_file);

    EXPECT_EQ(gen_fs.get_path(0), "/"); 
    EXPECT_EQ(gen_fs.get_path(1), "/folder");
    EXPECT_EQ(gen_fs.get_path(2), "/folder/file.txt");
}

TEST(GeneratedFsAccessorsTest, ThrowsOutOfRangeForInvalidIndex) {
    std::vector<tree_node_type> mock_tree = {
        {"", 0, 0, 0, false, true}
    };
    
    GeneratedFs gen_fs(mock_tree);

    EXPECT_THROW(gen_fs.get_node(1), std::out_of_range);
    EXPECT_THROW(gen_fs.get_path(999), std::out_of_range);
}

TEST(GeneratedFsAccessorsTest, GlobalListsReturnCorrectIndices) {
    std::vector<tree_node_type> mock_tree = {
        {"", 0, 0, 2, false, true},
        {"dir1", 0, 1, 0, false, false},
        {"file1.txt", 0, 1, 0, true, false},
        {"file2.txt", 0, 1, 0, true, false}
    };
    
    GeneratedFs gen_fs(mock_tree);

    const auto& files = gen_fs.get_files(); 
    const auto& dirs = gen_fs.get_dirs();

    ASSERT_EQ(files.size(), 2);
    
    EXPECT_EQ(files[0], 2);
    EXPECT_EQ(files[1], 3);

    ASSERT_EQ(dirs.size(), 2);
    
    EXPECT_EQ(dirs[0], 0);
    EXPECT_EQ(dirs[1], 1);
}
