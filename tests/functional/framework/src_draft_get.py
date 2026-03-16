#!/usr/bin/env python3
"""Copy the latest firmware HEX from the build output into the functional test tree.

This script is used by the functional test runner to ensure the firmware under test
is available in the location expected by the test configuration.

It reads the configuration from `tests/functional/config/test_cfg.yaml` and uses:
  - `app_firmware_name` to locate the built firmware in `src/build/`
  - `nightly_firmware_hex` as the destination path (relative to tests/functional/)

Example:
  src/build/can_sensors_demo.hex -> tests/functional/test_binaries/draft/can_sensors_demo.hex
"""

import argparse
import shutil
from pathlib import Path

import yaml


def load_cfg(cfg_path: Path) -> dict:
    if not cfg_path.exists():
        raise FileNotFoundError(f"Config file not found: {cfg_path}")

    with cfg_path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Copy built firmware HEX into the functional test tree as defined by test_cfg.yaml"
    )
    parser.add_argument(
        "--config",
        default="config/test_cfg.yaml",
        help="Path to test configuration YAML file (relative to tests/functional)",
    )
    parser.add_argument(
        "--build-dir",
        default="../../src/build",
        help="Path to the firmware build output directory (relative to tests/functional)",
    )
    parser.add_argument(
        "--target",
        choices=["nightly", "main"],
        default="nightly",
        help="Which firmware target to copy (uses test_cfg keys nightly_firmware_hex or main_firmware_hex)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be copied without actually doing it",
    )

    args = parser.parse_args(argv)

    script_dir = Path(__file__).resolve().parent
    root_dir = script_dir.parent  # tests/functional

    cfg_path = (root_dir / args.config).resolve()
    build_dir = (root_dir / args.build_dir).resolve()

    cfg = load_cfg(cfg_path)

    app_name = cfg.get("app_firmware_name")
    if not app_name:
        raise KeyError("Missing required config key: 'app_firmware_name'")

    if args.target == "nightly":
        dest_hex = cfg.get("nightly_firmware_hex")
        if not dest_hex:
            raise KeyError("Missing required config key: 'nightly_firmware_hex'")
    else:
        dest_hex = cfg.get("main_firmware_hex")
        if not dest_hex:
            raise KeyError("Missing required config key: 'main_firmware_hex'")

    src_hex = build_dir / app_name
    dest_path = (root_dir / dest_hex).resolve()

    print(f"Source HEX: {src_hex}")
    print(f"Destination: {dest_path}")

    if not src_hex.exists():
        raise FileNotFoundError(f"Built firmware not found: {src_hex}")

    dest_path.parent.mkdir(parents=True, exist_ok=True)

    if args.dry_run:
        print("Dry run: not copying")
        return 0

    shutil.copy2(src_hex, dest_path)
    print("Copy complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
