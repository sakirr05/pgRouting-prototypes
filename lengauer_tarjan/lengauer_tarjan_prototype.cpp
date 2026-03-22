// Compile: g++ -std=c++17 -Wall -Wextra lengauer_tarjan_prototype.cpp -o lengauer_tarjan_prototype
//
// Standalone prototype for pgr_lengauerTarjanDominatorTree: dominator tree via Boost's
// Lengauer-Tarjan algorithm. Uses a hardcoded 7-vertex directed road network; prints the
// dominator tree edges, immediate dominator for each vertex, and a practical interpretation.

#include <iostream>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/dominator_tree.hpp>
#include <boost/property_map/property_map.hpp>

// Note: prototype uses 0-based BGL vertex indices.
// pgRouting integration maps arbitrary SQL vertex IDs (1-based by convention)
// to contiguous BGL indices via the vertex ID remapping layer — see proposal Section 14.7.

// BGL adjacency_list: vecS vertex/edge containers, bidirectionalS (directed graph with in_edges).
// Dominator tree algorithms require a BidirectionalGraph (in_edges support).
using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::bidirectionalS>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using GraphTraits = boost::graph_traits<Graph>;

// Road network: 7 nodes modeling a city layout.
//   0 = Hospital      (source / emergency dispatch)
//   1 = North Gate
//   2 = South Gate
//   3 = Bridge        (single crossing — converges both routes)
//   4 = Downtown Hub  (bottleneck: all routes to airport/station pass here)
//   5 = Airport
//   6 = Train Station
// Edges represent one-way streets / directed road segments.
static Graph build_road_network() {
    // BGL adjacency_list constructor: create directed graph with 7 vertices.
    Graph g(7);
    // BGL add_edge: add directed edge (u -> v).
    add_edge(0, 1, g);  // Hospital   -> North Gate
    add_edge(0, 2, g);  // Hospital   -> South Gate
    add_edge(1, 3, g);  // North Gate -> Bridge
    add_edge(2, 3, g);  // South Gate -> Bridge
    add_edge(3, 4, g);  // Bridge     -> Downtown Hub
    add_edge(4, 5, g);  // Downtown   -> Airport
    add_edge(4, 6, g);  // Downtown   -> Train Station
    return g;
}

int main() {
    Graph g = build_road_network();
    // BGL num_vertices(g): returns the number of vertices in the graph.
    const auto n = num_vertices(g);

    // Source vertex: Hospital (vertex 0).
    const Vertex source = 0;

    // Allocate dominator map: domTree[v] = immediate dominator of v.
    // Initialise to null_vertex(); unreachable vertices keep this sentinel.
    std::vector<Vertex> domTree(n, GraphTraits::null_vertex());
    // BGL make_iterator_property_map: wraps a random-access iterator as a property map
    // indexed by the vertex_index property (0..n-1 for vecS containers).
    auto domMap = boost::make_iterator_property_map(
        domTree.begin(), get(boost::vertex_index, g));

    // BGL lengauer_tarjan_dominator_tree: computes dominator tree for directed graph g
    // rooted at source; writes the immediate dominator of each reachable vertex into domMap.
    // Boost convention: idom[source] = null_vertex() (source is root, has no parent).
    // Unreachable vertices also retain the null_vertex() sentinel.
    // Minimum Boost version: 1.38.
    boost::lengauer_tarjan_dominator_tree(g, source, domMap);

    // ---- Print dominator tree edges ----
    std::cout << "=== Dominator tree edges (immediate dominator -> vertex) ===\n";
    // BGL vertices(g): iterator pair over all vertices.
    for (auto [vi, vi_end] = vertices(g); vi != vi_end; ++vi) {
        Vertex v = *vi;
        // BGL get(domMap, v): read the immediate dominator of v from the property map.
        Vertex dom = get(domMap, v);
        if (v == source) {
            // Source is the dominator tree root; Boost leaves its idom as null_vertex().
            std::cout << "  vertex " << v << ": source (dominator tree root)\n";
        } else if (dom == GraphTraits::null_vertex()) {
            std::cout << "  vertex " << v << ": no dominator (unreachable from source)\n";
        } else {
            std::cout << "  " << dom << " -> " << v << "\n";
        }
    }

    // ---- Print immediate dominator per vertex ----
    const char* labels[] = {
        "Hospital", "North Gate", "South Gate", "Bridge",
        "Downtown Hub", "Airport", "Train Station"
    };
    std::cout << "\n=== Immediate dominator per vertex ===\n";
    for (auto [vi, vi_end] = vertices(g); vi != vi_end; ++vi) {
        Vertex v = *vi;
        Vertex dom = get(domMap, v);
        if (v == source) {
            // Boost leaves idom[source] = null_vertex(); source is the tree root.
            std::cout << "  vertex " << v << " (" << labels[v]
                      << "): idom = null (source / tree root)\n";
        } else if (dom == GraphTraits::null_vertex()) {
            std::cout << "  vertex " << v << " (" << labels[v]
                      << "): idom = null (unreachable)\n";
        } else {
            std::cout << "  vertex " << v << " (" << labels[v]
                      << "): idom = " << dom << " (" << labels[dom] << ")\n";
        }
    }

    // ---- Practical interpretation ----
    std::cout << "\n=== Practical interpretation (unavoidable intersections) ===\n";
    std::cout << "  From source (vertex 0 = Hospital):\n";
    for (auto [vi, vi_end] = vertices(g); vi != vi_end; ++vi) {
        Vertex v = *vi;
        if (v == source) continue;
        Vertex dom = get(domMap, v);
        if (dom == GraphTraits::null_vertex()) continue;
        if (dom != source) {
            // dom is an intermediate bottleneck: every path from source to v goes through dom.
            std::cout << "  vertex " << dom << " (" << labels[dom]
                      << ") is unavoidable on all paths from source to vertex "
                      << v << " (" << labels[v] << ")\n";
        }
    }

    std::cout << "\n  Note: BGL uses 0-based vertex indices.\n"
              << "  pgRouting remaps arbitrary SQL vertex IDs (1-based by convention)\n"
              << "  to contiguous BGL indices via the vertex ID remapping layer.\n";

    return 0;
}
