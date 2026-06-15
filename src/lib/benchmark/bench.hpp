#pragma once

#include <string>
#include <chrono>
#include <algorithm>

#include "filesystem.hpp"
#include "../common.hpp"

namespace benchmark {

enum class OperationType {
    Read,    // файл
    Write,   // файл
    Mkdir,   // папка
    Ls,      // папка
    Move,    // файл Х2
    Find     // похуй похуй
};

struct Operation {
    OperationType type;

    std::string path;
    std::string second_path;
    std::string pattern; // для поиска 
};


struct Metrics {
    double avg_latency_us;         // средняя задержка 
    double p99_latency_us;         // процентиль скорости операций
    double throughput_ops_sec;     // операций в секунду
    size_t memory_usage_bytes;
};


enum class Distribution
{
    Uniform,
    Zipf15,
    Zipf20
};

struct ExperimentConfig
{
    // дерево

    int depth;
    int width;
    double fill_factor;

    // probability jgthfwbazqqfwd;mgwf;gvm

    double p_read;
    double p_write;
    double p_mkdir;
    double p_ls;
    double p_mv;
    double p_find;

    // параметры доступа 

    Distribution distribution;

    double locality;

    // параметры эксперимента

    size_t operations;

    size_t repeats;
};



class Benchmark {

public:

    Benchmark() = default;
    ~Benchmark() = default;


    Metrics run(
        filesystem::IFileSystem& fs,
        const ExperimentConfig& cfg
    );
    
private:

    void executeOperation(
        filesystem::IFileSystem& fs,
        const Operation& op
    );

    // вспомогательные функции для вычислений
    double AvgLatency(const std::vector <double> &v);
    double P99Calc(const std::vector <double> &v);
    double ThroughputCalc(size_t op_num, double total_time);

    

};


}; // namespace benchmark


/*
if(random() < locality)
{
    return randomPathInsideSubtree(
        lastPath
    );
}
else
{
    return randomPathGlobal();
}
*/

namespace fsgenerator {

    class FsGenerator {

    /* ========================================================================================================================================

        Описание FsGenerator


        Необходимо конструктору:
            - D, W, F -- параметры файловой системы (глубина, ширина, заполненность)
            - file_p (*) -- вероятность, что очередной созданный узел будет файлом (ожидаемое отношение количества файлов к количеству узлов)
            - max_node_count (*) -- ограничение на количество узлов, беру себе право добавить W узлов сверх вот этой штуки


        Про копирование -- сам объект генератора хочется сделать легким, поэтому копировать его не стоит,
        тем более внутри хранится "дерево" (оно фейковое), а генерировать точно такое же дерево -- дело малополезное

        Про перемещение -- перемещать смысла чуть больше, но по-моему все равно недостаточно, поэтому пока тоже запрещаем


        generate() -- запускается понятное дело на уже сконструированном объекте,
        возвращает класс, с которым генератору путей будет удобно работать, но про него в другом файле

        tree_ -- хранит подобие дерева (облегченное)
        dirs_, files_, fillable_dirs_ -- индексы внутри tree_, директории, в которые еще можно расширять

        остальное -- просто ограничения и служебные методы

    ======================================================================================================================================== */

    public:

        FsGenerator(size_t D, size_t W, double F, double file_p=0.5, size_t max_nodes_count=1e7);

        FsGenerator(const FsGenerator&) = delete;
        FsGenerator(FsGenerator&&) = delete;

        FsGenerator& operator=(const FsGenerator&) = delete;
        FsGenerator& operator=(FsGenerator*&) = delete;

        ~FsGenerator() = default;


        GeneratedFs generate();
        
        void clear() noexcept;

    private:

        void add_root_();
        void grow_spine_();
        void add_node_(size_t parent, rnd::Random&);
        void add_dir_(std::string name, size_t parent, bool is_root=false);
        void add_file_(std::string name, size_t parent);

        std::vector<typename GeneratedFs::tree_node_type> tree_;
        std::vector<size_t> fillable_dirs_;

        size_t target_depth_;
        size_t target_width_;
        double file_p_;
        size_t target_nodes_count_;

        size_t files_count_;
        size_t dirs_count_;

    };

}; // namespace fsgenerator