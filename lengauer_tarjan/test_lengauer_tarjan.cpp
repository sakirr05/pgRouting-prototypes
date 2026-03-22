// Compile: g++ -std=c++17 -Wall -Wextra test_lengauer_tarjan.cpp -o test_lengauer_tarjan
//
// Tests for Lengauer-Tarjan dominator tree prototype. Each test prints name then PASS or FAIL.

#include <iostream>
#include <vector>
#include <cstdlib>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/dominator_tree.hpp>
#include <boost/property_map/property_map.hpp>

using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::bidirectionalS>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using GraphTraits = boost::graph_traits<Graph>;

// Helper: run lengauer_tarjan_dominator_tree and return the dominator vector.
static std::vector<Vertex> compute_dom(const Graph& g, Vertex source) {
    std::vector<Vertex> domTree(num_vertices(g), GraphTraits::null_vertex());
    auto domMap = boost::make_iterator_property_map(
        domTree.begin(), get(boost::vertex_index, g));
    boost::lengauer_tarjan_dominator_tree(g, source, domMap);
    return domTree;
}

static bool test_source_dominates_itself() {
    // Test 1: source node is the dominator tree root.
    // Boost convention: idom[source] == null_vertex() (no parent above the root).
    // Additionally verify source directly dominates its successors (dom[1] == source,
    // dom[2] == source) — confirming source is recognised as the root dominator.
    Graph g(7);
    add_edge(0, 1, g); add_edge(0, 2, g);
    add_edge(1, 3, g); add_edge(2, 3, g);
    add_edge(3, 4, g); add_edge(4, 5, g); add_edge(4, 6, g);
    const Vertex source = 0;
    auto dom = compute_dom(g, source);
    // Root has no dominator above it (null_vertex sentinel).
    bool root_ok = (dom[source] == GraphTraits::null_vertex());
    // Source directly dominates vertex 1 and vertex 2 (they are only reachable from source).
    bool child_ok = (dom[1] == source) && (dom[2] == source);
    bool ok = root_ok && child_ok;
    if (!root_ok)
        std::cout << "  FAIL reason: dom[source]=" << dom[source]
                  << " expected null_vertex() (tree root)\n";
    if (!child_ok)
        std::cout << "  FAIL reason: dom[1]=" << dom[1]
                  << " dom[2]=" << dom[2] << " both expected " << source << "\n";
    if (ok)
        std::cout << "  dom[source]=null (tree root), dom[1]=dom[2]=" << source
                  << " (source directly dominates successors)\n";
    return ok;
}

static bool test_known_dominator() {
    // Test 2: on the 7-node road network, vertex 3 (Bridge) must be idom of vertex 4 (Downtown).
    // All paths from 0 to 4: 0->1->3->4 and 0->2->3->4 — both must cross vertex 3.
    Graph g(7);
    add_edge(0, 1, g); add_edge(0, 2, g);
    add_edge(1, 3, g); add_edge(2, 3, g);
    add_edge(3, 4, g); add_edge(4, 5, g); add_edge(4, 6, g);
    auto dom = compute_dom(g, static_cast<Vertex>(0));
    bool ok = (dom[4] == static_cast<Vertex>(3));
    if (!ok)
        std::cout << "  FAIL reason: dom[4]=" << dom[4] << " expected 3 (Bridge)\n";
    else
        std::cout << "  dom[4] (Downtown Hub) = " << dom[4] << " (Bridge) — correct\n";
    return ok;
}

static bool test_unreachable_vertex() {
    // Test 3: vertex with no path from source must get GraphTraits::null_vertex() as dominator.
    // Graph: 0->1->2 with isolated vertex 3 (no edges from {0,1,2} reach it).
    Graph g(4);
    add_edge(0, 1, g);
    add_edge(1, 2, g);
    // vertex 3 is isolated — unreachable from 0.
    auto dom = compute_dom(g, static_cast<Vertex>(0));
    bool ok = (dom[3] == GraphTraits::null_vertex());
    if (!ok)
        std::cout << "  FAIL reason: dom[3]=" << dom[3]
                  << " expected null_vertex()\n";
    else
        std::cout << "  dom[3] = null_vertex() — unreachable vertex correctly identified\n";
    return ok;
}

