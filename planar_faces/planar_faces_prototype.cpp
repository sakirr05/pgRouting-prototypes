/*
 * planar_faces_prototype.cpp
 *
 * GSoC competency prototype for pgr_planarFaces.
 * Demonstrates:
 *   1. boost::boyer_myrvold_planarity_test  (planarity check + embedding)
 *   2. boost::planar_face_traversal         (face extraction via visitor)
 *
 * Test Case A: 3x3 planar street grid  (9 vertices, 12 edges, 5 faces)
 * Test Case B: K5 complete graph        (non-planar, graceful exit)
 *
 * Build:
 *   g++ -std=c++17 -Wall -Wextra planar_faces_prototype.cpp -o planar_faces_prototype
 *
 * Or via CMake from the project root (see README.md).
 */

#include <iostream>
#include <vector>
#include <algorithm>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/planar_face_traversal.hpp>
#include <boost/property_map/property_map.hpp>

// Graph type: undirected, vecS storage, with an edge_index property
// (edge_index is required by the BGL planar embedding machinery).
using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    boost::property<boost::vertex_index_t, int>,
    boost::property<boost::edge_index_t, int>>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using Edge   = boost::graph_traits<Graph>::edge_descriptor;

// ---------------------------------------------------------------------------
// FaceRecorderVisitor
//
// Conforms to the BGL PlanarFaceTraversalVisitor concept.
// On each face boundary the BGL calls: begin_face(), then a sequence of
// next_vertex(v) / next_edge(e) pairs, then end_face().
// We record the vertex sequence of every face for later printing.
// ---------------------------------------------------------------------------
struct FaceRecorderVisitor : public boost::planar_face_traversal_visitor {

    // Storage: one vector<int> per face, holding the vertex indices in order.
    std::vector<std::vector<int>> faces;

    // Scratch space for the face currently being walked.
    std::vector<int> current;

    // Called when the traversal enters a new face.
    void begin_face() {
        current.clear();
    }

    // Called for each vertex along the face boundary.
    template <typename V>
    void next_vertex(V v) {
        current.push_back(static_cast<int>(v));
    }

    // Called for each edge along the face boundary (unused here, but required).
    template <typename E>
    void next_edge(E) {}

    // Called when the face boundary is complete.
    void end_face() {
        faces.push_back(current);
    }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Assign sequential edge indices starting from 0.
// The BGL planar embedding algorithms require edge_index to be populated.
static void assign_edge_indices(Graph& g) {
    auto idx_map = get(boost::edge_index, g);
    int i = 0;
    for (auto [it, end] = edges(g); it != end; ++it) {
        put(idx_map, *it, i++);
    }
}

// Build a 3x3 street grid:
//
//   0 - 1 - 2
//   |   |   |
//   3 - 4 - 5
//   |   |   |
//   6 - 7 - 8
//
// 9 vertices, 12 edges. Planar with 5 faces (4 bounded + 1 outer).
static Graph build_3x3_grid() {
    Graph g(9);

    // horizontal rows
    add_edge(0, 1, g); add_edge(1, 2, g);
    add_edge(3, 4, g); add_edge(4, 5, g);
    add_edge(6, 7, g); add_edge(7, 8, g);

    // vertical columns
    add_edge(0, 3, g); add_edge(3, 6, g);
    add_edge(1, 4, g); add_edge(4, 7, g);
    add_edge(2, 5, g); add_edge(5, 8, g);

    assign_edge_indices(g);
    return g;
}

// Build K5 (complete graph on 5 vertices).
// K5 is the smallest non-planar complete graph (Kuratowski's theorem).
static Graph build_K5() {
    Graph g(5);
    for (int i = 0; i < 5; ++i)
        for (int j = i + 1; j < 5; ++j)
            add_edge(static_cast<Vertex>(i), static_cast<Vertex>(j), g);

    assign_edge_indices(g);
    return g;
}

// ---------------------------------------------------------------------------
// run_planar_pipeline
//
// Two-step pipeline matching the pgr_planarFaces design:
//   Step 1: boyer_myrvold_planarity_test  (planarity + embedding)
//   Step 2: planar_face_traversal         (face enumeration via visitor)
//
// If the graph is non-planar, prints a warning and returns gracefully.
// ---------------------------------------------------------------------------
static void run_planar_pipeline(Graph& g, const char* label) {
    const auto V = num_vertices(g);
    const auto E = num_edges(g);

    std::cout << "=== " << label << " ===" << "\n";
    std::cout << "Vertices: " << V << "   Edges: " << E << "\n";

    // Step 1: planarity test.
    // If the graph is planar, boyer_myrvold also computes a combinatorial
    // planar embedding (clockwise edge ordering per vertex) that is needed
    // by the face traversal.
    using EmbeddingEntry = std::vector<Edge>;
    std::vector<EmbeddingEntry> embedding(V);

    bool is_planar = boost::boyer_myrvold_planarity_test(
        boost::boyer_myrvold_params::graph = g,
        boost::boyer_myrvold_params::embedding = &embedding[0]);

    std::cout << "Planar: " << (is_planar ? "yes" : "no") << "\n";

    if (!is_planar) {
        // In the real pgRouting extension this would emit a SQL WARNING
        // and return an empty result set. Here we just print and return.
        std::cout << "Graph is non-planar, skipping traversal." << "\n\n";
        return;
    }

    // Step 2: face traversal using the computed embedding.
    FaceRecorderVisitor visitor;
    boost::planar_face_traversal(g, &embedding[0], visitor);

    const int F = static_cast<int>(visitor.faces.size());

    std::cout << "Faces: " << F
              << "  (Euler predicts E - V + 2 = " << (E - V + 2) << ")\n";

    for (size_t i = 0; i < visitor.faces.size(); ++i) {
        std::cout << "  Face " << (i + 1) << ": ";
        const auto& face = visitor.faces[i];
        for (size_t j = 0; j < face.size(); ++j) {
            if (j) std::cout << ", ";
            std::cout << face[j];
        }
        std::cout << "  (size " << face.size() << ")\n";
    }

    // Identify the outer (unbounded) face as the one with the most vertices.
    size_t outer = 0;
    for (size_t i = 1; i < visitor.faces.size(); ++i) {
        if (visitor.faces[i].size() > visitor.faces[outer].size())
            outer = i;
    }
    std::cout << "Outer face: Face " << (outer + 1)
              << " (" << visitor.faces[outer].size() << " vertices)\n";

    // Verify Euler's formula for connected planar graphs: V - E + F = 2
    int euler = static_cast<int>(V) - static_cast<int>(E) + F;
    std::cout << "Euler check: " << V << " - " << E << " + " << F
              << " = " << euler << "  (expected 2)\n\n";
}

// ---------------------------------------------------------------------------
int main() {
    // Test Case A: planar graph
    Graph grid = build_3x3_grid();
    run_planar_pipeline(grid, "3x3 Street Grid (planar)");

    // Test Case B: non-planar graph (must not crash)
    Graph k5 = build_K5();
    run_planar_pipeline(k5, "K5 Complete Graph (non-planar)");

    return 0;
}
