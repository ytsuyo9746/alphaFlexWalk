#include "Graph.h"

Graph::Graph(string data_dir) : data_dir(data_dir) {
    char splitter = ' ';
    
    _load_attribute();

    map<Node, set<Node>> adj_list_list;
    _load_edge_from_txt(adj_list_list);

    // construct CSR
    Edge current_edge_count = 0; 
    for (Node src_id = 0; src_id < node_count; src_id++) {
        start_suf_list.push_back(current_edge_count);
        set<Node> adj_set = adj_list_list[src_id];
        for (Node adj_id : adj_set) {
            end_node_list.push_back(adj_id);
        }
        current_edge_count += adj_set.size();
    }
    start_suf_list.push_back(current_edge_count);

    gen = mt19937(rd());
    rand_0_1 = uniform_real_distribution<>(0.0, 1.0);
    rand_int = uniform_int_distribution<>(0, INT_MAX);

    return;
}

vector<Graph::Node> Graph::get_adj_list(Node node_id) const {
    vector<Graph::Node> adj_list;
    int adj_num = get_adj_num(node_id);
    adj_list.reserve(adj_num);
    Edge start_suf = start_suf_list.at(node_id);
    for (int i = 0; i < adj_num; i++) adj_list.push_back(end_node_list.at(start_suf + i));
    return adj_list;
}

Graph::Node Graph::get_random_adjacent(Node node_id) const {
    int degree = get_adj_num(node_id);
    if (degree == 0) return -1;
    int adj_suf = rand_int(gen) % degree;
    return end_node_list.at(start_suf_list.at(node_id) + adj_suf);
}   

void Graph::get_paths_by_mc(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const {
    paths.resize(walk_count);
    for (long long i = 0; i < walk_count; i++) {
        Node current_node = source_id;
        paths.at(i).push_back(current_node);
        while (rand_0_1(gen) > alpha) {
            current_node = get_random_adjacent(current_node);
            paths.at(i).push_back(current_node);
            if (current_node == -1) break;
        } 
    }
}

void Graph::get_paths_by_thunderRW(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const {
    struct WalkerMeta {
        long long id_;
        Node current_;
    };
    struct BufferSlot {
        bool empty_;
        WalkerMeta w_;
        int64_t r_;
        Edge suf_;
    };
    
    // random_device rd;
    // mt19937 gen(rd());
    // uniform_real_distribution<> dist_0_1(0.0, 1.0);
    // uniform_int_distribution<> dist_int(0, INT_MAX);

    paths.resize(walk_count);
    for (long long i = 0; i < walk_count; i++) {
        paths.at(i).push_back(source_id);
        paths.at(i).reserve((long long)(3/alpha));
    }

    long long next = 0;
    long long num_completed_walkers = 0;
    int ring_size = 64;
    BufferSlot r[ring_size];

    int initial_walkers_in_ring = min((long long)ring_size, walk_count);
    while (next < initial_walkers_in_ring) {
        r[next].empty_ = false;
        r[next].w_ = {next, source_id};
        next++;
    }
    for (int i = initial_walkers_in_ring; i < ring_size; ++i) {
        r[i].empty_ = true;
    }

    while (num_completed_walkers < walk_count) {
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                // Update the status of the walker.
                if (rand_0_1(gen) < alpha) {
                    // If the walker completes, then set the slot as empty.
                    slot.empty_ = true;
                    num_completed_walkers += 1;
                }
            }

            // If the slot is empty, then add a new walker to the ring buffer.
            if (slot.empty_) {
                if (next < walk_count) {
                    slot.empty_ = false;
                    slot.w_ = {next++, source_id};
                }
            }
        }

        // Stage 1: generate random number & prefetch the degree.
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                slot.r_ = rand_int(gen);
                _mm_prefetch((void*)(start_suf_list.data() + slot.w_.current_), PREFETCH_HINT);
            }
        }

        // Stage 2: generate the position & prefetch the neighbor.
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                int degree = start_suf_list[slot.w_.current_ + 1] - start_suf_list[slot.w_.current_];
                if (degree == 0) {
                    paths.at(slot.w_.id_).push_back(-1);
                    slot.empty_ = true;
                    num_completed_walkers += 1;
                } else {
                    slot.r_ = slot.r_ % degree;
                    slot.suf_ = start_suf_list[slot.w_.current_] + slot.r_;
                    _mm_prefetch((void*)(end_node_list.data() + slot.suf_), PREFETCH_HINT);
                }
            }
        }

        // Stage 3: update the walker.
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                slot.w_.current_ = end_node_list[slot.suf_];
                paths.at(slot.w_.id_).push_back(slot.w_.current_);
            }
        }

    }

    return;
}

