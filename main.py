"""Minimal CGSHOP 2026 solver CLI.

Reads an instance JSON file and prints a CGSHOP2026Solution JSON that brings all
triangulations to the (presumably unique) Delaunay triangulation via flips.
"""

from __future__ import annotations

import argparse
import random
import time
from collections import deque
from pathlib import Path

from cgshop2026_pyutils.geometry import (
    Point,
    FlippableTriangulation,
    violates_local_delaunay as cgal_violates_local_delaunay,
)
from cgshop2026_pyutils.schemas import CGSHOP2026Instance, CGSHOP2026Solution
from cgshop2026_pyutils.io import read_instance
from cgshop2026_pyutils.verify import check_for_errors


random.seed(0)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Load a CGSHOP 2026 instance (see https://github.com/CG-SHOP/pyutils26) "
            "and print a JSON solution that flips every triangulation to Delaunay."
        )
    )
    parser.add_argument(
        "instance",
        type=Path,
        help="Path to a CGSHOP2026 instance JSON file.",
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Run cgshop2026_pyutils.verify.check_for_errors on the produced solution.",
    )
    return parser.parse_args()
def instance_points(instance: CGSHOP2026Instance) -> list[Point]:
    """Convert the integer coordinate lists into Point objects."""
    return [Point(x, y) for x, y in zip(instance.points_x, instance.points_y)]


def build_triangulations(
    instance: CGSHOP2026Instance, points: list[Point]
) -> list[FlippableTriangulation]:
    """Wrap every edge list of the instance in a FlippableTriangulation."""
    return [
        FlippableTriangulation.from_points_edges(points, edges)
        for edges in instance.triangulations
    ]


def normalize_edge(u: int, v: int) -> tuple[int, int]:
    return (u, v) if u < v else (v, u)


def convex_hull_vertices(
    points: list[Point], subset: set[int]
) -> list[int]:
    """Return the convex hull (ccw) of the provided vertex subset."""
    coords: list[tuple[float, float, int]] = [
        (float(points[idx].x()), float(points[idx].y()), idx) for idx in subset
    ]
    coords.sort()
    if len(coords) <= 1:
        return [coords[0][2]] if coords else []

    def cross(
        o: tuple[float, float, int],
        a: tuple[float, float, int],
        b: tuple[float, float, int],
    ) -> float:
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower: list[tuple[float, float, int]] = []
    for pt in coords:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], pt) <= 0:
            lower.pop()
        lower.append(pt)

    upper: list[tuple[float, float, int]] = []
    for pt in reversed(coords):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], pt) <= 0:
            upper.pop()
        upper.append(pt)

    hull = lower[:-1] + upper[:-1]
    result: list[int] = []
    for _, _, idx in hull:
        if not result or result[-1] != idx:
            result.append(idx)
    return result


def choose_hull_pair(hull: list[int]) -> tuple[int, int] | None:
    """Pick two non-adjacent hull vertices to start/end a cut."""
    n = len(hull)
    if n < 4:
        return None
    indices = list(range(n))
    for _ in range(64):
        i, j = random.sample(indices, 2)
        if abs(i - j) == 1 or abs(i - j) == n - 1:
            continue
        return hull[i], hull[j]
    return None


def build_adjacency(vertices: set[int], edges: list[tuple[int, int]]) -> dict[int, set[int]]:
    adjacency: dict[int, set[int]] = {v: set() for v in vertices}
    for u, v in edges:
        if u in adjacency and v in adjacency:
            adjacency[u].add(v)
            adjacency[v].add(u)
    return adjacency


def find_interior_path(
    adjacency: dict[int, set[int]],
    start: int,
    goal: int,
    hull_edges: set[tuple[int, int]],
) -> list[int] | None:
    queue: deque[int] = deque([start])
    parents: dict[int, int | None] = {start: None}
    while queue:
        node = queue.popleft()
        if node == goal:
            break
        for neighbor in adjacency.get(node, ()):
            edge = normalize_edge(node, neighbor)
            if (
                edge in hull_edges
                and node not in (start, goal)
                and neighbor not in (start, goal)
            ):
                continue
            if neighbor in parents:
                continue
            parents[neighbor] = node
            queue.append(neighbor)
    else:
        return None

    path: list[int] = []
    cur: int | None = goal
    while cur is not None:
        path.append(cur)
        cur = parents[cur]
    path.reverse()
    if not path or path[0] != start or path[-1] != goal:
        return None
    return path


def split_components_from_adj(
    adjacency: dict[int, set[int]], cut_edges: set[tuple[int, int]]
) -> list[set[int]]:
    visited: set[int] = set()
    components: list[set[int]] = []
    for vertex in adjacency:
        if vertex in visited:
            continue
        stack = [vertex]
        visited.add(vertex)
        component: set[int] = set()
        while stack:
            node = stack.pop()
            component.add(node)
            for neighbor in adjacency[node]:
                edge = normalize_edge(node, neighbor)
                if edge in cut_edges:
                    continue
                if neighbor in visited:
                    continue
                visited.add(neighbor)
                stack.append(neighbor)
        components.append(component)
    return components


