#include "flip_partner_map_native.h"

#include <algorithm>
#include <stdexcept>

namespace cgshop2026 {

FlipPartnerMapNative::FlipPartnerMapNative(
    const std::vector<Point> &points,
    const std::vector<std::tuple<int, int>> &edges)
    : points_(points) {
  edges_.reserve(edges.size());
  for (const auto &edge : edges) {
    edges_.insert(normalize_edge(edge));
  }
  rebuild_flip_map();
}

std::vector<std::tuple<int, int, int>>
FlipPartnerMapNative::compute_triangles() const {
  return cgshop2026::compute_triangles(points_, edge_list());
}

bool FlipPartnerMapNative::is_flippable(const std::tuple<int, int> &edge) const {
  EdgeKey key = normalize_edge(edge);
  return flip_map_.find(key) != flip_map_.end();
}

std::vector<std::tuple<int, int>>
FlipPartnerMapNative::flippable_edges() const {
  std::vector<std::tuple<int, int>> result;
  result.reserve(flip_map_.size());
  for (const auto &entry : flip_map_) {
    result.emplace_back(entry.first.u, entry.first.v);
  }
  return result;
}

std::tuple<int, int>
FlipPartnerMapNative::get_flip_partner(const std::tuple<int, int> &edge) const {
  EdgeKey key = normalize_edge(edge);
  auto it = flip_map_.find(key);
  if (it == flip_map_.end()) {
    throw std::runtime_error("Edge is not flippable");
  }
  return to_tuple(it->second);
}

std::vector<std::tuple<int, int>> FlipPartnerMapNative::conflicting_flips(
    const std::tuple<int, int> &edge) const {
  EdgeKey key = normalize_edge(edge);
  auto it = flip_map_.find(key);
  if (it == flip_map_.end()) {
    throw std::runtime_error("Edge is not flippable");
  }
  const EdgeKey &partner = it->second;
  std::vector<std::tuple<int, int>> conflicts;
  conflicts.reserve(4);
  auto add_if_flippable = [&](int a, int b) {
    EdgeKey candidate(a, b);
    if (flip_map_.find(candidate) != flip_map_.end()) {
      conflicts.emplace_back(candidate.u, candidate.v);
    }
  };
  add_if_flippable(key.u, partner.u);
  add_if_flippable(key.v, partner.u);
  add_if_flippable(key.u, partner.v);
  add_if_flippable(key.v, partner.v);
  return conflicts;
}

std::tuple<int, int>
FlipPartnerMapNative::flip(const std::tuple<int, int> &edge) {
  EdgeKey old_edge = normalize_edge(edge);
  if (edges_.find(old_edge) == edges_.end()) {
    throw std::runtime_error("Edge does not exist in the triangulation");
  }
  auto it_flip = flip_map_.find(old_edge);
  if (it_flip == flip_map_.end()) {
    throw std::runtime_error("Edge is not flippable");
  }
  EdgeKey new_edge = it_flip->second;
  flip_map_.erase(it_flip);

  auto tri_it = edge_to_triangles_.find(old_edge);
  if (tri_it == edge_to_triangles_.end() || tri_it->second.size() != 2) {
    throw std::runtime_error("Edge must be adjacent to exactly two triangles");
  }
  auto incident_tris = tri_it->second;
  edge_to_triangles_.erase(tri_it);

  Triangle new_tri_0{new_edge.u, new_edge.v, old_edge.u};
  Triangle new_tri_1{new_edge.u, new_edge.v, old_edge.v};
  edge_to_triangles_[new_edge] = {new_tri_0, new_tri_1};

  auto update_incident = [&](const EdgeKey &adj_edge, const Triangle &tri) {
    auto &vec = edge_to_triangles_[adj_edge];
    vec.erase(std::remove_if(vec.begin(), vec.end(),
                             [&](const Triangle &t) {
                               return contains_old_edge(t, old_edge);
                             }),
              vec.end());
    vec.insert(vec.begin(), tri);
    update_flip_partner(adj_edge);
  };

  update_incident(EdgeKey(new_edge.u, old_edge.u), new_tri_0);
  update_incident(EdgeKey(new_edge.v, old_edge.u), new_tri_0);
  update_incident(EdgeKey(new_edge.u, old_edge.v), new_tri_1);
  update_incident(EdgeKey(new_edge.v, old_edge.v), new_tri_1);

  edges_.erase(old_edge);
  edges_.insert(new_edge);
  update_flip_partner(new_edge);

  return to_tuple(new_edge);
}

std::vector<std::tuple<int, int>> FlipPartnerMapNative::edges() const {
  return edge_list();
}

std::vector<Point> FlipPartnerMapNative::points() const { return points_; }

std::unique_ptr<FlipPartnerMapNative>
FlipPartnerMapNative::deep_copy() const {
  return std::make_unique<FlipPartnerMapNative>(*this);
}

FlipPartnerMapNative::EdgeKey
FlipPartnerMapNative::normalize_edge(const std::tuple<int, int> &edge) const {
  return EdgeKey(std::get<0>(edge), std::get<1>(edge));
}

std::tuple<int, int>
FlipPartnerMapNative::to_tuple(const EdgeKey &edge) const {
  return std::make_tuple(edge.u, edge.v);
}

int FlipPartnerMapNative::opposite_vertex(const Triangle &tri,
                                          const EdgeKey &edge) const {
  for (int idx : tri) {
    if (idx != edge.u && idx != edge.v) {
      return idx;
    }
  }
  throw std::runtime_error("Triangle does not contain an opposite vertex");
}

std::optional<FlipPartnerMapNative::EdgeKey>
FlipPartnerMapNative::check_flippability(const EdgeKey &edge,
                                         const Triangle &tri1,
                                         const Triangle &tri2) const {
  int opp1 = opposite_vertex(tri1, edge);
  int opp2 = opposite_vertex(tri2, edge);
  Segment2 segment_ab(points_[edge.u], points_[edge.v]);
  Segment2 segment_cd(points_[opp1], points_[opp2]);
  if (do_cross(segment_ab, segment_cd)) {
    return EdgeKey(opp1, opp2);
  }
  return std::nullopt;
}

void FlipPartnerMapNative::rebuild_flip_map() {
  edge_to_triangles_.clear();
  flip_map_.clear();

  auto triangles = compute_triangles();
  for (const auto &tri_tuple : triangles) {
    Triangle tri{std::get<0>(tri_tuple), std::get<1>(tri_tuple),
                 std::get<2>(tri_tuple)};
    EdgeKey e1(tri[0], tri[1]);
    EdgeKey e2(tri[1], tri[2]);
    EdgeKey e3(tri[2], tri[0]);
    edges_.insert(e1);
    edges_.insert(e2);
    edges_.insert(e3);
    edge_to_triangles_[e1].push_back(tri);
    edge_to_triangles_[e2].push_back(tri);
    edge_to_triangles_[e3].push_back(tri);
  }

  for (const auto &entry : edge_to_triangles_) {
    update_flip_partner(entry.first);
  }
}

void FlipPartnerMapNative::update_flip_partner(const EdgeKey &edge) {
  auto it = edge_to_triangles_.find(edge);
  if (it == edge_to_triangles_.end()) {
    flip_map_.erase(edge);
    return;
  }
  auto &tris = it->second;
  if (tris.size() == 2) {
    if (auto partner = check_flippability(edge, tris[0], tris[1])) {
      flip_map_[edge] = *partner;
    } else {
      flip_map_.erase(edge);
    }
  } else {
    flip_map_.erase(edge);
  }
}

bool FlipPartnerMapNative::contains_old_edge(const Triangle &tri,
                                             const EdgeKey &edge) const {
  bool has_u = false;
  bool has_v = false;
  for (int idx : tri) {
    if (idx == edge.u) {
      has_u = true;
    } else if (idx == edge.v) {
      has_v = true;
    }
  }
  return has_u && has_v;
}

std::vector<std::tuple<int, int>> FlipPartnerMapNative::edge_list() const {
  std::vector<std::tuple<int, int>> result;
  result.reserve(edges_.size());
  for (const auto &edge : edges_) {
    result.emplace_back(edge.u, edge.v);
  }
  return result;
}

} // namespace cgshop2026