void Graph::get_paths_by_thunderRW_without_prefetch(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const {
    struct WalkerMeta {
        long long id_;
        Node current_;
    };
    struct BufferSlot {
        bool empty_;
        WalkerMeta w_;
        int64_t r_;
        Edge suf_;
    };
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dist_0_1(0.0, 1.0);
    uniform_int_distribution<> dist_int(0, INT_MAX);

    paths.resize(walk_count);
    for (long long i = 0; i < walk_count; i++) {
        paths.at(i).push_back(source_id);
        paths.at(i).reserve((long long)(3/alpha));
    }

    long long next = 0;
    long long num_completed_walkers = 0;
    int ring_size = 64;
    BufferSlot r[ring_size];

    int initial_walkers_in_ring = min((long long)ring_size, walk_count);
    while (next < initial_walkers_in_ring) {
        r[next].empty_ = false;
        r[next].w_ = {next, source_id};
        next++;
    }
    for (int i = initial_walkers_in_ring; i < ring_size; ++i) {
        r[i].empty_ = true;
    }

    while (num_completed_walkers < walk_count) {
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                // Update the status of the walker.
                if (dist_0_1(gen) < alpha) {
                    // If the walker completes, then set the slot as empty.
                    slot.empty_ = true;
                    num_completed_walkers += 1;
                }
            }

            // If the slot is empty, then add a new walker to the ring buffer.
            if (slot.empty_) {
                if (next < walk_count) {
                    slot.empty_ = false;
                    slot.w_ = {next++, source_id};
                }
            }
        }

        // Stage 1: generate random number & prefetch the degree.
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                slot.r_ = dist_int(gen);
                // _mm_prefetch((void*)(start_suf_list.data() + slot.w_.current_), PREFETCH_HINT);
            }
        }

        // Stage 2: generate the position & prefetch the neighbor.
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                int degree = start_suf_list[slot.w_.current_ + 1] - start_suf_list[slot.w_.current_];
                if (degree == 0) {
                    paths.at(slot.w_.id_).push_back(-1);
                    slot.empty_ = true;
                    num_completed_walkers += 1;
                } else {
                    slot.r_ = slot.r_ % degree;
                    slot.suf_ = start_suf_list[slot.w_.current_] + slot.r_;
                    // _mm_prefetch((void*)(end_node_list.data() + slot.suf_), PREFETCH_HINT);
                }
            }
        }

        // Stage 3: update the walker.
        for (int i = 0; i < ring_size; ++i) {
            BufferSlot& slot = r[i];
            if (!slot.empty_) {
                slot.w_.current_ = end_node_list[slot.suf_];
                paths.at(slot.w_.id_).push_back(slot.w_.current_);
            }
        }

    }

    return;
}

void Graph::get_paths_longer_than_1(Node source_id, double alpha, long long walk_count, vector<vector<Node>>& paths) const {
    paths.resize(walk_count);
    for (long long i = 0; i < walk_count; i++) {
        Node current_node = source_id;
        paths.at(i).push_back(current_node);
        do {
            current_node = get_random_adjacent(current_node);
            paths.at(i).push_back(current_node);
            if (current_node == -1) break;
        } while (rand_0_1(gen) < alpha);
    }
}

void Graph::calc_ppr_by_fp(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& residue, unordered_map<Node, double>& ppr) const {
    map<Node, double> normalized_src_map = get_normalized_map(src_map);
    set<Node> active_node_set;
    queue<Node> active_node_queue;
    for (const auto&[node_id, val] : normalized_src_map) {
        residue.emplace(node_id, val);
        if (val > get_adj_num(node_id) / (alpha * walk_count)) {
            active_node_set.insert(node_id);
            active_node_queue.push(node_id);
        }
    }

    while (active_node_queue.size() > 0) {
        Node node_id = active_node_queue.front();
        int node_degree = get_adj_num(node_id);
        active_node_queue.pop();
        active_node_set.erase(node_id);
        // if (ppr.count(node_id) == 0) ppr.emplace(node_id, 0);
        // dangling node 到達時はスーパーノード-1に渡す．スーパーノードはactive node 対象外
        if (node_degree == 0) {
            ppr[node_id] += alpha * residue.at(node_id);
            ppr[-1] += (1 - alpha) * residue.at(node_id);
        } else {
            vector<Node> adj_list = get_adj_list(node_id);
            for (int i = 0; i < node_degree; i++) {
                Node adj_id = adj_list[i];
                int adj_degree = get_adj_num(adj_id);
                residue[adj_id] += (1 - alpha) * residue.at(node_id) / node_degree;
                if ((residue.at(adj_id) > adj_degree / (alpha * walk_count)) && (active_node_set.count(adj_id) == 0)) {
                    active_node_set.insert(adj_id);
                    active_node_queue.push(adj_id);
                }
            }
            ppr[node_id] += alpha * residue.at(node_id);
        }
        residue[node_id] = 0;
    }

    return;
}

