// Compile: g++ -std=c++17 -Wall -Wextra test_planar_faces.cpp -o test_planar_faces -lboost_graph
//
// Tests for planar faces prototype. Each test prints name then PASS or FAIL.

#include <iostream>
#include <vector>
#include <set>
#include <cstdlib>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/planar_face_traversal.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>

using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    boost::property<boost::vertex_index_t, int>,
    boost::property<boost::edge_index_t, int>>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using Edge = boost::graph_traits<Graph>::edge_descriptor;

struct CountFacesVisitor : public boost::planar_face_traversal_visitor {
    int face_count = 0;
    std::vector<std::vector<int>> faces;
    std::vector<int> cur;

    void begin_face() { cur.clear(); }
    template <typename V> void next_vertex(V v) { cur.push_back(static_cast<int>(v)); }
    template <typename E> void next_edge(E) {}
    void end_face() { face_count++; faces.push_back(cur); }
};

static void set_edge_indices(Graph& g) {
    auto eidx = get(boost::edge_index, g);
    size_t i = 0;
    for (auto [ei, ei_end] = edges(g); ei != ei_end; ++ei)
        put(eidx, *ei, static_cast<int>(i++));
}

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

// Test 1: Euler formula V - E + F = 2 for 3x3 grid
static bool test_euler_formula() {
    Graph g = build_3x3_grid();
    set_edge_indices(g);
    const auto V = num_vertices(g);
    const auto E = num_edges(g);
    std::vector<std::vector<Edge>> embedding(V);
    if (!boost::boyer_myrvold_planarity_test(
            boost::boyer_myrvold_params::graph = g,
            boost::boyer_myrvold_params::embedding = &embedding[0]))
        return false;
    CountFacesVisitor vis;
    boost::planar_face_traversal(g, &embedding[0], vis);
    int F = vis.face_count;
    int euler = static_cast<int>(V) - static_cast<int>(E) + F;
    std::cout << "  V = " << V << ", E = " << E << ", F = " << F << " => V - E + F = " << euler << "\n";
    return euler == 2;
}

// Test 2: Face count equals E - V + 2
static bool test_face_count() {
    Graph g = build_3x3_grid();
    set_edge_indices(g);
    const auto V = num_vertices(g);
    const auto E = num_edges(g);
    std::vector<std::vector<Edge>> embedding(V);
    if (!boost::boyer_myrvold_planarity_test(
            boost::boyer_myrvold_params::graph = g,
            boost::boyer_myrvold_params::embedding = &embedding[0]))
        return false;
    CountFacesVisitor vis;
    boost::planar_face_traversal(g, &embedding[0], vis);
    int expected_faces = static_cast<int>(E) - static_cast<int>(V) + 2;
    std::cout << "  Expected faces E - V + 2 = " << expected_faces << ", actual = " << vis.face_count << "\n";
    return vis.face_count == expected_faces;
}

// Test 3: K5 is non-planar
static bool test_K5_non_planar() {
    Graph g(5);
    for (int i = 0; i < 5; ++i)
        for (int j = i + 1; j < 5; ++j)
            add_edge(static_cast<Vertex>(i), static_cast<Vertex>(j), g);
    set_edge_indices(g);
    bool planar = boost::boyer_myrvold_planarity_test(g);
    std::cout << "  K5 planarity test returned: " << (planar ? "true" : "false") << " (expected false)\n";
    return !planar;
}

// Test 4: Triangle has exactly 2 faces (1 inner + 1 outer)
static bool test_single_face_graph() {
    Graph g(3);
    add_edge(0, 1, g);
    add_edge(1, 2, g);
    add_edge(2, 0, g);
    set_edge_indices(g);
    std::vector<std::vector<Edge>> embedding(num_vertices(g));
    if (!boost::boyer_myrvold_planarity_test(
            boost::boyer_myrvold_params::graph = g,
            boost::boyer_myrvold_params::embedding = &embedding[0]))
        return false;
    CountFacesVisitor vis;
    boost::planar_face_traversal(g, &embedding[0], vis);
    std::cout << "  Triangle: expected 2 faces, actual " << vis.face_count << "\n";
    return vis.face_count == 2;
}

// Test 5: Every vertex appears in at least one face
static bool test_visitor_completeness() {
    Graph g = build_3x3_grid();
    set_edge_indices(g);
    std::vector<std::vector<Edge>> embedding(num_vertices(g));
    if (!boost::boyer_myrvold_planarity_test(
            boost::boyer_myrvold_params::graph = g,
            boost::boyer_myrvold_params::embedding = &embedding[0]))
        return false;
    CountFacesVisitor vis;
    boost::planar_face_traversal(g, &embedding[0], vis);
    std::set<int> in_face;
    for (const auto& face : vis.faces)
        for (int v : face) in_face.insert(v);
    bool all = (in_face.size() == static_cast<size_t>(num_vertices(g)));
    std::cout << "  Vertices in at least one face: " << in_face.size() << " / " << num_vertices(g) << "\n";
    return all;
}

int main() {
    int failed = 0;

    std::cout << "Test 1 — Euler formula: ";
    if (test_euler_formula()) std::cout << "PASS\n";
    else { std::cout << "FAIL (V - E + F != 2)\n"; ++failed; }

    std::cout << "Test 2 — Face count: ";
    if (test_face_count()) std::cout << "PASS\n";
    else { std::cout << "FAIL (face count != E - V + 2)\n"; ++failed; }

    std::cout << "Test 3 — K5 non-planar: ";
    if (test_K5_non_planar()) std::cout << "PASS\n";
    else { std::cout << "FAIL (K5 should be non-planar)\n"; ++failed; }

    std::cout << "Test 4 — Single face graph (triangle): ";
    if (test_single_face_graph()) std::cout << "PASS\n";
    else { std::cout << "FAIL (triangle should have 2 faces)\n"; ++failed; }

    std::cout << "Test 5 — Visitor completeness: ";
    if (test_visitor_completeness()) std::cout << "PASS\n";
    else { std::cout << "FAIL (every vertex should appear in some face)\n"; ++failed; }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
