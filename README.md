# pgRouting Algorithm Prototypes

Standalone C++17 prototypes for two Boost Graph Library algorithms that I am proposing to implement in pgRouting as part of Google Summer of Code.

| Algorithm | BGL Function | Prototype File |
|-----------|-------------|----------------|
| Planar Face Extraction (`pgr_planarFaces`) | `boost::boyer_myrvold_planarity_test` + `boost::planar_face_traversal` | `planar_faces/planar_faces_prototype.cpp` |
| K-Core Decomposition (`pgr_coreNumbers`) | `boost::core_numbers` (Batagelj-Zaversnik) | `kcore/kcore_prototype.cpp` |

Both programs are self-contained: they hardcode their test graphs, run the BGL algorithms, and print the results. No PostgreSQL, no external data files, no runtime dependencies beyond Boost headers.

## Requirements

- C++17 compiler (GCC 8+, Clang 6+, or MSVC 19.14+)
- CMake 3.15 or newer
- Boost 1.40 or newer (header-only; no compiled Boost libraries needed)

Install Boost:

```bash
# Ubuntu / Debian
sudo apt install libboost-all-dev

# macOS
brew install boost

# Windows (vcpkg)
vcpkg install boost
```

## Build Instructions

```bash
mkdir build && cd build
cmake ..
make
```

This produces two executables: `planar_faces_prototype` and `kcore_prototype`.

If Boost is installed in a non-standard location:

```bash
cmake .. -DBOOST_ROOT=/path/to/boost
make
```

To build only one target:

```bash
make planar_faces_prototype
make kcore_prototype
```

## Running

From the `build/` directory:

```bash
./planar_faces_prototype
./kcore_prototype
```

## Expected Output

### planar_faces_prototype

```
=== 3x3 Street Grid (planar) ===
Vertices: 9   Edges: 12
Planar: yes
Faces: 5  (Euler predicts E - V + 2 = 5)
  Face 1: 0, 1, 2, 5, 8, 7, 6, 3  (size 8)
  Face 2: 1, 0, 3, 4  (size 4)
  Face 3: 2, 1, 4, 5  (size 4)
  Face 4: 4, 3, 6, 7  (size 4)
  Face 5: 5, 4, 7, 8  (size 4)
Outer face: Face 1 (8 vertices)
Euler check: 9 - 12 + 5 = 2  (expected 2)

=== K5 Complete Graph (non-planar) ===
Vertices: 5   Edges: 10
Planar: no
Graph is non-planar, skipping traversal.
```

The 3x3 grid has 4 bounded rectangular faces plus 1 unbounded outer face, satisfying Euler's formula V - E + F = 2. K5 is correctly detected as non-planar and the program exits cleanly without crashing.

### kcore_prototype

```
=== K-Core Decomposition (Batagelj-Zaversnik) ===
Vertices: 12   Edges: 18

Vertex  | Core Number
--------+------------
    0   |     1
    1   |     2
    2   |     2
    3   |     2
    4   |     2
    5   |     3
    6   |     3
    7   |     3
    8   |     3
    9   |     3
   10   |     3
   11   |     1

Core 1 vertices: 0, 11
Core 2 vertices: 1, 2, 3, 4
Core 3 vertices: 5, 6, 7, 8, 9, 10

Dead-end verification (core 1 = degree-1 leaves): vertex 0 (degree 1), vertex 11 (degree 1)
```

Vertices 0 and 11 are degree-1 leaves and get peeled first (core 1). The corridor ring 1-2-3-4 collapses next (core 2). The dense downtown cluster 5-10 survives to core 3 because every vertex in it has at least 3 neighbors within the cluster.

## Algorithm Notes

**Planar Face Traversal** works in two steps. First, `boyer_myrvold_planarity_test` checks whether the graph can be drawn on a plane without edge crossings. If it can, the test also computes a *planar embedding*, which is a clockwise ordering of edges around each vertex. Second, `planar_face_traversal` walks that embedding and reports the vertex sequence of every face through a visitor callback. In the pgRouting extension, the visitor output maps directly to the SQL result set rows.

**K-Core Decomposition** uses the Batagelj-Zaversnik algorithm. It repeatedly removes vertices whose current degree is below *k*, starting from k=1 and working upward. The core number of a vertex is the highest value of *k* for which that vertex still belongs to the remaining subgraph. This is useful for identifying the structurally important core of a road network versus peripheral dead-end streets.

## Repository Structure

```
pgRouting-prototypes/
  CMakeLists.txt
  README.md
  planar_faces/
    planar_faces_prototype.cpp
  kcore/
    kcore_prototype.cpp
```

## Author

Sakir ([@sakirr05](https://github.com/sakirr05))

GSoC proposal: Planar Face Extraction and K-Core Decomposition for pgRouting.
