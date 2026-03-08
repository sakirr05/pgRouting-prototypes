// Compile: g++ -std=c++17 -Wall -Wextra test_stoer_wagner.cpp -o test_stoer_wagner -lboost_graph
//
// Tests for Stoer-Wagner min-cut prototype. Each test prints name then PASS or FAIL.

#include <iostream>
#include <vector>
#include <set>
#include <cstdlib>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/one_bit_color_map.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/property_map/property_map.hpp>

using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    boost::no_property,
    boost::property<boost::edge_weight_t, double>>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using EdgeIter = boost::graph_traits<Graph>::edge_iterator;

static bool test_basic_cut_weight() {
    // Test 1: 8-vertex graph, run stoer_wagner_min_cut, verify cut weight == 3.
    Graph g(8);
    auto wm = get(boost::edge_weight, g);
    auto add = [&g, &wm](unsigned u, unsigned v, double w) {
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        put(wm, e, w);
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

    auto parities = boost::make_one_bit_color_map(num_vertices(g), get(boost::vertex_index, g));
    double actual = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::parity_map(parities));
    const double expected = 3.0;  // Known min-cut for this graph (cut isolating vertex 3).
    if (actual != expected)
        std::cout << "  FAIL reason: min-cut weight mismatch. Expected: " << expected << ", actual: " << actual << "\n";
    else
        std::cout << "  Expected min-cut weight: " << expected << ", actual: " << actual << "\n";
    return actual == expected;
}

static bool test_partition_validity() {
    Graph g(8);
    auto wm = get(boost::edge_weight, g);
    auto add = [&g, &wm](unsigned u, unsigned v, double w) {
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        put(wm, e, w);
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

    auto parities = boost::make_one_bit_color_map(num_vertices(g), get(boost::vertex_index, g));
    boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::parity_map(parities));

    std::set<Vertex> inA, inB;
    for (auto [vi, vi_end] = vertices(g); vi != vi_end; ++vi) {
        if (get(parities, *vi)) inA.insert(*vi);
        else inB.insert(*vi);
    }
    bool disjoint = true;
    for (Vertex v : inA) if (inB.count(v)) disjoint = false;
    bool cover = (inA.size() + inB.size() == num_vertices(g));
    if (!disjoint) std::cout << "  FAIL reason: vertex in both partitions. A size=" << inA.size() << " B size=" << inB.size() << "\n";
    if (!cover) std::cout << "  FAIL reason: partitions do not cover all vertices. Expected 8, actual A+B=" << (inA.size() + inB.size()) << "\n";
    return disjoint && cover;
}

static bool test_cut_edge_validity() {
    Graph g(8);
    auto wm = get(boost::edge_weight, g);
    auto add = [&g, &wm](unsigned u, unsigned v, double w) {
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        put(wm, e, w);
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

    auto parities = boost::make_one_bit_color_map(num_vertices(g), get(boost::vertex_index, g));
    boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::parity_map(parities));

    // Verify every edge in the cut set has one endpoint in partition A and one in partition B.
    bool all_cross = true;
    EdgeIter ei, ei_end;
    for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
        Vertex u = source(*ei, g);
        Vertex v = target(*ei, g);
        if (get(parities, u) != get(parities, v)) {
            // This edge is in the cut; one must be A (true) and one B (false).
            bool u_in_A = get(parities, u);
            bool v_in_A = get(parities, v);
            if (u_in_A == v_in_A) {
                all_cross = false;
                std::cout << "  FAIL reason: cut edge " << u << "-" << v << " has both endpoints in same partition (u_in_A=" << u_in_A << " v_in_A=" << v_in_A << ")\n";
            }
        }
    }
    return all_cross;
}

static bool test_single_edge_graph() {
    // Test 4: 2-vertex graph with one edge weight 5; verify cut weight 5 and partitions {0}, {1}.
    Graph g(2);
    auto wm = get(boost::edge_weight, g);
    auto e = add_edge(static_cast<Vertex>(0), static_cast<Vertex>(1), g).first;
    put(wm, e, 5.0);

    auto parities = boost::make_one_bit_color_map(num_vertices(g), get(boost::vertex_index, g));
    double w = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::parity_map(parities));
    bool weight_ok = (w == 5.0);
    bool p0 = get(parities, static_cast<Vertex>(0));
    bool p1 = get(parities, static_cast<Vertex>(1));
    bool parts_ok = (p0 != p1);  // One in A, one in B.
    if (!weight_ok) std::cout << "  FAIL reason: expected cut weight 5, actual " << w << "\n";
    if (!parts_ok) std::cout << "  FAIL reason: expected partitions {0} and {1}, got same parity for both\n";
    return weight_ok && parts_ok;
}

static bool test_disconnected_resilience() {
    // Test 5: Path 0-1-2-3 with weights 2,1,2; min-cut weight must be 1 (weakest link is edge 1-2).
    Graph g(4);
    auto wm = get(boost::edge_weight, g);
    auto add = [&g, &wm](unsigned u, unsigned v, double w) {
        auto e = add_edge(static_cast<Vertex>(u), static_cast<Vertex>(v), g).first;
        put(wm, e, w);
    };
    add(0, 1, 2);
    add(1, 2, 1);  // weakest link
    add(2, 3, 2);

    auto parities = boost::make_one_bit_color_map(num_vertices(g), get(boost::vertex_index, g));
    double w = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::parity_map(parities));
    if (w != 1.0)
        std::cout << "  FAIL reason: expected min-cut 1 (weakest link), actual: " << w << "\n";
    else
        std::cout << "  Path 0-1-2-3 (weights 2,1,2); expected min-cut 1, actual: " << w << "\n";
    return w == 1.0;
}

int main() {
    int failed = 0;

    std::cout << "Test 1 — Basic cut weight: ";
    if (test_basic_cut_weight()) std::cout << "PASS\n";
    else { std::cout << "FAIL (min-cut weight mismatch)\n"; ++failed; }

    std::cout << "Test 2 — Partition validity: ";
    if (test_partition_validity()) std::cout << "PASS\n";
    else { std::cout << "FAIL (partitions must contain all vertices exactly once)\n"; ++failed; }

    std::cout << "Test 3 — Cut edge validity: ";
    if (test_cut_edge_validity()) std::cout << "PASS\n";
    else { std::cout << "FAIL (cut edges must cross partition)\n"; ++failed; }

    std::cout << "Test 4 — Single edge graph: ";
    if (test_single_edge_graph()) std::cout << "PASS\n";
    else { std::cout << "FAIL (weight 5, partitions {0} and {1})\n"; ++failed; }

    std::cout << "Test 5 — Disconnected resilience: ";
    if (test_disconnected_resilience()) std::cout << "PASS\n";
    else { std::cout << "FAIL (weakest link cut)\n"; ++failed; }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