static bool test_linear_chain() {
    // Test 4: linear chain 0->1->2->3->4.
    // Boost convention: idom[0] = null_vertex() (source is root).
    // For i > 0: idom[i] = i-1 (only one path to each vertex).
    Graph g(5);
    add_edge(0, 1, g); add_edge(1, 2, g); add_edge(2, 3, g); add_edge(3, 4, g);
    auto dom = compute_dom(g, static_cast<Vertex>(0));
    bool ok = true;
    // Source (0) is root: null_vertex sentinel.
    if (dom[0] != GraphTraits::null_vertex()) {
        ok = false;
        std::cout << "  FAIL: dom[0]=" << dom[0] << " expected null_vertex() (root)\n";
    }
    // Each subsequent vertex's idom is the previous vertex in the chain.
    if (dom[1] != static_cast<Vertex>(0)) {
        ok = false;
        std::cout << "  FAIL: dom[1]=" << dom[1] << " expected 0\n";
    }
    if (dom[2] != static_cast<Vertex>(1)) {
        ok = false;
        std::cout << "  FAIL: dom[2]=" << dom[2] << " expected 1\n";
    }
    if (dom[3] != static_cast<Vertex>(2)) {
        ok = false;
        std::cout << "  FAIL: dom[3]=" << dom[3] << " expected 2\n";
    }
    if (dom[4] != static_cast<Vertex>(3)) {
        ok = false;
        std::cout << "  FAIL: dom[4]=" << dom[4] << " expected 3\n";
    }
    if (ok)
        std::cout << "  Chain 0->1->2->3->4: idom[0]=null (root), idom[i]=i-1 for i>0\n";
    return ok;
}

static bool test_bottleneck_vertex() {
    // Test 5: graph with a known bottleneck vertex (2) that all paths to 4 and 5 must pass.
    // Graph: 0->1->2, 0->3->2, 2->4, 2->5.
    // Two routes to vertex 2 (via 1 or via 3), but 4 and 5 are only reachable through 2.
    // Expected: idom[4] = 2, idom[5] = 2 (vertex 2 is the bottleneck for 4 and 5).
    Graph g(6);
    add_edge(0, 1, g); add_edge(0, 3, g);
    add_edge(1, 2, g); add_edge(3, 2, g);
    add_edge(2, 4, g); add_edge(2, 5, g);
    auto dom = compute_dom(g, static_cast<Vertex>(0));
    bool ok = true;
    if (dom[4] != static_cast<Vertex>(2)) {
        ok = false;
        std::cout << "  FAIL: dom[4]=" << dom[4] << " expected 2\n";
    }
    if (dom[5] != static_cast<Vertex>(2)) {
        ok = false;
        std::cout << "  FAIL: dom[5]=" << dom[5] << " expected 2\n";
    }
    if (ok)
        std::cout << "  Bottleneck vertex 2: dom[4]=" << dom[4]
                  << ", dom[5]=" << dom[5] << " (both = 2)\n";
    return ok;
}

int main() {
    int failed = 0;

    std::cout << "Test 1 — Source dominates itself: ";
    if (test_source_dominates_itself()) std::cout << "PASS\n";
    else { std::cout << "FAIL\n"; ++failed; }

    std::cout << "Test 2 — Known dominator relationship: ";
    if (test_known_dominator()) std::cout << "PASS\n";
    else { std::cout << "FAIL\n"; ++failed; }

    std::cout << "Test 3 — Unreachable vertex gets null dominator: ";
    if (test_unreachable_vertex()) std::cout << "PASS\n";
    else { std::cout << "FAIL\n"; ++failed; }

    std::cout << "Test 4 — Linear chain: idom[i]=i-1 for i>0, source is root: ";
    if (test_linear_chain()) std::cout << "PASS\n";
    else { std::cout << "FAIL\n"; ++failed; }

    std::cout << "Test 5 — Bottleneck vertex: ";
    if (test_bottleneck_vertex()) std::cout << "PASS\n";
    else { std::cout << "FAIL\n"; ++failed; }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
