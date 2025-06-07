#ifndef INDEX_H_
#define INDEX_H_
#define PREFETCH_HINT _MM_HINT_T0
#include "Graph.h"
// #include <emmintrin.h>
// #define NDEBUG
using namespace std;

class GeometricDistribution {
public:
    GeometricDistribution(double success_prob)
        : coef(1.0 / std::log2(1 - success_prob)) {}

    // Get the number of failure trials until success (>= 0)
    int get() const {
        return static_cast<int>(log2(dist(mt)) * coef);
    }

private:
    double coef;
    inline static std::random_device rd;
    inline static std::mt19937 mt{rd()};
    inline static std::uniform_real_distribution<double> dist{0.0, 1.0};
};

class Index {
public:
    using Node = Graph::Node;

    Index(Graph& graph, double alpha_index);
    double get_alpha_index() const {return alpha_index;}
    unordered_map<Node, int> get_referred_count_map() const {return referred_count_map;}
    void reset_referred_count_map() {referred_count_map.clear();}
    
    void generate_index_from_scratch(double size_ratio);
    void save_index(string file_path) const;
    void load_index(string file_path);
    void get(Node source_id, vector<Node>& path);
    void get(Node source_id, int max_len, vector<Node>& path);
    // void get_paths_without_prefetch(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths);
    void get_paths(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths) {
        reset_referred_count_map();
        _get_paths(source_id, walk_count, alpha, paths);
    }
    void calc_ppr_by_fora_plus(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& ppr, bool enable_thunder);
    // void calc_ppr_by_fora_plus_with_thunder(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& ppr);
    void show_index() const;

private:
    Graph& graph;
    vector<Node> node_in_path_list;
    vector<long long> path_start_suf_list;
    vector<long long> source_start_suf_list;
    unordered_map<Node, int> referred_count_map;

    mutable random_device rd;
    mutable mt19937 gen;
    mutable uniform_real_distribution<> rand_0_1;
    mutable uniform_int_distribution<> rand_int;

    int _get_index_size_for_node(Node node_id) const {return source_start_suf_list.at(node_id + 1) - source_start_suf_list.at(node_id);}
    int _required_index_size(Node src_id, double size_ratio) const {return ceil(size_ratio * graph.get_adj_num(src_id) / alpha_index);}
    void _get_paths(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths);
    // void _get_paths_with_thunder(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths);
    // void _get_paths_samescale(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths);
    // void _get_paths_upscale(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths, GeometricDistribution& geo_dist);
    // void _get_paths_downscale(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths);
    void _get_paths_downscale_with_thunder(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths, GeometricDistribution& geo_dist);

    // vector<vector<vector<int>>> node_to_path_list;
    int index_size;
    double alpha_index;
    random_device seed_gen;
    int ring_size=64;
};

#endif