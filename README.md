# pgRouting Algorithm Prototypes

## Overview

Standalone C++ prototypes for three Boost Graph Library (BGL) algorithms used in pgRouting. These programs demonstrate technical competency for GSoC and map directly to the SQL → C bridge → C++ driver → BGL architecture of pgRouting.

- **Stoer–Wagner min-cut** — minimum cut in undirected weighted graphs (road network resilience).
- **Planar faces** — planarity test and face traversal (Boyer–Myrvold + planar face traversal).
- **Maximum weighted matching** — maximum weight matching (vehicle–pickup assignment).

No dependencies beyond **Boost** and the **C++17** standard library.

---

## Algorithms

### pgr_stoerWagner

Uses `boost::stoer_wagner_min_cut()` on an undirected weighted graph to find a minimum cut and its weight. The prototype builds an 8-vertex road network (intersections 0–7) with given edge capacities and prints the min-cut weight, the two partitions, and the edges that form the cut.

**Minimum Boost version:** 1.38

### pgr_planarFaces

Uses `boost::boyer_myrvold_planarity_test()` and `boost::planar_face_traversal()` to test planarity and enumerate faces. The prototype runs on a 3×3 street grid and on K5; it prints whether the graph is planar, the number of faces, each face’s vertex sequence, the outer face, and Euler’s formula (V − E + F = 2). Non-planar graphs (e.g. K5) are handled without crashing.

**Minimum Boost version:** 1.40

### pgr_maxWeightedMatch

Uses `boost::maximum_weighted_matching()` to compute a maximum-weight matching in an undirected edge-weighted graph. The prototype models 4 vehicles (0–3) and 4 pickup requests (4–7) with compatibility scores on edges and prints matched pairs, edge weights, total weight, and unmatched vertices.

**Minimum Boost version:** 1.76

---

## Requirements

- **C++17** compiler (e.g. GCC 8+, Clang 6+)
- **CMake** ≥ 3.15
- **Boost** ≥ 1.76 (required by `max_weighted_match`; stoer_wagner and planar_faces need 1.38+ and 1.40+ respectively)

**Install Boost:**

- **Ubuntu:** `sudo apt install libboost-all-dev`
- **macOS:** `brew install boost`
- **Windows:** `choco install boost-msvc-14.3`

---

## Build Instructions

From the project root (`pgRouting-prototypes/`):

```bash
mkdir build && cd build
cmake ..
make
make test
```

Optional: if Boost is not in a standard location, set `BOOST_ROOT`:

```bash
cmake .. -DBOOST_ROOT=/path/to/boost
make
make test
```

To build a single target (e.g. Stoer–Wagner prototype):

```bash
make stoer_wagner_prototype
```

Standalone compile (without CMake) for one file, from the algorithm directory:

```bash
g++ -std=c++17 -Wall -Wextra stoer_wagner_prototype.cpp -o stoer_wagner_prototype -lboost_graph
```

---

## Running Tests

Run all test executables and get PASS/FAIL per test:

```bash
cd build
make test
```

Or run each test binary manually:

```bash
./test_stoer_wagner
./test_planar_faces
./test_max_weighted_match
```

---

## Sample Output

### stoer_wagner_prototype

```
=== Graph edges and weights (road network) ===
  0-1: 4
  0-4: 3
  1-2: 3
  1-4: 2
  1-5: 2
  2-3: 1
  2-6: 2
  3-7: 2
  4-5: 3
  5-6: 2
  6-7: 3

Minimum cut weight: 3
Partition A vertices: 3
Partition B vertices: 0, 1, 2, 4, 5, 6, 7
Edges in the cut (crossing the partition): (2-3, weight=1) (3-7, weight=2)
Removing these 2 road(s) would disconnect the network.
Interpretation: min-cut weight = 3.
```

### planar_faces_prototype

```
=== 3x3 street grid ===
Vertices V = 9, Edges E = 12
Planar: yes
Total number of faces F = 5 (Euler: F = E - V + 2 = 5)
Face 1 vertex sequence: 0, 1, 4, 3 (size 4)
Face 2 vertex sequence: 1, 2, 5, 4 (size 4)
Face 3 vertex sequence: 3, 4, 7, 6 (size 4)
Face 4 vertex sequence: 4, 5, 8, 7 (size 4)
Face 5 vertex sequence: 0, 3, 6, 7, 8, 5, 2, 1 (size 8)
Outer (unbounded) face: Face 5 (has most vertices: 8)
Euler formula: V - E + F = 9 - 12 + 5 = 2 (expected 2)

=== K5 (complete graph on 5 vertices) ===
Vertices V = 5, Edges E = 10
Planar: no
Graph is NOT planar.
Non-planar graph: skipping face traversal (no embedding).
```

### max_weighted_match_prototype

```
=== Full graph (edge weights = compatibility scores) ===
  0-4: 9
  0-5: 3
  1-4: 2
  1-6: 8
  2-5: 7
  2-7: 4
  3-6: 3
  3-7: 6

=== Matched pairs (vehicle → pickup) ===
  vertex 0 -> vertex 4 (weight: 9)
  vertex 1 -> vertex 6 (weight: 8)
  vertex 2 -> vertex 5 (weight: 7)
  vertex 3 -> vertex 7 (weight: 6)
Total matching weight: 30
Unmatched vertices: (none)
Verified: no vertex appears in more than one pair
```

---

## pgRouting Integration Notes

Each algorithm follows the same three-layer pattern:

| Layer | Role | Example |
|-------|------|--------|
| **SQL wrapper** | User calls e.g. `pgr_stoerWagner(edges_sql)`; the SQL function is declared in the extension and invokes the C bridge. | `pgr_stoerWagner`, `pgr_planarFaces`, `pgr_maxWeightedMatch` |
| **C bridge** | A C function (e.g. in `stoerWagner.c`) receives the query text and options, calls the C++ driver, and returns results into PostgreSQL `SetOfRecord` / `Tuplestore`, with error and notice handling. | Converts options and edges to driver input; writes result rows. |
| **C++ driver** | Builds a pgRouting graph from the edges, calls the BGL algorithm, and converts the result into the C structs expected by the bridge. | `Pgr_stoerWagner`, planar faces driver, max weighted match driver. |
| **BGL call** | The actual algorithm used in the prototype. | `boost::stoer_wagner_min_cut`, `boost::boyer_myrvold_planarity_test` + `boost::planar_face_traversal`, `boost::maximum_weighted_matching` |

The prototypes use the same BGL types and algorithms so that the logic can be carried over into the pgRouting codebase with minimal change, aside from graph construction from SQL and result formatting for PostgreSQL.