def restricted_local_flips(
    triangulation: FlippableTriangulation,
    points: list[Point],
    allowed_vertices: set[int],
) -> list[list[tuple[int, int]]]:
    """Run the greedy local Delaunay loop restricted to allowed_vertices."""
    if not allowed_vertices:
        return []
    batches: list[list[tuple[int, int]]] = []
    while True:
        pending_batch: list[tuple[int, int]] = []
        candidates = triangulation.possible_flips()
        random.shuffle(candidates)
        for edge in candidates:
            if edge[0] not in allowed_vertices or edge[1] not in allowed_vertices:
                continue
            c_idx, d_idx = triangulation.get_flip_partner(edge)
            if c_idx not in allowed_vertices or d_idx not in allowed_vertices:
                continue
            if not cgal_violates_local_delaunay(
                points[edge[0]], points[edge[1]], points[c_idx], points[d_idx]
            ):
                continue
            try:
                triangulation.add_flip(edge)
            except ValueError:
                continue
            pending_batch.append(edge)
        if not pending_batch:
            break
        triangulation.commit()
        batches.append(pending_batch)
    return batches


def try_split_component(
    triangulation: FlippableTriangulation,
    points: list[Point],
    allowed_vertices: set[int],
) -> tuple[set[int], set[int]] | None:
    """Attempt to cut the component into two disjoint vertex sets."""
    if len(allowed_vertices) < 6:
        return None
    edges = [
        normalize_edge(*edge)
        for edge in triangulation.get_edges()
        if edge[0] in allowed_vertices and edge[1] in allowed_vertices
    ]
    if not edges:
        return None
    adjacency = build_adjacency(allowed_vertices, edges)
    hull = convex_hull_vertices(points, allowed_vertices)
    if len(hull) < 4:
        return None
    hull_edges = {
        normalize_edge(hull[i], hull[(i + 1) % len(hull)]) for i in range(len(hull))
    }
    for _ in range(32):
        pair = choose_hull_pair(hull)
        if pair is None:
            return None
        path = find_interior_path(adjacency, pair[0], pair[1], hull_edges)
        if not path or len(path) < 2:
            continue
        cut_edges = {
            normalize_edge(path[i], path[i + 1]) for i in range(len(path) - 1)
        }
        components = split_components_from_adj(adjacency, cut_edges)
        components = [comp for comp in components if comp]
        if len(components) == 2:
            return components[0], components[1]
    return None


def divide_and_conquer_batches(
    triangulation: FlippableTriangulation, points: list[Point]
) -> list[list[tuple[int, int]]]:
    """Recursively split the triangulation and solve subproblems."""

    def merge_disjoint_batches(
        left: list[list[tuple[int, int]]],
        right: list[list[tuple[int, int]]],
    ) -> list[list[tuple[int, int]]]:
        if not left:
            return right
        if not right:
            return left
        merged: list[list[tuple[int, int]]] = []
        max_len = max(len(left), len(right))
        for i in range(max_len):
            batch: list[tuple[int, int]] = []
            if i < len(left):
                batch.extend(left[i])
            if i < len(right):
                batch.extend(right[i])
            merged.append(batch)
        return merged

    def solve_subset(allowed_vertices: set[int]) -> list[list[tuple[int, int]]]:
        split = try_split_component(triangulation, points, allowed_vertices)
        if not split:
            return restricted_local_flips(triangulation, points, allowed_vertices)
        left, right = split
        left_batches = solve_subset(left)
        right_batches = solve_subset(right)
        return merge_disjoint_batches(left_batches, right_batches)

    full_vertices = set(range(len(points)))
    return solve_subset(full_vertices)


def flip_to_delaunay(
    triangulation: FlippableTriangulation, points: list[Point]
) -> list[list[tuple[int, int]]]:
    """Flip all non-Delaunay edges; returns batches of concurrently flipped edges."""
    batches: list[list[tuple[int, int]]] = divide_and_conquer_batches(
        triangulation, points
    )
    while True:
        pending_batch: list[tuple[int, int]] = []
        ord = triangulation.possible_flips()
        random.shuffle(ord)
        for edge in ord:
            a_idx, b_idx = edge
            c_idx, d_idx = triangulation.get_flip_partner(edge)
            if cgal_violates_local_delaunay(
                points[a_idx], points[b_idx], points[c_idx], points[d_idx]
            ):
                try:
                    triangulation.add_flip(edge)
                except ValueError:
                    # Another flip in this batch now conflicts with this edge.
                    continue
                pending_batch.append(edge)
        if not pending_batch:
            break
        triangulation.commit()
        batches.append(pending_batch)
    return batches


def solve_instance(instance: CGSHOP2026Instance) -> CGSHOP2026Solution:
    """Return a CGSHOP2026Solution JSONable object."""
    points = instance_points(instance)
    triangulations = build_triangulations(instance, points)
    return CGSHOP2026Solution(
        instance_uid=instance.instance_uid,
        flips=[flip_to_delaunay(tri, points) for tri in triangulations],
        meta={"algorithm": "local_delaunay_flips"},
    )


def solution_metrics(solution: CGSHOP2026Solution) -> tuple[int, int]:
    """Return total flipped edges and total parallel flip steps."""
    total_steps = sum(len(tri_flips) for tri_flips in solution.flips)
    total_flips = sum(
        len(parallel_flips) for tri_flips in solution.flips for parallel_flips in tri_flips
    )
    return total_flips, total_steps


def main() -> None:
    args = parse_args()
    try:
        instance = read_instance(args.instance)
    except FileNotFoundError:
        raise SystemExit(f"Instance file not found: {args.instance}") from None

    start_time = time.perf_counter()
    solution = solve_instance(instance)
    if args.verify:
        errors = check_for_errors(instance, solution, full_recompute=True)
        if errors:
            raise SystemExit(
                "Solution verification failed:\n" + "\n".join(f"- {msg}" for msg in errors)
            )
    elapsed = time.perf_counter() - start_time
    total_flips, total_steps = solution_metrics(solution)
    print(
        f"{solution.instance_uid}: {total_flips} flips across "
        f"{total_steps} parallel steps in {elapsed:.2f}s"
    )


if __name__ == "__main__":
    main()
