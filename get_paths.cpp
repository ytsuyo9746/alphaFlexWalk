#include "Graph.h"
#include "Index.h"
#include <iostream>
#include <chrono>

int main(int argc, char *argv[]) {
    const string data_dir = argv[1];
    double alpha_index = 0.4;
    Graph::Node source_id = 0;
    vector<double> alpha_list{0.1, 0.4, 0.7};
    const int walk_count = 10;
    
    /* Initializing Graph Phase */
    Graph graph(data_dir);
    Index index(graph, alpha_index);
    index.generate_index_from_scratch(1.0);
    cout << "Index for alpha_index = " << alpha_index <<  endl;
    index.show_index();

    cout << "Start Query" << endl;
    for (double alpha : alpha_list) {
        vector<vector<Graph::Node>> paths;
        index.get_paths(source_id, walk_count, alpha, paths);
        cout << "Paths for alpha = " << alpha << endl;
        for (vector<Graph::Node> path : paths) {
            for (Graph::Node node : path) {
                cout << node << " ";
            }
            cout << endl;
        }
        cout << endl;
    }

    return 0;
}