void Graph::calc_ppr_by_fora_thunder(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& ppr) const {
    map<Node, double> normalized_src_map = get_normalized_map(src_map);
    unordered_map<Node, double> residue;
    calc_ppr_by_fp(src_map, alpha, walk_count, residue, ppr);
    residue.erase(-1);
    ppr.erase(-1);

    for (const auto&[node_id, r_val] : residue) {
        if (r_val == 0) continue;
        
        long long walk_count_i = (long long)ceil(r_val * walk_count);
        vector<vector<Node>> paths;
        get_paths_by_thunderRW(node_id, alpha, walk_count_i, paths);
        for (vector<Node> path : paths) {
            ppr[path.back()] += (double)r_val / walk_count_i;
        }
    }
}

void Graph::calc_ppr_by_fora_mc(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& ppr) const {
    map<Node, double> normalized_src_map = get_normalized_map(src_map);
    unordered_map<Node, double> residue;
    calc_ppr_by_fp(src_map, alpha, walk_count, residue, ppr);
    residue.erase(-1);
    ppr.erase(-1);

    for (const auto&[node_id, r_val] : residue) {
        if (r_val == 0) continue;
        
        long long walk_count_i = (long long)ceil(r_val * walk_count);
        vector<vector<Node>> paths;
        get_paths_by_mc(node_id, alpha, walk_count_i, paths);
        for (vector<Node> path : paths) {
            ppr[path.back()] += (double)r_val / walk_count_i;
        }
    }
}

void Graph::show_graph() const {
    cout << "end_node_list" << endl;
    for (Node node_id : end_node_list) cout << node_id << " ";
    cout << endl << endl;

    cout << "start_suf_list" << endl;
    for (Edge edge_id : start_suf_list) cout << edge_id << " ";
    cout << endl << endl;

    cout << "edges of this graph" << endl;
    for (Node node_id = 0; node_id < node_count; node_id++) {
        cout << node_id << endl;
        for (Edge edge_suf = start_suf_list.at(node_id); edge_suf < start_suf_list.at(node_id + 1); edge_suf++) {
            Node adj_id = end_node_list.at(edge_suf);
            cout << adj_id << " ";
        }
        cout << endl << endl;
    }

    return;
}

// load attribute written in "./dataset/" + data_dir + "/attributes.txt".
// In Graph, graph needs to be static and unweighted.
void Graph::_load_attribute() {
    ifstream file;
    char splitter = ' ';
    string attribute_file_path = "./dataset/" + data_dir + "/attributes.txt";
    file.open(attribute_file_path, ios::in);
    assert(file.is_open());
    string line, attribute, val;
    bool error_flag = false;
    while (getline(file, line)) {
        stringstream ss{line};
        getline(ss, attribute, splitter);
        if (attribute == "n") {
            getline(ss, val, splitter);
            node_count = stoll(val);
        } else if (attribute == "is_directed") {
            getline(ss, val, splitter);
            if (val == "true") is_directed = true;
            else if (val == "false") is_directed = false;
            else error_flag = true;
        } else if (attribute == "is_dynamic") {
        } else if (attribute == "initial_edge_count") {
        } else if (attribute == "is_weighted") {
            getline(ss, val, splitter);
            if (val == "true") error_flag = true;
            else if (val == "false");
            else error_flag = true;
        } else if (attribute == "is_bipartite") {
        } else error_flag = true;
    }
    file.close();
    assert(!error_flag);
}

// load edge list from string(getenv("HOME")) + "/dataset/" + data_dir + "/edges.txt"
// all node ids need to be within [0, n-1].
// This function creates adjacent list in adj_list_list.
void Graph::_load_edge_from_txt(map<Node, set<Node>> &adj_list_list) {
    ifstream file;
    string line;
    char splitter = ' ';
    string file_path = "./dataset/" + data_dir + "/edges.txt";
    file.open(file_path, ios::in);
    assert(file.is_open());
    string node_str;
    while (getline(file, line)) {
        stringstream ss{line};
        Node src_id, dst_id;
        getline(ss, node_str, splitter);
        src_id = stoll(node_str);
        getline(ss, node_str, splitter);
        dst_id = stoll(node_str);

        assert(src_id < node_count);
        assert(dst_id < node_count);
        if (src_id == dst_id) continue; // not accepting self-loop
        
        adj_list_list[src_id].insert(dst_id);
        if (!is_directed) adj_list_list[dst_id].insert(src_id);
    }
    file.close();
    return;
}

map<Node, double> get_normalized_map(const map<long long, double>& input_map) {
    double total_val = 0;
    map<Node, double> normalized_ppr;
    for (const auto&[node_id, val] : input_map) total_val += val;

    for (const auto&[node_id, val] : input_map) {
        normalized_ppr.emplace(node_id, val / total_val);
    }

    return normalized_ppr;
}