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


        tree_ -- хранит подобие дерева (облегченное)
        dirs_, files_ -- индексы внутри tree_, директории и файлы соответственно

        остальное -- просто служебные методы

    ======================================================================================================================================== */
    
    public:

        struct node_type {
            std::string name;
            size_t parent;
            size_t depth;
            size_t children;
            bool is_file;
            bool is_root;
        };


        GeneratedFs(std::vector<node_type> tree);

        GeneratedFs(const GeneratedFs&) = default;
        GeneratedFs(GeneratedFs&&);

        GeneratedFs& operator=(const GeneratedFs&) = default;
        GeneratedFs& operator=(GeneratedFs&&);

        ~GeneratedFs() = default;


        void fill(filesystem::IFileSystem&);

    private:

        std::vector<node_type> tree_;
        std::vector<size_t> dirs_;
        std::vector<size_t> files_;
    };

};  // namespace fsgenerator