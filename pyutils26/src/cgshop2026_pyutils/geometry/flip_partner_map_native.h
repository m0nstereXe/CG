#pragma once

#include "cgal_types.h"
#include "geometry_operations.h"

#include <array>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cgshop2026 {

class FlipPartnerMapNative {
public:
  FlipPartnerMapNative(const std::vector<Point> &points,
                       const std::vector<std::tuple<int, int>> &edges);

  std::vector<std::tuple<int, int, int>> compute_triangles() const;
  bool is_flippable(const std::tuple<int, int> &edge) const;
  std::vector<std::tuple<int, int>> flippable_edges() const;
  std::tuple<int, int> get_flip_partner(const std::tuple<int, int> &edge) const;
  std::vector<std::tuple<int, int>>
  conflicting_flips(const std::tuple<int, int> &edge) const;
  std::tuple<int, int> flip(const std::tuple<int, int> &edge);
  std::vector<std::tuple<int, int>> edges() const;
  std::vector<Point> points() const;
  std::unique_ptr<FlipPartnerMapNative> deep_copy() const;

private:
  struct EdgeKey {
    int u;
    int v;
    EdgeKey() : u(0), v(0) {}
    EdgeKey(int a, int b) {
      if (a <= b) {
        u = a;
        v = b;
      } else {
        u = b;
        v = a;
      }
    }
    bool operator==(const EdgeKey &other) const {
      return u == other.u && v == other.v;
    }
  };

  struct EdgeHash {
    std::size_t operator()(const EdgeKey &edge) const noexcept {
      return (static_cast<std::size_t>(edge.u) << 32) ^
             static_cast<std::size_t>(edge.v);
    }
  };

  using Triangle = std::array<int, 3>;

  EdgeKey normalize_edge(const std::tuple<int, int> &edge) const;
  std::tuple<int, int> to_tuple(const EdgeKey &edge) const;
  int opposite_vertex(const Triangle &tri, const EdgeKey &edge) const;
  std::optional<EdgeKey> check_flippability(const EdgeKey &edge,
                                            const Triangle &tri1,
                                            const Triangle &tri2) const;
  void rebuild_flip_map();
  void update_flip_partner(const EdgeKey &edge);
  bool contains_old_edge(const Triangle &tri, const EdgeKey &edge) const;
  std::vector<std::tuple<int, int>> edge_list() const;

  std::vector<Point> points_;
  std::unordered_set<EdgeKey, EdgeHash> edges_;
  std::unordered_map<EdgeKey, std::vector<Triangle>, EdgeHash>
      edge_to_triangles_;
  std::unordered_map<EdgeKey, EdgeKey, EdgeHash> flip_map_;
};

} // namespace cgshop2026
