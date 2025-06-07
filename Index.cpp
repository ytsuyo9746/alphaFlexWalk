#include "Index.h"

struct WalkerMeta {
    long long id_;
    Node current_;
    int refer_count_;
    int current_refer_count;
};

struct BufferSlot {
    bool empty_;
    WalkerMeta w_;
    int refer_count_of_current_node;
    int index_size_of_current_node;
    long long source_start_suf;
    long long path_start_suf;
    int path_size;
    int next_path_index;
};

Index::Index(Graph& graph, double alpha_index) : graph(graph), alpha_index(alpha_index) {
    gen = mt19937(rd());
    rand_0_1 = uniform_real_distribution<>(0.0, 1.0);
    rand_int = uniform_int_distribution<>(0, INT_MAX);
}

void Index::generate_index_from_scratch(double size_ratio) {
    node_in_path_list.clear();
    path_start_suf_list.clear();
    source_start_suf_list.clear();
    
    Node node_count = graph.get_node_count();
    for (Node source_id = 0; source_id < node_count; source_id++) {
        int generate_count = _required_index_size(source_id, size_ratio);
        vector<vector<Node>> path_list;
        graph.get_paths_longer_than_1(source_id, alpha_index, generate_count, path_list);

        source_start_suf_list.push_back((long long)path_start_suf_list.size());
        for (vector<Node> path : path_list) {
            path_start_suf_list.push_back((long long)node_in_path_list.size());
            for (Node node : path) {
                node_in_path_list.push_back(node);
            }
        }
    }
    source_start_suf_list.push_back((long long)path_start_suf_list.size());
    path_start_suf_list.push_back((long long)node_in_path_list.size());
}

void Index::save_index(string file_path) const {
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open file for writing: " + file_path);
    }

    // node_in_path_list（Node = long long）
    size_t node_count = node_in_path_list.size();
    ofs.write(reinterpret_cast<const char*>(&node_count), sizeof(size_t));
    ofs.write(reinterpret_cast<const char*>(node_in_path_list.data()), node_count * sizeof(Node));

    // path_start_suf_list（long long）
    size_t path_suf_count = path_start_suf_list.size();
    ofs.write(reinterpret_cast<const char*>(&path_suf_count), sizeof(size_t));
    ofs.write(reinterpret_cast<const char*>(path_start_suf_list.data()), path_suf_count * sizeof(long long));

    // source_start_suf_list（long long）
    size_t source_suf_count = source_start_suf_list.size();
    ofs.write(reinterpret_cast<const char*>(&source_suf_count), sizeof(size_t));
    ofs.write(reinterpret_cast<const char*>(source_start_suf_list.data()), source_suf_count * sizeof(long long));
}

void Index::load_index(string file_path) {
    node_in_path_list.clear();
    path_start_suf_list.clear();
    source_start_suf_list.clear();

    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Failed to open file for reading: " + file_path);
    }

    // node_in_path_list
    size_t node_count;
    ifs.read(reinterpret_cast<char*>(&node_count), sizeof(size_t));
    node_in_path_list.resize(node_count);
    ifs.read(reinterpret_cast<char*>(node_in_path_list.data()), node_count * sizeof(Node));

    // path_start_suf_list
    size_t path_suf_count;
    ifs.read(reinterpret_cast<char*>(&path_suf_count), sizeof(size_t));
    path_start_suf_list.resize(path_suf_count);
    ifs.read(reinterpret_cast<char*>(path_start_suf_list.data()), path_suf_count * sizeof(long long));

    // source_start_suf_list
    size_t source_suf_count;
    ifs.read(reinterpret_cast<char*>(&source_suf_count), sizeof(size_t));
    source_start_suf_list.resize(source_suf_count);
    ifs.read(reinterpret_cast<char*>(source_start_suf_list.data()), source_suf_count * sizeof(long long));
}
    
void Index::get(Node source_id, vector<Node>& path) {
    int referred_count = referred_count_map[source_id];

    if (referred_count < _get_index_size_for_node(source_id)) {
        long long path_id = source_start_suf_list.at(source_id) + referred_count;
        long long path_size = path_start_suf_list.at(path_id + 1) - path_start_suf_list.at(path_id);
        long long path_start_suf = path_start_suf_list.at(path_id);
        for (long long i = 0; i < path_size; i++) {
            path.push_back(node_in_path_list.at(path_start_suf + i));
        }
        referred_count_map[source_id]++;
        return;
    } else {
        Node current_node_id = source_id;
        path.push_back(current_node_id);
        do {
            if (current_node_id == -1) {
                break;
            }
            if (referred_count_map[current_node_id] < _get_index_size_for_node(current_node_id)) {
                long long path_id = source_start_suf_list.at(current_node_id) + referred_count_map[current_node_id]++;
                long long path_size = path_start_suf_list.at(path_id + 1) - path_start_suf_list.at(path_id);
                long long path_start_suf = path_start_suf_list.at(path_id);
                path.pop_back();
                for (long long i = 0; i < path_size; i++) {
                    path.push_back(node_in_path_list.at(path_start_suf + i));
                }
                break;
            } else {
                current_node_id = graph.get_random_adjacent(current_node_id);
                path.push_back(current_node_id);
                if (current_node_id == -1) break;
            }
        } while (rand_0_1(gen) > alpha_index);
        return;
    }
    
}

