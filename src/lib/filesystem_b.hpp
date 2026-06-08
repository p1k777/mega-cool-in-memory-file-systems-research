#pragma once
#include "filesystem.hpp"


namespace filesystem {

class FileSystemB final: public IFileSystem {
public:
    bytes_type op_read(const path_type&) const override;
    void op_write(const path_type&, const bytes_type&) override;

    void op_mkdir(const path_type&) override;
    units_list_type op_ls(const path_type&) const override;

    void op_mv(const path_type&, const path_type&) override;
    units_list_type op_find(const path_type&, const path_type&) const override;

    size_t get_memory_usage() const noexcept override;
};

} // namespace filesystem