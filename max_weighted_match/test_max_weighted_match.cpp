// Compile: g++ -std=c++17 -Wall -Wextra test_max_weighted_match.cpp -o test_max_weighted_match -lboost_graph
//
#include <boost/version.hpp>
#if BOOST_VERSION < 107600
#error "This prototype requires Boost >= 1.76.0."
#endif

// Tests for maximum weighted matching prototype. Each test prints name then PASS or FAIL.

#include <iostream>
#include <vector>
#include <set>
#include <cstdlib>
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

// Test 1: Optimal weight 9+8+7+6 = 30 for vehicle-pickup graph
static bool test_optimal_weight() {
    Graph g = build_vehicle_pickup_graph();
    const auto n = num_vertices(g);
    std::vector<Vertex> mate(n, boost::graph_traits<Graph>::null_vertex());
    boost::maximum_weighted_matching(g, &mate[0]);
    double sum = 0;
    auto wm = get(boost::edge_weight, g);
    for (Vertex v = 0; v < n; ++v) {
        Vertex m = mate[v];
        if (m != boost::graph_traits<Graph>::null_vertex() && v < m) {
            auto [e, found] = edge(v, m, g);
            if (found) sum += get(wm, e);
        }
    }
    const double expected = 30.0;  // 9+8+7+6
    std::cout << "  Expected total weight: " << expected << ", actual: " << sum << "\n";
    if (sum != expected)
        std::cout << "  FAIL reason: total weight mismatch. Expected: " << expected << ", actual: " << sum << "\n";
    return sum == expected;
}

// Test 2: No vertex in more than one matched pair; for every matched v, mate[mate[v]] == v
static bool test_matching_validity() {
    Graph g = build_vehicle_pickup_graph();
    const auto n = num_vertices(g);
    std::vector<Vertex> mate(n, boost::graph_traits<Graph>::null_vertex());
    boost::maximum_weighted_matching(g, &mate[0]);
    for (Vertex v = 0; v < n; ++v) {
        if (mate[v] != boost::graph_traits<Graph>::null_vertex()) {
            if (mate[mate[v]] != v) {
                std::cout << "  FAIL reason: mate[" << v << "]=" << mate[v] << " but mate[" << mate[v] << "]!=" << v << "\n";
                return false;
            }
        }
    }
    return true;
}

// Test 3: Complete bipartite K(3,3): left 0,1,2 right 3,4,5 — all 6 vertices matched (perfect matching)
static bool test_all_vertices_covered() {
    Graph g(6);  // K(3,3): vertices 0,1,2 | 3,4,5
    auto wm = get(boost::edge_weight, g);
    auto add = [&g, &wm](int u, int v, double w) {
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        put(wm, e, w);
    };
    add(0, 3, 1);
    add(0, 4, 1);
    add(0, 5, 1);
    add(1, 3, 1);
    add(1, 4, 1);
    add(1, 5, 1);
    add(2, 3, 1);
    add(2, 4, 1);
    add(2, 5, 1);
    const auto n = num_vertices(g);
    std::vector<Vertex> mate(n, boost::graph_traits<Graph>::null_vertex());
    boost::maximum_weighted_matching(g, &mate[0]);
    int matched = 0;
    for (Vertex v = 0; v < n; ++v)
        if (mate[v] != boost::graph_traits<Graph>::null_vertex()) matched++;
    std::cout << "  Matched vertex count: " << matched << " (expected " << n << ")\n";
    if (matched != static_cast<int>(n))
        std::cout << "  FAIL reason: K(3,3) should have perfect matching; expected 6 matched, got " << matched << "\n";
    return matched == static_cast<int>(n);
}

// Test 4: Two vertices, one edge of weight 5 — mate[0]==1, mate[1]==0, total weight 5
static bool test_single_edge() {
    Graph g(2);
    auto e = add_edge(0, 1, g).first;
    put(get(boost::edge_weight, g), e, 5.0);
    std::vector<Vertex> mate(2, boost::graph_traits<Graph>::null_vertex());
    boost::maximum_weighted_matching(g, &mate[0]);
    bool both_matched = (mate[0] == static_cast<Vertex>(1) && mate[1] == static_cast<Vertex>(0));
    auto wm = get(boost::edge_weight, g);
    double w = get(wm, edge(0, 1, g).first);
    std::cout << "  mate[0]=" << mate[0] << " mate[1]=" << mate[1] << ", total weight: " << w << " (expected 5)\n";
    if (!both_matched)
        std::cout << "  FAIL reason: expected mate[0]==1 and mate[1]==0\n";
    if (w != 5.0)
        std::cout << "  FAIL reason: expected weight 5, got " << w << "\n";
    return both_matched && w == 5.0;
}

// Test 5: Graph where greedy gets 11 but optimal is 15.
// Vertices 0,1,2,3; edges 0-1:10, 0-2:8, 1-3:7, 2-3:1.
// Greedy: pick 0-1 (10), then only 2-3 (1) = 11. Optimal: 0-2 (8) + 1-3 (7) = 15.
static bool test_weight_preference() {
    Graph g(4);
    auto wm = get(boost::edge_weight, g);
    auto add = [&g, &wm](int u, int v, double w) {
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        put(wm, e, w);
    };
    add(0, 1, 10);
    add(0, 2, 8);
    add(1, 3, 7);
    add(2, 3, 1);
    const auto n = num_vertices(g);
    std::vector<Vertex> mate(n, boost::graph_traits<Graph>::null_vertex());
    boost::maximum_weighted_matching(g, &mate[0]);
    double sum = 0;
    for (Vertex v = 0; v < n; ++v) {
        Vertex m = mate[v];
        if (m != boost::graph_traits<Graph>::null_vertex() && v < m) {
            auto [e, found] = edge(v, m, g);
            if (found) sum += get(wm, e);
        }
    }
    const double expected_optimal = 15.0;
    std::cout << "  Total weight: " << sum << " (expected optimal " << expected_optimal << "; greedy would get 11)\n";
    if (sum != expected_optimal)
        std::cout << "  FAIL reason: algorithm should get optimal 15, got " << sum << "\n";
    return sum == expected_optimal;
}

int main() {
    int failed = 0;

    std::cout << "Test 1 — Optimal weight: ";
    if (test_optimal_weight()) std::cout << "PASS\n";
    else { std::cout << "FAIL (total weight mismatch)\n"; ++failed; }

    std::cout << "Test 2 — Matching validity: ";
    if (test_matching_validity()) std::cout << "PASS\n";
    else { std::cout << "FAIL (vertex in more than one pair)\n"; ++failed; }

    std::cout << "Test 3 — All vertices covered: ";
    if (test_all_vertices_covered()) std::cout << "PASS\n";
    else { std::cout << "FAIL (not all matched)\n"; ++failed; }

    std::cout << "Test 4 — Single edge: ";
    if (test_single_edge()) std::cout << "PASS\n";
    else { std::cout << "FAIL (both should be matched, weight 5)\n"; ++failed; }

    std::cout << "Test 5 — Weight preference: ";
    if (test_weight_preference()) std::cout << "PASS\n";
    else { std::cout << "FAIL (algorithm should pick globally optimal 12)\n"; ++failed; }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
