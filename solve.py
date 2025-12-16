"""Batch solver CLI that writes CGSHOP2026 solutions to a zip archive."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys
import time

REPO_ROOT = Path(__file__).resolve().parent
PYUTILS_SRC = REPO_ROOT / "pyutils26" / "src"
if PYUTILS_SRC.exists() and str(PYUTILS_SRC) not in sys.path:
    sys.path.insert(0, str(PYUTILS_SRC))
VENV_SITE = (
    REPO_ROOT
    / ".venv"
    / f"lib/python{sys.version_info.major}.{sys.version_info.minor}/site-packages"
)
if VENV_SITE.exists() and str(VENV_SITE) not in sys.path:
    sys.path.insert(0, str(VENV_SITE))

from cgshop2026_pyutils.io import read_instance
from cgshop2026_pyutils.verify import check_for_errors
from cgshop2026_pyutils.zip.zip_writer import ZipWriter

from main import solve_instance, solution_metrics


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Solve every CGSHOP 2026 instance JSON in a directory and bundle the "
            "solutions into solutions.zip."
        )
    )
    parser.add_argument(
        "instances_dir",
        type=Path,
        help="Directory that contains CGSHOP2026 instance JSON files.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("solutions.zip"),
        help="Destination zip archive (defaults to ./solutions.zip).",
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Verify each generated solution with cgshop2026_pyutils.verify.check_for_errors.",
    )
    return parser.parse_args()


def json_instances(instances_dir: Path) -> list[Path]:
    """Return sorted JSON files that should be solved."""
    return sorted(
        path
        for path in instances_dir.glob("*.json")
        if path.is_file()
    )


def main() -> None:
    args = parse_args()
    if not args.instances_dir.exists():
        raise SystemExit(f"Instance directory not found: {args.instances_dir}")
    if not args.instances_dir.is_dir():
        raise SystemExit(f"Not a directory: {args.instances_dir}")

    instance_files = json_instances(args.instances_dir)
    if not instance_files:
        raise SystemExit(f"No .json instances found in {args.instances_dir}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists():
        args.output.unlink()
    total = len(instance_files)
    total_start = time.perf_counter()
    with ZipWriter(args.output) as archive:
        for idx, instance_path in enumerate(instance_files, start=1):
            instance_start = time.perf_counter()
            instance = read_instance(instance_path)
            solution = solve_instance(instance)
            if args.verify:
                errors = check_for_errors(instance, solution, full_recompute=True)
                if errors:
                    raise SystemExit(
                        f"Verification failed for {instance_path.name}:\n"
                        + "\n".join(f"- {msg}" for msg in errors)
                    )
            archive.add_solution(solution)
            instance_elapsed = time.perf_counter() - instance_start
            total_flips, total_steps = solution_metrics(solution)
            percent = idx / total * 100
            print(
                f"[{idx}/{total} | {percent:5.1f}%] "
                f"Solved {instance_path.name} -> {solution.instance_uid}.solution.json "
                f"in {instance_elapsed:.2f}s "
                f"({total_flips} flips / {total_steps} steps)"
            )
    total_elapsed = time.perf_counter() - total_start
    print(
        f"Wrote {len(instance_files)} solutions to {args.output} "
        f"in {total_elapsed:.2f}s"
    )


if __name__ == "__main__":
    main()
