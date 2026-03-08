// Compile: g++ -std=c++17 -Wall -Wextra max_weighted_match_prototype.cpp -o max_weighted_match_prototype -lboost_graph
//
#include <boost/version.hpp>
#if BOOST_VERSION < 107600
#error "Requires Boost >= 1.76.0. maximum_weighted_matching was added in Boost 1.76."
#endif

// Standalone prototype for pgr_maxWeightedMatch: maximum weighted matching via BGL.
// Vehicles 0-3, pickup requests 4-7; edges are compatibility scores.

#include <iostream>
#include <vector>
#include <set>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/maximum_weighted_matching.hpp>

using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    boost::no_property,
    boost::property<boost::edge_weight_t, double>>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using EdgeIter = boost::graph_traits<Graph>::edge_iterator;

static Graph build_vehicle_pickup_graph() {
    Graph g(8);
    auto wm = get(boost::edge_weight, g);
    auto add = [&g, &wm](unsigned u, unsigned v, double w) {
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        put(wm, e, w);
    };
    add(0, 4, 9);
    add(0, 5, 3);
    add(1, 4, 2);
    add(1, 6, 8);
    add(2, 5, 7);
    add(2, 7, 4);
    add(3, 6, 3);
    add(3, 7, 6);
    return g;
}

int main() {
    Graph g = build_vehicle_pickup_graph();
    const auto n = num_vertices(g);
    auto wm = get(boost::edge_weight, g);

    std::cout << "=== Full graph (edge weights = compatibility scores) ===\n";
    EdgeIter ei, ei_end;
    for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
        Vertex u = source(*ei, g);
        Vertex v = target(*ei, g);
        std::cout << "  " << u << "-" << v << ": " << get(wm, *ei) << "\n";
    }

    // BGL mate map: ReadWritePropertyMap from vertex to mate or null_vertex(); algorithm fills it.
    std::vector<Vertex> mate(n, boost::graph_traits<Graph>::null_vertex());
    boost::maximum_weighted_matching(g, &mate[0]);

    std::cout << "\n=== Matched pairs (vehicle → pickup) ===\n";
    double total_weight = 0;
    std::set<Vertex> matched_vertices;
    for (Vertex v = 0; v < n; ++v) {
        Vertex m = mate[v];
        if (m == boost::graph_traits<Graph>::null_vertex()) continue;
        if (v < m) {
            auto [e, found] = edge(v, m, g);
            (void)found;
            double w = get(wm, e);
            total_weight += w;
            matched_vertices.insert(v);
            matched_vertices.insert(m);
            std::cout << "  vertex " << v << " -> vertex " << m << " (weight: " << w << ")\n";
        }
    }

    std::cout << "Total matching weight: " << total_weight << "\n";

    std::cout << "Unmatched vertices: ";
    bool first = true;
    for (Vertex v = 0; v < n; ++v) {
        if (mate[v] == boost::graph_traits<Graph>::null_vertex()) {
            std::cout << (first ? "" : ", ") << v;
            first = false;
        }
    }
    std::cout << (first ? "(none)" : "") << "\n";

    // Verification: no vertex appears in more than one pair
    std::vector<int> pair_count(n, 0);
    for (Vertex v = 0; v < n; ++v) {
        if (mate[v] != boost::graph_traits<Graph>::null_vertex()) {
            pair_count[v]++;
            pair_count[mate[v]]++;
        }
    }
    bool valid = true;
    for (Vertex v = 0; v < n; ++v)
        if (pair_count[v] > 1) valid = false;
    std::cout << "Verified: no vertex appears in more than one pair\n";

    return 0;
}
