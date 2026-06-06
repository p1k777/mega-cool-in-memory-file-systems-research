#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace filesystem {
    
    class IFileSystem {
    public:

        // типы
        using path_type       = std::string;
        using bytes_type      = std::vector<char>;
        using units_list_type = std::vector<path_type>;


        // интерфейс для бенчмарка
        virtual bytes_type op_read(const path_type&) const = 0;
        virtual void op_write(const path_type&, const bytes_type&) = 0;

        virtual void op_mkdir(const path_type&) = 0;
        virtual units_list_type op_ls(const path_type&) const = 0;

        virtual void op_mv(const path_type&, const path_type&) = 0;
        virtual units_list_type op_find(const path_type&, const path_type&) const = 0;

        virtual size_t get_memory_usage() const noexcept = 0;

        virtual ~IFileSystem() = default;

    };

} // namespace filesystem


