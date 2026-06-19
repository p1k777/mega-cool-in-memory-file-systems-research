#!/usr/bin/env python3

from __future__ import annotations

import argparse
import concurrent.futures
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


PROFILE_ALIASES = {
    "build_system": "build_system",
    "build": "build_system",
    "bs": "build_system",
    "file_manager": "file_manager",
    "file": "file_manager",
    "fm": "file_manager",
    "backup": "backup",
    "bu": "backup",
    "refactoring": "refactoring",
    "ref": "refactoring",
    "database": "database",
    "db": "database",
    "web_server": "web_server",
    "web": "web_server",
    "ws": "web_server",
}


@dataclass(frozen=True)
class Job:
    fs_type: str
    profile: str
    output_csv: Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run bench_run as a set of independent jobs. "
            "Each job uses one filesystem type and one operation profile."
        )
    )
    parser.add_argument(
        "--bench",
        default="build/lib/benchmark/bench_run",
        help="Path to bench_run binary.",
    )
    parser.add_argument(
        "--fs",
        nargs="+",
        default=["A", "B", "C"],
        help="Filesystem types to run: A B C.",
    )
    parser.add_argument(
        "--profiles",
        nargs="+",
        default=["build", "file", "db", "web"],
        help=(
            "Profiles to run. Supported aliases: "
            "build/bs, file/fm, backup/bu, refactoring/ref, db, web/ws."
        ),
    )
    parser.add_argument(
        "--repeats",
        type=int,
        required=True,
        help="Repeats argument for bench_run.",
    )
    parser.add_argument(
        "--ops",
        type=int,
        required=True,
        help="Ops argument for bench_run.",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=max(1, (os.cpu_count() or 2) // 2),
        help="Maximum number of concurrent bench_run processes.",
    )
    parser.add_argument(
        "--output-dir",
        default="bench_results",
        help="Directory for per-job CSV files.",
    )
    parser.add_argument(
        "--prefix",
        default="results",
        help="Prefix for generated CSV names.",
    )
    parser.add_argument(
        "--merge-csv",
        help=(
            "Optional path for a merged CSV built from all successful per-job "
            "outputs."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without starting processes.",
    )
    return parser.parse_args()


def normalize_fs(values: list[str]) -> list[str]:
    normalized: list[str] = []
    for value in values:
        fs_type = value.upper()
        if fs_type not in {"A", "B", "C"}:
            raise ValueError(f"Unsupported filesystem type: {value}")
        normalized.append(fs_type)
    return normalized


def normalize_profiles(values: list[str]) -> list[str]:
    normalized: list[str] = []
    for value in values:
        profile = PROFILE_ALIASES.get(value.lower())
        if profile is None:
            raise ValueError(f"Unsupported profile: {value}")
        normalized.append(profile)
    return normalized


def make_jobs(
    fs_types: list[str],
    profiles: list[str],
    output_dir: Path,
    prefix: str,
) -> list[Job]:
    jobs: list[Job] = []
    for fs_type in fs_types:
        for profile in profiles:
            output_csv = output_dir / f"{prefix}_{fs_type}_{profile}.csv"
            jobs.append(Job(fs_type=fs_type, profile=profile, output_csv=output_csv))
    return jobs


def build_command(bench: Path, repeats: int, ops: int, job: Job) -> list[str]:
    return [
        str(bench),
        job.fs_type,
        str(repeats),
        str(ops),
        job.profile,
        str(job.output_csv),
    ]


def run_job(bench: Path, repeats: int, ops: int, job: Job) -> tuple[Job, int, float]:
    command = build_command(bench, repeats, ops, job)
    started_at = time.monotonic()
    completed = subprocess.run(command, check=False)
    elapsed = time.monotonic() - started_at
    return job, completed.returncode, elapsed


def merge_csv_files(inputs: list[Path], output: Path) -> None:
    wrote_header = False

    with output.open("w", encoding="utf-8", newline="") as merged:
        for csv_path in inputs:
            if not csv_path.is_file():
                raise FileNotFoundError(f"missing CSV for merge: {csv_path}")

            with csv_path.open("r", encoding="utf-8", newline="") as current:
                for line_number, line in enumerate(current):
                    if line_number == 0:
                        if not wrote_header:
                            merged.write(line)
                            wrote_header = True
                        continue

                    merged.write(line)


def main() -> int:
    args = parse_args()

    try:
        fs_types = normalize_fs(args.fs)
        profiles = normalize_profiles(args.profiles)
    except ValueError as error:
        print(error, file=sys.stderr)
        return 2

    if args.repeats <= 0:
        print("--repeats must be > 0", file=sys.stderr)
        return 2
    if args.ops <= 0:
        print("--ops must be > 0", file=sys.stderr)
        return 2
    if args.jobs <= 0:
        print("--jobs must be > 0", file=sys.stderr)
        return 2

    bench = Path(args.bench)
    if not bench.is_file():
        print(f"bench binary not found: {bench}", file=sys.stderr)
        return 2

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    jobs = make_jobs(fs_types, profiles, output_dir, args.prefix)

    if args.dry_run:
        for job in jobs:
            print(" ".join(build_command(bench, args.repeats, args.ops, job)))
        return 0

    print(
        f"Starting {len(jobs)} jobs with concurrency={args.jobs}. "
        f"Output dir: {output_dir}"
    )

    failures = 0
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = [
                pool.submit(run_job, bench, args.repeats, args.ops, job)
                for job in jobs
            ]

            for future in concurrent.futures.as_completed(futures):
                job, returncode, elapsed = future.result()
                status = "OK" if returncode == 0 else f"FAIL({returncode})"
                print(
                    f"[{status}] FS={job.fs_type} profile={job.profile} "
                    f"time={elapsed:.1f}s output={job.output_csv}"
                )
                if returncode != 0:
                    failures += 1
    except KeyboardInterrupt:
        print("Interrupted. Some child processes may still be shutting down.", file=sys.stderr)
        return 130

    if failures != 0:
        print(f"Completed with {failures} failed job(s).", file=sys.stderr)
        return 1

    if args.merge_csv:
        merge_target = Path(args.merge_csv)
        merge_target.parent.mkdir(parents=True, exist_ok=True)
        merge_csv_files([job.output_csv for job in jobs], merge_target)
        print(f"Merged CSV written to {merge_target}")

    print("All jobs completed successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
