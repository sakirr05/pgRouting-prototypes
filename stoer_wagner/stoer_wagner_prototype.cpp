// Compile: g++ -std=c++17 -Wall -Wextra stoer_wagner_prototype.cpp -o stoer_wagner_prototype -lboost_graph
//
// Standalone prototype for pgr_stoerWagner: minimum cut via Boost's Stoer-Wagner algorithm.
// Uses a hardcoded 8-vertex road network; prints min-cut weight, partitions, and cut edges.

#include <iostream>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/one_bit_color_map.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/property_map/property_map.hpp>

// BGL adjacency_list: vecS vertex/edge containers, undirectedS, edge weight stored as property.
// Named edge weight property map is required by stoer_wagner_min_cut.
using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    boost::no_property,
    boost::property<boost::edge_weight_t, double>>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using Edge = boost::graph_traits<Graph>::edge_descriptor;
using EdgeIter = boost::graph_traits<Graph>::edge_iterator;
using WeightMap = boost::property_map<Graph, boost::edge_weight_t>::type;

// Build the 8-vertex road network: intersections 0-7 with given capacities.
static Graph build_road_network() {
    // BGL adjacency_list constructor: create graph with 8 vertices (vecS vertex list).
    Graph g(8);
    // BGL property_map: get the edge weight property map for reading/writing weights.
    WeightMap wm = get(boost::edge_weight, g);

    auto add = [&g, &wm](unsigned u, unsigned v, double weight) {
        // BGL add_edge: add an edge (u, v) and return (edge_descriptor, bool); we use .first.
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        // BGL put: assign the weight to edge e in the weight property map.
        put(wm, e, weight);
    };

    add(0, 1, 4);
    add(0, 4, 3);
    add(1, 2, 3);
    add(1, 4, 2);
    add(1, 5, 2);
    add(2, 3, 1);
    add(2, 6, 2);
    add(3, 7, 2);
    add(4, 5, 3);
    add(5, 6, 2);
    add(6, 7, 3);

    return g;
}

int main() {
    Graph g = build_road_network();
    // BGL num_vertices(g): returns the number of vertices in the graph.
    const auto n = num_vertices(g);

    if (n < 2) {
        std::cout << "Graph has fewer than 2 vertices; no min-cut defined.\n";
        return 0;
    }

    // ---- Print graph edges and weights ----
    std::cout << "=== Graph edges and weights (road network) ===\n";
    WeightMap wm = get(boost::edge_weight, g);
    EdgeIter ei, ei_end;
    // BGL edges(g): returns (begin, end) iterator pair over all edges.
    for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
        // BGL source/target: get the two endpoints of the edge.
        Vertex u = source(*ei, g);
        Vertex v = target(*ei, g);
        // BGL get(wm, e): read the edge weight from the weight property map.
        std::cout << "  " << u << "-" << v << ": " << get(wm, *ei) << "\n";
    }

    // BGL one_bit_color_map: stores partition (A/B) per vertex; vertex_index map required.
    auto parities = boost::make_one_bit_color_map(n, get(boost::vertex_index, g));

    // BGL stoer_wagner_min_cut: computes global min-cut weight and assigns parities so
    // that edges with get(parities, u) != get(parities, v) are exactly the cut edges.
    double min_cut_weight = boost::stoer_wagner_min_cut(
        g,
        get(boost::edge_weight, g),
        boost::parity_map(parities));

    std::cout << "\nMinimum cut weight: " << min_cut_weight << "\n";

    // Collect partition A (parity true) and B (parity false).
    std::vector<int> partitionA, partitionB;
    // BGL vertices(g): returns (begin, end) iterator pair over all vertices.
    for (boost::graph_traits<Graph>::vertex_iterator vi = vertices(g).first;
         vi != vertices(g).second; ++vi) {
        // BGL get(parities, v): read the partition (true = A, false = B) for vertex v.
        if (get(parities, *vi))
            partitionA.push_back(static_cast<int>(*vi));
        else
            partitionB.push_back(static_cast<int>(*vi));
    }

    std::cout << "Partition A vertices: ";
    for (size_t i = 0; i < partitionA.size(); ++i)
        std::cout << (i ? ", " : "") << partitionA[i];
    std::cout << "\n";
    std::cout << "Partition B vertices: ";
    for (size_t i = 0; i < partitionB.size(); ++i)
        std::cout << (i ? ", " : "") << partitionB[i];
    std::cout << "\n";

    // Edges that cross the partition (the cut): parity differs at the two endpoints.
    std::cout << "Edges in the cut (crossing the partition): ";
    std::vector<std::pair<int, int>> cut_edges;
    for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
        Vertex u = source(*ei, g);
        Vertex v = target(*ei, g);
        // Cut edge: one endpoint in A, one in B (parities differ).
        if (get(parities, u) != get(parities, v)) {
            int a = static_cast<int>(u);
            int b = static_cast<int>(v);
            if (a > b) std::swap(a, b);
            cut_edges.push_back({a, b});
            std::cout << "(" << u << "-" << v << ", weight=" << get(wm, *ei) << ") ";
        }
    }
    std::cout << "\n";

    std::cout << "Removing these " << cut_edges.size()
              << " road(s) would disconnect the network.\n";
    std::cout << "Interpretation: min-cut weight = " << min_cut_weight << ".\n";

    return 0;
}
