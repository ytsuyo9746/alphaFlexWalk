#ifndef GRAPH_H_
#define GRAPH_H_
#include <iostream>
using namespace std;
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <vector>
#include <queue>
#include <cassert>
#include <random>
#include <fstream>
#include <sstream>
#include <climits>
#include <emmintrin.h>
#define PREFETCH_HINT _MM_HINT_T0

using Node = long long;
using Edge = long long;

class Graph {
public:
    using Node = long long;
    using Edge = long long;
    
    Graph(string data_dir);
    string get_data_dir() const {return data_dir;}
    Node get_node_count() const {return node_count;}
    int get_adj_num(Node node_id) const {return start_suf_list.at(node_id + 1) - start_suf_list.at(node_id);}
    vector<Node> get_adj_list(Node node_id) const;
    Node get_random_adjacent(Node node_id) const;
    void get_paths_by_mc(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const;
    void get_paths_by_thunderRW(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const;
    void get_paths_by_thunderRW_without_prefetch(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const;
    void get_paths_longer_than_1(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const;
    void calc_ppr_by_fp(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& residue, unordered_map<Node, double>& ppr) const;
    void calc_ppr_by_fora_thunder(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& ppr) const;
    void calc_ppr_by_fora_thunder(Node src_id, double alpha, long long walk_count, unordered_map<Node, double>& ppr) const {
        map<Node, double> src_map{{src_id, 1}};
        calc_ppr_by_fora_thunder(src_map, alpha, walk_count, ppr);
    }
    void calc_ppr_by_fora_mc(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& ppr) const;
    void calc_ppr_by_fora_mc(Node src_id, double alpha, long long walk_count, unordered_map<Node, double>& ppr) const {
        map<Node, double> src_map{{src_id, 1}};
        calc_ppr_by_fora_mc(src_map, alpha, walk_count, ppr);
    }
    void show_graph() const;

private:
    string data_dir;
    long long node_count;
    bool is_directed;
    vector<Node> end_node_list;
    vector<Edge> start_suf_list;

    mutable random_device rd;
    mutable mt19937 gen;
    mutable uniform_real_distribution<> rand_0_1;
    mutable uniform_int_distribution<> rand_int;

    void _load_attribute();
    void _load_edge_from_txt(map<Node, set<Node>> &adj_list_list);
};

map<Node, double> get_normalized_map(const map<long long, double>& input_map);

#endif