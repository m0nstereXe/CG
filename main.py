"""Minimal CGSHOP 2026 solver CLI.

Reads an instance JSON file and prints a CGSHOP2026Solution JSON that brings all
triangulations to the (presumably unique) Delaunay triangulation via flips.
"""

from __future__ import annotations

import argparse
import time
from pathlib import Path

from cgshop2026_pyutils.geometry import (
    Point,
    FlippableTriangulation,
    violates_local_delaunay as cgal_violates_local_delaunay,
)
from cgshop2026_pyutils.schemas import CGSHOP2026Instance, CGSHOP2026Solution
from cgshop2026_pyutils.io import read_instance
from cgshop2026_pyutils.verify import check_for_errors


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


def flip_to_delaunay(
    triangulation: FlippableTriangulation, points: list[Point]
) -> list[list[tuple[int, int]]]:
    """Flip all non-Delaunay edges; returns batches of concurrently flipped edges."""
    batches: list[list[tuple[int, int]]] = []
    while True:
        pending_batch: list[tuple[int, int]] = []
        for edge in triangulation.possible_flips():
            opposite = triangulation.get_flip_partner(edge)
            a_idx, b_idx = edge
            c_idx, d_idx = opposite
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
