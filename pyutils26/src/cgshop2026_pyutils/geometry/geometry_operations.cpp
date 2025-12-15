#include "geometry_operations.h"
#include "cgal_utils.h"
#include "triangulation_validation.h"
#include <CGAL/convex_hull_2.h>
#include <CGAL/predicates_on_points_2.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>

namespace cgshop2026 {

namespace {

struct Edge {
  int first;
  int second;
  bool operator==(const Edge &other) const noexcept {
    return first == other.first && second == other.second;
  }
};

struct EdgeHash {
  std::size_t operator()(const Edge &edge) const noexcept {
    return (static_cast<std::size_t>(edge.first) << 32) ^
           static_cast<std::size_t>(edge.second);
  }
};

inline Edge normalize_edge(int u, int v) {
  return (u < v) ? Edge{u, v} : Edge{v, u};
}

inline Edge normalize_edge(const std::tuple<int, int> &edge) {
  return normalize_edge(std::get<0>(edge), std::get<1>(edge));
}

struct CandidateEdge {
  Edge edge;
  Edge partner;
  int opp1;
  int opp2;
};

struct EdgeBucket {
  Edge key{};
  bool used = false;
  bool blocked = false;
  bool candidate_valid = false;
  uint8_t triangle_count = 0;
  std::array<std::array<int, 3>, 2> triangles{};
  CandidateEdge candidate{};
};

constexpr size_t MAX_POINTS = 15000;
constexpr size_t MAX_EDGES_PER_TRIANG = 60000;
constexpr size_t EDGE_TABLE_SIZE = 1 << 17;
constexpr size_t EDGE_TABLE_MASK = EDGE_TABLE_SIZE - 1;

thread_local std::array<EdgeBucket, EDGE_TABLE_SIZE> g_edge_table;
thread_local std::array<Edge, MAX_EDGES_PER_TRIANG> g_candidate_order_buf;
thread_local std::array<Edge, MAX_EDGES_PER_TRIANG> g_offender_buf;
thread_local std::array<Edge, MAX_EDGES_PER_TRIANG> g_selected_buf;

inline std::array<int, 3>
to_triangle(const std::tuple<int, int, int> &tri_tpl) {
  return {std::get<0>(tri_tpl), std::get<1>(tri_tpl), std::get<2>(tri_tpl)};
}

inline int opposite_vertex(const std::array<int, 3> &tri, const Edge &edge) {
  for (int vertex : tri) {
    if (vertex != edge.first && vertex != edge.second) {
      return vertex;
    }
  }
  throw std::runtime_error("Triangle missing opposite vertex.");
}

inline std::vector<Edge>
conflicting_edges_from_candidate(const CandidateEdge &candidate) {
  std::vector<Edge> conflicts;
  conflicts.reserve(4);
  const int u = candidate.edge.first;
  const int v = candidate.edge.second;
  conflicts.push_back(normalize_edge(u, candidate.opp1));
  conflicts.push_back(normalize_edge(v, candidate.opp1));
  conflicts.push_back(normalize_edge(u, candidate.opp2));
  conflicts.push_back(normalize_edge(v, candidate.opp2));
  return conflicts;
}

inline void clear_edge_table(std::array<EdgeBucket, EDGE_TABLE_SIZE> &table) {
  for (auto &bucket : table) {
    bucket.used = false;
    bucket.blocked = false;
    bucket.candidate_valid = false;
    bucket.triangle_count = 0;
  }
}

inline EdgeBucket &insert_bucket(std::array<EdgeBucket, EDGE_TABLE_SIZE> &table,
                                 const Edge &edge) {
  size_t idx =
      (static_cast<size_t>(edge.first) * 73856093u ^
       static_cast<size_t>(edge.second) * 19349663u) &
      EDGE_TABLE_MASK;
  while (true) {
    EdgeBucket &bucket = table[idx];
    if (!bucket.used) {
      bucket.used = true;
      bucket.key = edge;
      bucket.blocked = false;
      bucket.candidate_valid = false;
      bucket.triangle_count = 0;
      return bucket;
    }
    if (bucket.key == edge) {
      return bucket;
    }
    idx = (idx + 1) & EDGE_TABLE_MASK;
  }
}

inline EdgeBucket *find_bucket(std::array<EdgeBucket, EDGE_TABLE_SIZE> &table,
                               const Edge &edge) {
  size_t idx =
      (static_cast<size_t>(edge.first) * 73856093u ^
       static_cast<size_t>(edge.second) * 19349663u) &
      EDGE_TABLE_MASK;
  while (true) {
    EdgeBucket &bucket = table[idx];
    if (!bucket.used) {
      return nullptr;
    }
    if (bucket.key == edge) {
      return &bucket;
    }
    idx = (idx + 1) & EDGE_TABLE_MASK;
  }
}

inline std::vector<std::tuple<int, int>>
edges_vector_from_set(const std::unordered_set<Edge, EdgeHash> &edges) {
  std::vector<std::tuple<int, int>> vec;
  vec.reserve(edges.size());
  for (const auto &edge : edges) {
    vec.emplace_back(edge.first, edge.second);
  }
  return vec;
}

} // namespace

/**
 * Two segments cross if they intersect in a point that is not an endpoint.
 * No endpoint is allowed to lie on the other segment.
 */
std::vector<std::vector<std::tuple<int, int>>>
compute_delaunay_batches(const std::vector<Point> &points,
                         const std::vector<std::tuple<int, int>> &edges) {
  std::unordered_set<Edge, EdgeHash> edge_set;
  edge_set.reserve(edges.size() * 2);
  for (const auto &edge_tpl : edges) {
    edge_set.insert(normalize_edge(edge_tpl));
  }
  std::vector<std::vector<std::tuple<int, int>>> batches;
  batches.reserve(64);
  while (true) {
    clear_edge_table(g_edge_table);
    auto edge_vec = edges_vector_from_set(edge_set);
    const auto triangles = compute_triangles(points, edge_vec);
    for (const auto &tri_tpl : triangles) {
      const auto tri = to_triangle(tri_tpl);
      const Edge e1 = normalize_edge(tri[0], tri[1]);
      const Edge e2 = normalize_edge(tri[1], tri[2]);
      const Edge e3 = normalize_edge(tri[2], tri[0]);
      Edge tri_edges[3] = {e1, e2, e3};
      for (const auto &edge : tri_edges) {
        EdgeBucket &bucket = insert_bucket(g_edge_table, edge);
        if (bucket.triangle_count < 2) {
          bucket.triangles[bucket.triangle_count++] = tri;
        }
      }
    }

    size_t candidate_count = 0;
    for (auto &bucket : g_edge_table) {
      if (!bucket.used || bucket.triangle_count != 2) {
        continue;
      }
      const Edge &edge = bucket.key;
      const int opp1 = opposite_vertex(bucket.triangles[0], edge);
      const int opp2 = opposite_vertex(bucket.triangles[1], edge);
      const Segment2 diag(points[edge.first], points[edge.second]);
      const Segment2 partner(points[opp1], points[opp2]);
      if (!do_cross(diag, partner)) {
        continue;
      }
      if (!violates_local_delaunay(points[edge.first], points[edge.second],
                                   points[opp1], points[opp2])) {
        continue;
      }
      bucket.candidate_valid = true;
      bucket.blocked = false;
      bucket.candidate =
          CandidateEdge{edge, normalize_edge(opp1, opp2), opp1, opp2};
      g_candidate_order_buf[candidate_count++] = edge;
      if (candidate_count >= MAX_EDGES_PER_TRIANG) {
        break;
      }
    }

    if (candidate_count == 0) {
      break;
    }

    size_t selected_count = 0;
    for (size_t i = 0; i < candidate_count; ++i) {
      Edge edge = g_candidate_order_buf[i];
      EdgeBucket *bucket = find_bucket(g_edge_table, edge);
      if (!bucket || bucket->blocked || !bucket->candidate_valid) {
        continue;
      }
      g_selected_buf[selected_count++] = edge;
      bucket->blocked = true;
      for (const auto &conflict :
           conflicting_edges_from_candidate(bucket->candidate)) {
        if (EdgeBucket *conf_bucket = find_bucket(g_edge_table, conflict)) {
          conf_bucket->blocked = true;
        }
      }
    }

    if (selected_count == 0) {
      break;
    }

    std::vector<std::tuple<int, int>> batch;
    batch.reserve(selected_count);
    for (size_t i = 0; i < selected_count; ++i) {
      const Edge &edge = g_selected_buf[i];
      batch.emplace_back(edge.first, edge.second);
      if (EdgeBucket *bucket = find_bucket(g_edge_table, edge)) {
        edge_set.erase(edge);
        edge_set.insert(bucket->candidate.partner);
      }
    }
    batches.push_back(std::move(batch));
  }
  return batches;
}

bool do_cross(const Segment2 &s1, const Segment2 &s2) {
  auto result = CGAL::intersection(s1, s2);
  if (result) {
    if (const Point *p = std::get_if<Point>(&*result)) {
      // Check if the intersection point is an endpoint of either segment
      if (*p == s1.source() || *p == s1.target() || *p == s2.source() ||
          *p == s2.target()) {
        return false; // Intersection at an endpoint, not a crossing
      }
      return true; // Proper crossing
    }
  }
  return false; // No intersection
}

bool violates_local_delaunay(const Point &a, const Point &b, const Point &c,
                             const Point &d) {
  const auto orient = CGAL::orientation(a, b, c);
  if (orient == CGAL::COLLINEAR) {
    return false;
  }
  const Point *pa = &a;
  const Point *pb = &b;
  if (orient == CGAL::NEGATIVE) {
    std::swap(pa, pb);
  }
  const auto side = CGAL::side_of_oriented_circle(*pa, *pb, c, d);
  return side == CGAL::ON_POSITIVE_SIDE;
}

std::vector<std::tuple<int, int>>
sample_parallel_batch(const std::vector<Point> &points,
                      const std::vector<std::tuple<int, int>> &edges,
                      const std::vector<std::tuple<int, int>> &candidates,
                      const std::vector<std::tuple<int, int>> &offenders,
                      double random_pick_prob, uint64_t seed) {
  clear_edge_table(g_edge_table);
  const auto triangles = compute_triangles(points, edges);
  for (const auto &tri_tpl : triangles) {
    const auto tri = to_triangle(tri_tpl);
    const Edge e1 = normalize_edge(tri[0], tri[1]);
    const Edge e2 = normalize_edge(tri[1], tri[2]);
    const Edge e3 = normalize_edge(tri[2], tri[0]);
    Edge tri_edges[3] = {e1, e2, e3};
    for (const auto &edge : tri_edges) {
      EdgeBucket &bucket = insert_bucket(g_edge_table, edge);
      if (bucket.triangle_count < 2) {
        bucket.triangles[bucket.triangle_count++] = tri;
      }
    }
  }

  size_t candidate_count = 0;
  for (const auto &edge_tpl : candidates) {
    Edge edge = normalize_edge(edge_tpl);
    EdgeBucket *bucket = find_bucket(g_edge_table, edge);
    if (!bucket || bucket->triangle_count != 2) {
      continue;
    }
    const int opp1 = opposite_vertex(bucket->triangles[0], edge);
    const int opp2 = opposite_vertex(bucket->triangles[1], edge);
    const Segment2 diag(points[edge.first], points[edge.second]);
    const Segment2 partner(points[opp1], points[opp2]);
    if (!do_cross(diag, partner)) {
      continue;
    }
    if (!violates_local_delaunay(points[edge.first], points[edge.second],
                                 points[opp1], points[opp2])) {
      continue;
    }
    bucket->candidate_valid = true;
    bucket->candidate =
        CandidateEdge{edge, normalize_edge(opp1, opp2), opp1, opp2};
    g_candidate_order_buf[candidate_count++] = edge;
    if (candidate_count >= MAX_EDGES_PER_TRIANG) {
      break;
    }
  }

  if (candidate_count == 0) {
    return {};
  }

  size_t offender_count = 0;
  for (const auto &edge_tpl : offenders) {
    Edge edge = normalize_edge(edge_tpl);
    EdgeBucket *bucket = find_bucket(g_edge_table, edge);
    if (!bucket || !bucket->candidate_valid) {
      continue;
    }
    g_offender_buf[offender_count++] = edge;
    if (offender_count >= MAX_EDGES_PER_TRIANG) {
      break;
    }
  }

  std::mt19937 rng(static_cast<std::mt19937::result_type>(seed));
  for (size_t i = candidate_count; i > 1; --i) {
    std::uniform_int_distribution<size_t> dist(0, i - 1);
    size_t j = dist(rng);
    std::swap(g_candidate_order_buf[i - 1], g_candidate_order_buf[j]);
  }

  std::uniform_real_distribution<double> real_dist(0.0, 1.0);
  size_t selected_count = 0;
  auto random_offender = [&]() -> Edge {
    std::uniform_int_distribution<size_t> dist(0, offender_count - 1);
    return g_offender_buf[dist(rng)];
  };

  for (size_t i = 0; i < candidate_count; ++i) {
    Edge chosen = g_candidate_order_buf[i];
    bool use_candidate =
        offender_count == 0 || real_dist(rng) < random_pick_prob;
    if (!use_candidate && offender_count > 0) {
      chosen = random_offender();
    }
    EdgeBucket *bucket = find_bucket(g_edge_table, chosen);
    if (!bucket || bucket->blocked || !bucket->candidate_valid) {
      continue;
    }
    g_selected_buf[selected_count++] = chosen;
    bucket->blocked = true;
    for (const auto &conflict :
         conflicting_edges_from_candidate(bucket->candidate)) {
      if (EdgeBucket *conf_bucket = find_bucket(g_edge_table, conflict)) {
        conf_bucket->blocked = true;
      }
    }
  }

  std::vector<std::tuple<int, int>> result;
  result.reserve(selected_count);
  for (size_t i = 0; i < selected_count; ++i) {
    result.emplace_back(g_selected_buf[i].first, g_selected_buf[i].second);
  }
  return result;
}



// ============================================================================
// is_triangulation
// ============================================================================
// is_triangulation - Main validation function
// ============================================================================

/**
 * This function checks if the given set of edges forms a triangulation of the
 * provided points. It uses the CGAL arrangement data structure to insert the
 * edges and verify the triangulation properties.
 */
bool is_triangulation(const std::vector<Point> &points,
                      const std::vector<std::tuple<int, int>> &edges,
                      bool verbose) {
  if (verbose) {
    fmt::print("Validating triangulation with {} points and {} edges.\n",
               points.size(), edges.size());
  }

  // Step 1: Build point-to-index mapping and check for duplicates
  const auto idx_of_opt = build_point_index_map(points, verbose);
  if (!idx_of_opt) {
    return false;
  }
  const auto &idx_of = *idx_of_opt;

  // Step 2: Create arrangement and insert edges
  Arrangement_2 arrangement;
  PointLocation point_location(arrangement);

  if (!insert_edges_into_arrangement(points, edges, arrangement, point_location,
                                     verbose)) {
    return false;
  }

  // Step 3: Add convex hull edges
  add_convex_hull_to_arrangement(points, arrangement, point_location, verbose);

  // Step 4: Validate vertex count (no new intersections, no missing points)
  if (!validate_vertex_count(arrangement, points.size(), points, verbose)) {
    return false;
  }

  // Step 5: Validate all faces are triangular and collect edges
  // Reserve space: triangulation of n points has ~3n edges (Euler's formula)
  std::unordered_set<std::tuple<int, int>, TupleHash> edges_in_arrangement;
  edges_in_arrangement.reserve(3 * points.size());

  if (!validate_all_faces_triangular(arrangement, idx_of, edges_in_arrangement,
                                     verbose)) {
    return false;
  }

  // Step 6: Verify all input edges appear in the arrangement
  if (!validate_input_edges_present(edges, edges_in_arrangement, verbose)) {
    return false;
  }

  // Success
  if (verbose) {
    fmt::print("Triangulation validation complete: Valid triangulation\n");
  }

  return true;
}

// ============================================================================
// Helper functions for compute_triangles
// ============================================================================

/**
 * Build arrangement from points and edges, including convex hull.
 */
static void
build_arrangement_for_triangles(const std::vector<Point> &points,
                                const std::vector<std::tuple<int, int>> &edges,
                                Arrangement_2 &arrangement,
                                PointLocation &point_location) {

  // Insert input edges
  for (const auto &edge : edges) {
    const int i = std::get<0>(edge);
    const int j = std::get<1>(edge);
    if (i < 0 || i >= static_cast<int>(points.size()) || j < 0 ||
        j >= static_cast<int>(points.size())) {
      throw std::runtime_error("Edge indices are out of bounds.");
    }
    const Segment2 seg(points[i], points[j]);
    CGAL::insert(arrangement, seg, point_location);
  }

  // Add convex hull edges
  std::vector<Point> hull;
  hull.reserve(points.size());
  CGAL::convex_hull_2(points.begin(), points.end(), std::back_inserter(hull));
  for (size_t k = 0; k < hull.size(); ++k) {
    const Point &p1 = hull[k];
    const Point &p2 = hull[(k + 1) % hull.size()];
    const Segment2 hull_edge(p1, p2);
    CGAL::insert(arrangement, hull_edge, point_location);
  }
}

/**
 * Extract triangular faces from the arrangement.
 * Expects that all bounded faces are triangles and will throw if not.
 */
static std::vector<std::tuple<int, int, int>>
extract_triangular_faces(const Arrangement_2 &arrangement,
                         const std::map<Point, int, LessPointXY> &idx_of) {

  std::vector<std::tuple<int, int, int>> triangles;
  triangles.reserve(arrangement.number_of_faces());

  for (auto fit = arrangement.faces_begin(); fit != arrangement.faces_end();
       ++fit) {
    if (fit->is_unbounded())
      continue;

    // Walk the boundary and collect vertex indices
    std::array<int, 3> idxs;
    int deg = 0;

    Halfedge_const_handle e = fit->outer_ccb();
    Halfedge_const_handle start = e;

    do {
      if (deg > 3) {
        throw std::runtime_error("Bound face is not triangular.");
      }; // Early out: not a triangle

      const Point &pv = e->source()->point();
      auto it = idx_of.find(pv);
      if (it == idx_of.end()) {
        // Vertex not in original points (likely intersection) - skip face
        throw std::runtime_error(
            "Face vertex not found in original points list.");
      }
      if (deg < 3)
        idxs[deg] = it->second;
      ++deg;

      e = e->next();
    } while (e != start);

    if (deg == 3) {
      // Canonicalize order and add triangle
      std::sort(idxs.begin(), idxs.end());
      triangles.emplace_back(idxs[0], idxs[1], idxs[2]);
    }
  }

  return triangles;
}

// ============================================================================
// compute_triangles - Main function
// ============================================================================

/**
 * This function computes all triangles formed by the given set of points and
 * edges. It returns a list of triangles, where each triangle is represented by
 * a tuple of three point indices. Edges that appear only once will be on the
 * convex hull. Otherwise, all edges should appear exactly twice. The indices
 * will be sorted in each triangle, and the list of triangles will also be
 * sorted.
 * Expects that all bounded faces are triangles and will throw if not.
 */
std::vector<std::tuple<int, int, int>>
compute_triangles(const std::vector<Point> &points,
                  const std::vector<std::tuple<int, int>> &edges) {
  // Step 1: Build point-to-index mapping
  std::map<Point, int, LessPointXY> idx_of;
  for (int i = 0; i < static_cast<int>(points.size()); ++i) {
    idx_of.emplace(points[i], i);
  }

  // Step 2: Build arrangement with edges and convex hull
  Arrangement_2 arrangement;
  PointLocation point_location(arrangement);
  build_arrangement_for_triangles(points, edges, arrangement, point_location);

  // Step 3: Extract triangular faces
  std::vector<std::tuple<int, int, int>> triangles =
      extract_triangular_faces(arrangement, idx_of);

  // Step 4: Sort
  std::sort(triangles.begin(), triangles.end());
  const auto num_triangles = triangles.size();
  triangles.erase(std::unique(triangles.begin(), triangles.end()),
                  triangles.end());
  if (triangles.size() != num_triangles) {
    throw std::runtime_error(
        "Duplicate triangles found after extraction. This should not happen.");
  }

  return triangles;
}

} // namespace cgshop2026
