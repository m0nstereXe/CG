"""Minimal CGSHOP 2026 solver CLI.

Reads an instance JSON file and prints a CGSHOP2026Solution JSON that brings all
triangulations to the (presumably unique) Delaunay triangulation via flips.
"""

from __future__ import annotations

import argparse
import time
from pathlib import Path

from cgshop2026_pyutils.geometry import Point, compute_local_delaunay_flip_batches
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


def solve_instance(instance: CGSHOP2026Instance) -> CGSHOP2026Solution:
    """Return a CGSHOP2026Solution JSONable object."""
    points = instance_points(instance)
    return CGSHOP2026Solution(
        instance_uid=instance.instance_uid,
        flips=compute_local_delaunay_flip_batches(points, instance.triangulations),
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
