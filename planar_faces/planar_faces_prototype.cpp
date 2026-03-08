// Compile: g++ -std=c++17 -Wall -Wextra planar_faces_prototype.cpp -o planar_faces_prototype -lboost_graph
//
// Standalone prototype for pgr_planarFaces: planarity test and face traversal via BGL.
// Uses 3x3 grid and K5; prints faces, outer face, and Euler formula.

#include <iostream>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/planar_face_traversal.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/property_map/property_map.hpp>

using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    boost::property<boost::vertex_index_t, int>,
    boost::property<boost::edge_index_t, int>>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using Edge = boost::graph_traits<Graph>::edge_descriptor;

// Custom visitor: records each face's vertex and edge sequences; counts faces.
struct FaceRecorderVisitor : public boost::planar_face_traversal_visitor {
    const Graph* g_ptr = nullptr;
    int face_count = 0;
    std::vector<int> current_face_vertices;
    std::vector<std::pair<int, int>> current_face_edges;
    std::vector<std::vector<int>> all_faces;

    // planar_face_traversal_visitor: begin_face() — called when starting a new face.
    void begin_face() {
        face_count++;
        current_face_vertices.clear();
        current_face_edges.clear();
    }

    // planar_face_traversal_visitor: next_vertex(Vertex v) — called for each vertex on the face boundary.
    template <typename V>
    void next_vertex(V v) {
        current_face_vertices.push_back(static_cast<int>(v));
    }

    // planar_face_traversal_visitor: next_edge(Edge e) — called for each edge on the face boundary.
    template <typename E>
    void next_edge(E e) {
        if (g_ptr) {
            int u = static_cast<int>(source(e, *g_ptr));
            int v = static_cast<int>(target(e, *g_ptr));
            current_face_edges.push_back({u, v});
        }
    }

    // planar_face_traversal_visitor: end_face() — called when finishing the current face.
    void end_face() {
        all_faces.push_back(current_face_vertices);
        (void)current_face_edges;  // recorded for completeness; we print vertex sequence
    }
};

// Build 3x3 street grid: 0-1-2 / 3-4-5 / 6-7-8 with horizontal and vertical edges.
static Graph build_3x3_grid() {
    Graph g(9);
    add_edge(0, 1, g);
    add_edge(1, 2, g);
    add_edge(3, 4, g);
    add_edge(4, 5, g);
    add_edge(6, 7, g);
    add_edge(7, 8, g);
    add_edge(0, 3, g);
    add_edge(3, 6, g);
    add_edge(1, 4, g);
    add_edge(4, 7, g);
    add_edge(2, 5, g);
    add_edge(5, 8, g);
    return g;
}

// Assign edge indices (required for planar_face_traversal / embedding).
static void set_edge_indices(Graph& g) {
    auto eidx = get(boost::edge_index, g);
    boost::graph_traits<Graph>::edges_size_type i = 0;
    for (auto [ei, ei_end] = edges(g); ei != ei_end; ++ei)
        put(eidx, *ei, static_cast<int>(i++));
}

static void run_planar_demo(Graph& g, const char* name) {
    const auto V = num_vertices(g);
    const auto E = num_edges(g);

    std::cout << "=== " << name << " ===\n";
    std::cout << "Vertices V = " << V << ", Edges E = " << E << "\n";

    // BGL planar embedding: for each vertex, clockwise order of incident edges (used by face traversal).
    using Embedding = std::vector<std::vector<Edge>>;
    Embedding embedding(V);

    // BGL boyer_myrvold_planarity_test: tests planarity; if planar, fills embedding for face traversal.
    bool planar = boost::boyer_myrvold_planarity_test(
        boost::boyer_myrvold_params::graph = g,
        boost::boyer_myrvold_params::embedding = &embedding[0]);

    std::cout << "Planar: " << (planar ? "yes" : "no") << "\n";

    if (!planar) {
        std::cout << "Graph is NOT planar.\n";
        std::cout << "Non-planar graph: skipping face traversal (no embedding).\n\n";
        return;
    }

    // BGL planar_face_traversal: walks each face using the embedding; visitor records vertices/edges per face.
    FaceRecorderVisitor vis;
    vis.g_ptr = &g;
    boost::planar_face_traversal(g, &embedding[0], vis);

    const int F = vis.face_count;
    std::cout << "Total number of faces F = " << F << " (Euler: F = E - V + 2 = " << (E - V + 2) << ")\n";

    for (size_t i = 0; i < vis.all_faces.size(); ++i) {
        std::cout << "Face " << (i + 1) << " vertex sequence: ";
        for (size_t j = 0; j < vis.all_faces[i].size(); ++j)
            std::cout << (j ? ", " : "") << vis.all_faces[i][j];
        std::cout << " (size " << vis.all_faces[i].size() << ")\n";
    }

    // Outer (unbounded) face: the one with the most vertices.
    size_t outer_idx = 0;
    for (size_t i = 1; i < vis.all_faces.size(); ++i)
        if (vis.all_faces[i].size() > vis.all_faces[outer_idx].size())
            outer_idx = i;
    std::cout << "Outer (unbounded) face: Face " << (outer_idx + 1) << " (has most vertices: "
              << vis.all_faces[outer_idx].size() << ")\n";

    std::cout << "Euler formula: V - E + F = " << V << " - " << E << " + " << F
              << " = " << (V - E + F) << " (expected 2)\n\n";
}

// K5: complete graph on 5 vertices (non-planar).
static Graph build_K5() {
    Graph g(5);
    for (int i = 0; i < 5; ++i)
        for (int j = i + 1; j < 5; ++j)
            add_edge(static_cast<Vertex>(i), static_cast<Vertex>(j), g);
    return g;
}

int main() {
    try {
        Graph grid = build_3x3_grid();
        set_edge_indices(grid);
        run_planar_demo(grid, "3x3 street grid");

        Graph k5 = build_K5();
        set_edge_indices(k5);
        run_planar_demo(k5, "K5 (complete graph on 5 vertices)");
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
