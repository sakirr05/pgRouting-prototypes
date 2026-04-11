/*
 * kcore_prototype.cpp
 *
 * GSoC competency prototype for pgr_coreNumbers.
 * Demonstrates boost::core_numbers (Batagelj-Zaversnik degree-peeling).
 *
 * The test graph models a small urban street network with:
 *   - Dead-end streets (degree 1)  => expected core number 1
 *   - A winding corridor (degree 2) => expected core number 2
 *   - A dense downtown cluster       => expected core number 3
 *
 * Build:
 *   g++ -std=c++17 -Wall -Wextra kcore_prototype.cpp -o kcore_prototype
 *
 * Or via CMake from the project root (see README.md).
 */

#include <iostream>
#include <vector>
#include <algorithm>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/core_numbers.hpp>
#include <boost/property_map/property_map.hpp>

// Undirected, unweighted graph. vecS storage gives us implicit vertex indices.
using Graph  = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>;
using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

// ---------------------------------------------------------------------------
// build_urban_network
//
// 12-vertex graph designed to exercise all three pruning layers:
//
//   Vertex 0  : dead-end leaf off vertex 1        => core 1
//   Vertex 11 : dead-end leaf off vertex 4        => core 1
//   Vertices 1,2,3,4 : corridor ring (1-2-3-4-1)
//       plus bridge edge 4-5 into the cluster     => core 2
//   Vertices 5,6,7,8,9,10 : dense cluster
//       each vertex has >= 3 neighbors in cluster  => core 3
//
// After Batagelj-Zaversnik peeling:
//   k=1 peels: 0, 11   (degree-1 leaves)
//   k=2 peels: 1,2,3,4 (corridor: each has exactly 2 corridor neighbors)
//   k=3 stays: 5,6,7,8,9,10 (dense cluster survives)
// ---------------------------------------------------------------------------
static Graph build_urban_network() {
    Graph g(12);

    // Dead-end streets (degree 1 leaves)
    add_edge(0,  1, g);    // vertex 0 hangs off vertex 1
    add_edge(11, 4, g);    // vertex 11 hangs off vertex 4

    // Corridor ring: 1 - 2 - 3 - 4 - 1
    // Each corridor vertex has degree 2 within the ring, so they form
    // a 2-core that survives the k=1 round but collapses at k=2.
    add_edge(1, 2, g);
    add_edge(2, 3, g);
    add_edge(3, 4, g);
    add_edge(4, 1, g);

    // Bridge: corridor connects to the dense cluster through vertex 5
    add_edge(4, 5, g);

    // Dense downtown cluster on {5, 6, 7, 8, 9, 10}
    // Wired so every vertex has at least 3 neighbors within the cluster.
    add_edge(5,  6, g);
    add_edge(5,  7, g);
    add_edge(5,  8, g);
    add_edge(6,  7, g);
    add_edge(6,  8, g);
    add_edge(6,  9, g);
    add_edge(7,  9, g);
    add_edge(7, 10, g);
    add_edge(8,  9, g);
    add_edge(8, 10, g);
    add_edge(9, 10, g);

    return g;
}

int main() {
    Graph g = build_urban_network();

    const auto V = num_vertices(g);
    const auto E = num_edges(g);

    std::cout << "=== K-Core Decomposition (Batagelj-Zaversnik) ===" << "\n";
    std::cout << "Vertices: " << V << "   Edges: " << E << "\n\n";

    // Allocate storage for the core number of each vertex.
    std::vector<int> core(V, 0);

    // Build a property map that maps vertex descriptors to entries in `core`.
    // boost::core_numbers needs a ReadWritePropertyMap keyed by vertex.
    auto core_map = boost::make_iterator_property_map(
        core.begin(),
        get(boost::vertex_index, g));

    // Run the Batagelj-Zaversnik algorithm.
    // This fills core_map with the core number of each vertex.
    boost::core_numbers(g, core_map);

    // Full report: print each vertex and its core number.
    std::cout << "Vertex  | Core Number\n";
    std::cout << "--------+------------\n";
    for (size_t v = 0; v < V; ++v) {
        std::cout << "   " << v;
        if (v < 10) std::cout << " ";    // alignment for single-digit IDs
        std::cout << "   |     " << core[v] << "\n";
    }
    std::cout << "\n";

    // Summarize by core level to prove the pruning logic.
    int max_core = *std::max_element(core.begin(), core.end());

    for (int k = 1; k <= max_core; ++k) {
        std::cout << "Core " << k << " vertices: ";
        bool first = true;
        for (size_t v = 0; v < V; ++v) {
            if (core[v] == k) {
                if (!first) std::cout << ", ";
                std::cout << v;
                first = false;
            }
        }
        std::cout << "\n";
    }

    std::cout << "\n";

    // Specifically call out the dead-end pruning (core 1) to prove the
    // Batagelj-Zaversnik peeling order is correct.
    std::cout << "Dead-end verification (core 1 = degree-1 leaves): ";
    bool found_any = false;
    for (size_t v = 0; v < V; ++v) {
        if (core[v] == 1) {
            if (found_any) std::cout << ", ";
            std::cout << "vertex " << v << " (degree "
                      << boost::degree(static_cast<Vertex>(v), g) << ")";
            found_any = true;
        }
    }
    if (!found_any) std::cout << "(none)";
    std::cout << "\n";

    return 0;
}
