// Compile: g++ -std=c++17 -Wall -Wextra articulation_points_prototype.cpp -o articulation_points_prototype -lboost_graph
//
// Standalone prototype for pgr_articulationPoints: articulation points (cut vertices) via BGL.
//
// Documentation:
// ------------
// Articulation points (cut vertices) are vertices whose removal increases the number of
// connected components in an undirected graph. They represent single points of failure:
// if the vertex is removed, the graph becomes disconnected. This prototype uses
// boost::articulation_points(), which implements Tarjan's DFS-based algorithm with
// time complexity O(V + E).
//
// Use cases in spatial networks:
// - Critical intersections in road networks: articulation points are junctions whose
//   removal would disconnect parts of the network (e.g. a bridge endpoint, or a single
//   road linking two regions). Identifying them helps prioritize resilience (e.g. bypass
//   roads) and vulnerability analysis.
// - Infrastructure and connectivity planning: finding cut vertices in utility or
//   transport graphs supports redundancy and contingency planning.

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/graph_traits.hpp>

// ---- Input edge type (pgRouting-style) ----
struct Edge {
    int64_t id;
    int64_t source;
    int64_t target;
    double cost;
    double reverse_cost;
};

// ---- BGL graph type: vecS, vecS, undirectedS (no edge weights needed for articulation points) ----
using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

// ---- Step 1: Parse edge list -> collect unique node IDs and build index map ----
// Returns sorted unique node IDs; fills node_id_to_index such that node_id_to_index[id] = vertex index.
static std::vector<int64_t> collect_node_ids_and_index(
    const std::vector<Edge>& edges,
    std::unordered_map<int64_t, std::size_t>& node_id_to_index)
{
    std::vector<int64_t> node_ids;
    node_ids.reserve(edges.size() * 2);
    for (const Edge& e : edges) {
        node_ids.push_back(e.source);
        node_ids.push_back(e.target);
    }
    std::sort(node_ids.begin(), node_ids.end());
    node_ids.erase(std::unique(node_ids.begin(), node_ids.end()), node_ids.end());
    node_id_to_index.clear();
    node_id_to_index.reserve(node_ids.size());
    for (std::size_t i = 0; i < node_ids.size(); ++i)
        node_id_to_index[node_ids[i]] = i;
    return node_ids;
}

// ---- Step 2 & 3: Build Boost adjacency_list from edges ----
static Graph build_graph(
    const std::vector<Edge>& edges,
    const std::unordered_map<int64_t, std::size_t>& node_id_to_index)
{
    const std::size_t n = node_id_to_index.size();
    Graph g(static_cast<boost::graph_traits<Graph>::vertices_size_type>(n));
    for (const Edge& e : edges) {
        auto it_s = node_id_to_index.find(e.source);
        auto it_t = node_id_to_index.find(e.target);
        if (it_s != node_id_to_index.end() && it_t != node_id_to_index.end()) {
            Vertex u = static_cast<Vertex>(it_s->second);
            Vertex v = static_cast<Vertex>(it_t->second);
            if (u != v)
                add_edge(u, v, g);
        }
    }
    return g;
}

// ---- Run BGL articulation_points: graph building is separate from algorithm execution ----
// Steps: 1) Collect unique node IDs  2) Create nodeID→index mapping  3) Build Boost graph
//        4) Insert edges  5) Run articulation_points()  6) Map vertex indices back to node IDs
//        7) Return articulation nodes sorted ascending.  Time complexity: O(V + E).
std::vector<int64_t> pgr_articulationPoints(const std::vector<Edge>& edges)
{
    if (edges.empty())
        return {};

    std::unordered_map<int64_t, std::size_t> node_id_to_index;
    std::vector<int64_t> node_ids = collect_node_ids_and_index(edges, node_id_to_index);
    if (node_ids.size() < 2)
        return {};

    Graph g = build_graph(edges, node_id_to_index);
    std::vector<Vertex> ap_vertices;
    ap_vertices.reserve(node_ids.size());
    boost::articulation_points(g, std::back_inserter(ap_vertices));

    std::vector<int64_t> result;
    result.reserve(ap_vertices.size());
    for (Vertex v : ap_vertices)
        result.push_back(node_ids[static_cast<std::size_t>(v)]);
    std::sort(result.begin(), result.end());
    return result;
}

// ---- Demo: graph where 3, 6, 7, 8 are the only articulation points ----
// Triangle 1-2-3 (no AP); 3 connects to 4 and to chain 6-7-8-9 (APs: 3, 6, 7, 8).
static std::vector<Edge> make_demo_edges()
{
    return {
        {1, 1, 2, 1.0, 1.0},
        {2, 2, 3, 1.0, 1.0},
        {3, 3, 1, 1.0, 1.0},
        {4, 3, 4, 1.0, 1.0},
        {5, 3, 6, 1.0, 1.0},
        {6, 6, 7, 1.0, 1.0},
        {7, 7, 8, 1.0, 1.0},
        {8, 8, 9, 1.0, 1.0},
    };
}

int main()
{
    std::vector<Edge> edges = make_demo_edges();
    std::vector<int64_t> nodes = pgr_articulationPoints(edges);
    std::cout << "node\n";
    for (int64_t id : nodes)
        std::cout << id << "\n";
    return 0;
}
