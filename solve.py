"""Batch solver CLI that writes CGSHOP2026 solutions to a zip archive."""

from __future__ import annotations

import argparse
from pathlib import Path
import time
import zipfile

from cgshop2026_pyutils.io import read_instance
from cgshop2026_pyutils.verify import check_for_errors

from main import solve_instance


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


def entry_name(instance_uid: str) -> str:
    """Ensure the zip member has a .json suffix."""
    return instance_uid if instance_uid.endswith(".json") else f"{instance_uid}.json"


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
    total = len(instance_files)
    total_start = time.perf_counter()
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
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
            archive.writestr(
                entry_name(solution.instance_uid),
                solution.model_dump_json(indent=2).encode("utf-8"),
            )
            instance_elapsed = time.perf_counter() - instance_start
            percent = idx / total * 100
            print(
                f"[{idx}/{total} | {percent:5.1f}%] "
                f"Solved {instance_path.name} -> {entry_name(solution.instance_uid)} "
                f"in {instance_elapsed:.2f}s"
            )
    total_elapsed = time.perf_counter() - total_start
    print(
        f"Wrote {len(instance_files)} solutions to {args.output} "
        f"in {total_elapsed:.2f}s"
    )


if __name__ == "__main__":
    main()
