from __future__ import annotations

from ._bindings import (
    FlipPartnerMapNative as _FlipPartnerMapNative,
    Point,
    is_triangulation,
)
from .typing import Edge


def normalize_edge(v: int, w: int) -> Edge:
    """Returns a tuple representing the edge in a consistent order (min, max)."""
    return (v, w) if v < w else (w, v)


class FlipPartnerMap:
    """Lightweight Python wrapper around the native FlipPartnerMap implementation."""

    def __init__(self, impl: _FlipPartnerMapNative):
        self._impl = impl

    @staticmethod
    def build(points: list[Point], edges: list[tuple[int, int]]) -> "FlipPartnerMap":
        """Construct a FlipPartnerMap for the given triangulation."""
        return FlipPartnerMap(_FlipPartnerMapNative(points, edges))

    def compute_triangles(self) -> list[tuple[int, int, int]]:
        return self._impl.compute_triangles()

    def is_flippable(self, edge: tuple[int, int]) -> bool:
        return self._impl.is_flippable(edge)

    def conflicting_flips(self, edge: tuple[int, int]) -> set[tuple[int, int]]:
        return set(map(tuple, self._impl.conflicting_flips(edge)))

    def flip(self, edge: tuple[int, int]) -> tuple[int, int]:
        return self._impl.flip(edge)

    def deep_copy(self) -> "FlipPartnerMap":
        return FlipPartnerMap(self._impl.deep_copy())

    def flippable_edges(self) -> list[tuple[int, int]]:
        return self._impl.flippable_edges()

    def get_flip_partner(self, edge: tuple[int, int]) -> tuple[int, int]:
        return self._impl.get_flip_partner(edge)

    @property
    def edges(self) -> set[tuple[int, int]]:
        return set(map(tuple, self._impl.edges()))

    @property
    def points(self) -> list[Point]:
        return list(self._impl.points())


def expand_edges_by_convex_hull_edges(
    points: list[Point], edges: list[tuple[int, int]]
) -> list[tuple[int, int]]:
    """
    Expands the given set of edges by adding the edges of the convex hull of the points.
    This ensures that the triangulation is valid and covers the entire convex hull.

    Args:
        points: List of Point objects representing the vertices.
        edges: List of edges represented as tuples of vertex indices.

    Returns:
        A new list of edges that includes the original edges and the convex hull edges.
    """
    if not is_triangulation(points, edges):
        raise ValueError(
            "The provided edges do not form a valid triangulation of the given points."
        )
    flip_partner_map = FlipPartnerMap.build(points, edges)
    return list(flip_partner_map.edges)

