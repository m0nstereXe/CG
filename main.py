"""Minimal CGSHOP 2026 solver CLI.

Reads an instance JSON file and prints a CGSHOP2026Solution JSON that brings all
triangulations to the (presumably unique) Delaunay triangulation via flips.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from cgshop2026_pyutils.geometry import Point, FlippableTriangulation
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


def point_xy(p: Point) -> tuple[int, int]:
    """Return integer coordinates of the CGAL point."""
    return int(float(p.x())), int(float(p.y()))


def orientation(a: Point, b: Point, c: Point) -> int:
    """Return twice the signed area of the triangle (a,b,c)."""
    ax, ay = point_xy(a)
    bx, by = point_xy(b)
    cx, cy = point_xy(c)
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)


def incircle(a: Point, b: Point, c: Point, d: Point) -> int:
    """Return the determinant that is > 0 iff d lies inside circumcircle of (a,b,c) (CCW)."""
    ax, ay = point_xy(a)
    bx, by = point_xy(b)
    cx, cy = point_xy(c)
    dx, dy = point_xy(d)
    ax -= dx
    ay -= dy
    bx -= dx
    by -= dy
    cx -= dx
    cy -= dy
    a_len = ax * ax + ay * ay
    b_len = bx * bx + by * by
    c_len = cx * cx + cy * cy
    return (
        a_len * (bx * cy - cx * by)
        - b_len * (ax * cy - cx * ay)
        + c_len * (ax * by - bx * ay)
    )


def violates_local_delaunay(
    points: list[Point], edge: tuple[int, int], opposite: tuple[int, int]
) -> bool:
    """Check whether the shared edge fails the empty circumcircle test."""
    a_idx, b_idx = edge
    c_idx, d_idx = opposite
    a, b, c, d = points[a_idx], points[b_idx], points[c_idx], points[d_idx]
    orient = orientation(a, b, c)
    if orient == 0:
        return False
    if orient < 0:
        a, b = b, a
    return incircle(a, b, c, d) > 0


def flip_to_delaunay(
    triangulation: FlippableTriangulation, points: list[Point]
) -> list[tuple[int, int]]:
    """Flip all non-Delaunay edges; returns the sequence of flipped edges."""
    flip_log: list[tuple[int, int]] = []
    while True:
        flipped = False
        for edge in triangulation.possible_flips():
            opposite = triangulation.get_flip_partner(edge)
            if violates_local_delaunay(points, edge, opposite):
                triangulation.add_flip(edge)
                triangulation.commit()
                flip_log.append(edge)
                flipped = True
                break
        if not flipped:
            break
    return flip_log


def solve_instance(instance: CGSHOP2026Instance) -> CGSHOP2026Solution:
    """Return a CGSHOP2026Solution JSONable object."""
    points = instance_points(instance)
    triangulations = build_triangulations(instance, points)
    all_flips: list[list[list[tuple[int, int]]]] = []
    for triangulation in triangulations:
        flip_sequence = flip_to_delaunay(triangulation, points)
        all_flips.append([[edge] for edge in flip_sequence])
    return CGSHOP2026Solution(
        instance_uid=instance.instance_uid,
        flips=all_flips,
        meta={"algorithm": "local_delaunay_flips"},
    )

def main() -> None:
    args = parse_args()
    try:
        instance = read_instance(args.instance)
    except FileNotFoundError:
        raise SystemExit(f"Instance file not found: {args.instance}") from None

    solution = solve_instance(instance)
    if args.verify:
        errors = check_for_errors(instance, solution, full_recompute=True)
        if errors:
            raise SystemExit(
                "Solution verification failed:\n" + "\n".join(f"- {msg}" for msg in errors)
            )
        print("Solution verified: no errors detected.")
    print(solution.model_dump_json(indent=2))


if __name__ == "__main__":
    main()