void Index::get(Node source_id, int max_len, vector<Node>& path) {
    int current_path_size = 0;

    if (referred_count_map[source_id] < _get_index_size_for_node(source_id)) {
        long long path_id = source_start_suf_list.at(source_id) + referred_count_map[source_id]++;
        long long path_size = path_start_suf_list.at(path_id + 1) - path_start_suf_list.at(path_id);
        long long path_start_suf = path_start_suf_list.at(path_id);
        for (long long i = 0; i < path_size; i++) {
            path.push_back(node_in_path_list.at(path_start_suf + i));
            current_path_size++;
            if (current_path_size >= max_len) return;
        }
        return;
    } else {
        Node current_node_id = source_id;
        path.push_back(current_node_id);
        current_path_size++;
        do {
            if (current_node_id == -1) {
                break;
            }
            if (referred_count_map[current_node_id] < _get_index_size_for_node(current_node_id)) {
                long long path_id = source_start_suf_list.at(current_node_id) + referred_count_map[current_node_id]++;
                long long path_size = path_start_suf_list.at(path_id + 1) - path_start_suf_list.at(path_id);
                long long path_start_suf = path_start_suf_list.at(path_id);
                path.pop_back();
                for (long long i = 0; i < path_size; i++) {
                    path.push_back(node_in_path_list.at(path_start_suf + i));
                    current_path_size++;
                    if (current_path_size >= max_len) break;
                }
                break;
            } else {
                current_node_id = graph.get_random_adjacent(current_node_id);
                path.push_back(current_node_id);
                current_path_size++;
                if (current_node_id == -1 || current_path_size >= max_len) break;
            }
        } while (rand_0_1(gen) > alpha_index);
        return;
    }
}

void Index::_get_paths(Node source_id, long long walk_count, double alpha, vector<vector<Node>>& paths) {
    paths.resize(walk_count);

    binomial_distribution<> bin_dist(walk_count, alpha);
    default_random_engine engine(seed_gen());
    const long long length_1_count = bin_dist(engine);

    if (alpha < alpha_index) {
        long long completed_walker_count = 0;
        const double accept_prob = alpha / alpha_index;
        GeometricDistribution geo_dist_downscale(accept_prob);

        vector<WalkerMeta> walkers;
        for (long long i = 0; i < walk_count - length_1_count; i++) {
            int refer_count = geo_dist_downscale.get() + 1;
            get(source_id, paths.at(i));
            if (refer_count >= 2) {
                walkers.push_back({i, paths.at(i).back(), refer_count, 1});
            } else completed_walker_count++;
        }

        BufferSlot ring[ring_size];
        long long walkers_next_suf = 0;
        long long walkers_size = walkers.size();
        for (int i = 0; i < ring_size; ++i) {
            if (walkers_next_suf < walkers_size) {
                ring[i].empty_ = false;
                ring[i].w_ = walkers[walkers_next_suf++];
            } else {
                ring[i].empty_ = true;
            }
        }

        while (completed_walker_count < walk_count - length_1_count) {
            // Stage 1: prefetch referred count.
            for (int i = 0; i < ring_size; ++i) {
                BufferSlot& slot = ring[i];
                if (!slot.empty_) {
                    if (slot.w_.current_ == -1) {
                        slot.empty_ = true;
                        completed_walker_count++;
                    } else {   
                        _mm_prefetch((void*)(&referred_count_map[slot.w_.current_]), PREFETCH_HINT);
                    }
                }
            }

            // Stage 2: prefetch index size.
            for (int i = 0; i < ring_size; ++i) {
                BufferSlot& slot = ring[i];
                if (!slot.empty_) {
                    slot.refer_count_of_current_node = referred_count_map[slot.w_.current_]++;
                    _mm_prefetch((void*)(source_start_suf_list.data() + slot.w_.current_), PREFETCH_HINT);
                }
            }

            // Stage 3: prefetch suffix in node_in_path_list.
            for (int i = 0; i < ring_size; ++i) {
                BufferSlot& slot = ring[i];
                if (!slot.empty_) {
                    slot.source_start_suf = source_start_suf_list[slot.w_.current_];
                    slot.index_size_of_current_node = source_start_suf_list[slot.w_.current_ + 1] - source_start_suf_list[slot.w_.current_]; 
                    if (slot.refer_count_of_current_node < slot.index_size_of_current_node) {
                        _mm_prefetch((void*)(path_start_suf_list.data() + source_start_suf_list[slot.w_.current_] + slot.refer_count_of_current_node), PREFETCH_HINT);
                    }
                }
            }

            // Stage 4: prefetch path.
            for (int i = 0; i < ring_size; ++i) {
                BufferSlot& slot = ring[i];
                if (!slot.empty_) {
                    if (slot.refer_count_of_current_node < slot.index_size_of_current_node) {
                        slot.path_start_suf = path_start_suf_list[slot.source_start_suf + slot.refer_count_of_current_node]; 
                        slot.path_size = path_start_suf_list[slot.source_start_suf + slot.refer_count_of_current_node + 1] - path_start_suf_list[slot.source_start_suf + slot.refer_count_of_current_node];
                        _mm_prefetch((void*)(node_in_path_list.data() + slot.path_start_suf), PREFETCH_HINT);
                    }
                }
            }

            // Stage 5: update the walker.
            for (int i = 0; i < ring_size; ++i) {
                BufferSlot& slot = ring[i];
                if (!slot.empty_) {
                    paths.at(slot.w_.id_).pop_back();

                    if (slot.refer_count_of_current_node < slot.index_size_of_current_node) {
                        for (int j = 0; j < slot.path_size; j++) {
                            paths.at(slot.w_.id_).push_back(node_in_path_list[slot.path_start_suf + j]);
                        }
                    } else {
                        get(slot.w_.current_, paths.at(slot.w_.id_));
                    }
                    
                    slot.w_.current_refer_count++;
                    if (slot.w_.current_refer_count >= slot.w_.refer_count_) {
                        slot.empty_ = true;
                        completed_walker_count++;
                    } else {
                        slot.w_.current_ = paths.at(slot.w_.id_).back();
                    }
                }

                if (slot.empty_) {
                    if (walkers_next_suf < walkers_size) {
                        slot.empty_ = false;
                        slot.w_ = walkers[walkers_next_suf++];
                    }
                }
            }
        }
    } else if (alpha > alpha_index) {
        const double terminate_prob = (alpha - alpha_index) / (1 - alpha_index);
        GeometricDistribution geo_dist_upscale(terminate_prob);
        
        for (long long i = 0; i < walk_count - length_1_count; i++) {
            int max_len = geo_dist_upscale.get() + 2;
            get(source_id, max_len, paths.at(i));
        }
    } else {
        for (long long i = 0; i < walk_count - length_1_count; i++) get(source_id, paths.at(i));
    }
    
    for (long long i = walk_count - length_1_count; i < walk_count; i++) paths.at(i).push_back(source_id);

    return;
}

