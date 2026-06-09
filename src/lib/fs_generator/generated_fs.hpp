#pragma once

#include <cstddef>
#include <vector>

#include "../filesystem.hpp"

namespace fsgenerator {

    class GeneratedFs {

    /* ========================================================================================================================================

        Описание GeneratedFs


        Необходимо конструктору
            - tree -- буквально готовое дерево, основная идея в том, что это уже сгенерированная структура и по сути просто интерфейс для
              дальнейшего взаимодействия
        

        fill(IFileSystem&) -- заполняет файловую систему


        TBD -- интерфейс для генератора путей (пока не ясно как он должен выглядеть)


        fs_ -- хранит файловую систему в удобном формате (облегченную)
        dirs_, files_ -- индексы внутри tree_, директории и файлы соответственно

        остальное -- просто служебные методы

    ======================================================================================================================================== */
    
    public:

        using path_type = filesystem::IFileSystem::path_type;

        struct tree_node_type {
            std::string name;
            size_t parent;
            size_t depth;
            size_t children;
            bool is_file;
            bool is_root;
        };


        GeneratedFs(std::vector<tree_node_type> tree);

        GeneratedFs(const GeneratedFs&) = default;
        GeneratedFs(GeneratedFs&&) = default;

        GeneratedFs& operator=(GeneratedFs);

        ~GeneratedFs() = default;


        void fill(filesystem::IFileSystem&);

    private:

        struct FsNode {
            path_type path;
            bool is_file;
        };

        std::vector<FsNode> fs_;
        std::vector<size_t> dirs_;
        std::vector<size_t> files_;
    };

};  // namespace fsgenerator