void Index::calc_ppr_by_fora_plus(const map<Node, double>& src_map, double alpha, long long walk_count, unordered_map<Node, double>& ppr, bool enable_thunder) {
    assert(alpha > 0 && alpha <= 1);
    reset_referred_count_map();
    map<Node, double> normalized_src_map = get_normalized_map(src_map);
    unordered_map<Node, double> residue;
    graph.calc_ppr_by_fp(src_map, alpha, walk_count, residue, ppr);
    residue.erase(-1);
    ppr.erase(-1);

    for (const auto&[node_id, r_val] : residue) {
        if (r_val == 0) continue;
        
        long long walk_count_i = (long long)ceil(r_val * walk_count);
        vector<vector<Node>> paths;
        _get_paths(node_id, walk_count_i, alpha, paths);
        
        for (vector<Node> path : paths) {
            ppr[path.back()] += (double)r_val / walk_count_i;
        }
    }
}

void Index::show_index() const {
    // cout << "node_in_path_list" << endl;
    // for (Node node_id : node_in_path_list) {
    //     cout << node_id << " ";
    // }
    // cout << endl << endl;

    // cout << "path_start_suf_list" << endl;
    // for (long long path_start_suf : path_start_suf_list) {
    //     cout << path_start_suf << " ";
    // }
    // cout << endl << endl;

    // cout << "source_start_suf_list" << endl;
    // for (long long source_start_suf : source_start_suf_list) {
    //     cout << source_start_suf << " ";
    // }
    // cout << endl << endl;

    long long node_count = graph.get_node_count();
    for (long long node_id = 0; node_id < node_count; node_id++) {
        cout << node_id << endl;
        long long path_count = source_start_suf_list.at(node_id + 1) - source_start_suf_list.at(node_id);
        for (long long path_suf = 0; path_suf < path_count; path_suf++) {
            long long node_suf = path_start_suf_list.at(source_start_suf_list.at(node_id) + path_suf);
            int path_size = path_start_suf_list.at(source_start_suf_list.at(node_id) + path_suf + 1) - path_start_suf_list.at(source_start_suf_list.at(node_id) + path_suf);
            for (int i = 0; i < path_size; i++) {
                cout << node_in_path_list.at(node_suf + i) << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
}