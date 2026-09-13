#!/usr/bin/env python3
"""Check that the JS Runner app-local API table stays in sync with api_symbols.csv.

The MJS runtime is linked into js_app.fap (and cli_js.fal) instead of the
firmware. JS .fal modules resolve their mjs_* imports from the app-local table
in applications/system/js_app/plugin_api/app_api_table_i.h, while the firmware
API table in targets/f7/api_symbols.csv keeps the same functions as inactive
("-") rows so the SDK headers stay published. This script fails when the two
lists drift apart: a function present on one side but not the other, or the
same function with a different signature.

Usage: python3 scripts/check_js_api_table.py [--target f7]
"""

import argparse
import csv
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TABLE = ROOT / "applications/system/js_app/plugin_api/app_api_table_i.h"

# mjs_* functions declared in the public MJS headers that the JS Runner does
# not export to modules. Debug and file helpers, compiled out or unused.
NOT_EXPORTED = {
    "mjs_disasm_all",
    "mjs_dump",
    "mjs_fprintf",
    "mjs_get_bcode_filename_by_offset",
    "mjs_print_error",
    "mjs_set_generate_jsc",
}

API_METHOD_RE = re.compile(
    r"API_METHOD\(\s*(?P<name>mjs_\w+)\s*,\s*(?P<ret>[^,]+?)\s*,\s*\((?P<args>.*?)\)\s*\)",
    re.DOTALL,
)


def normalize(sig: str) -> str:
    sig = sig.replace("struct ", "")
    sig = re.sub(r"\s*\*\s*", "*", sig)
    sig = re.sub(r"\s*,\s*", ",", sig)
    sig = re.sub(r"\s+", " ", sig)
    return sig.strip()


def load_table():
    text = TABLE.read_text()
    entries = {}
    for m in API_METHOD_RE.finditer(text):
        entries[m["name"]] = (normalize(m["ret"]), normalize(m["args"]))
    return entries


def load_csv(target: str):
    path = ROOT / f"targets/{target}/api_symbols.csv"
    entries = {}
    with path.open(newline="") as f:
        for row in csv.DictReader(f):
            if row["entry"] != "Function" or not row["name"].startswith("mjs_"):
                continue
            entries[row["name"]] = (
                row["status"],
                normalize(row["type"]),
                normalize(row["params"]),
            )
    return entries


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--target", default="f7")
    args = parser.parse_args()

    table = load_table()
    api = load_csv(args.target)
    errors = []

    for name, (status, ret, params) in sorted(api.items()):
        if status == "+":
            errors.append(
                f"{name}: still an active firmware export; MJS must not be in firmware"
            )
        if name in NOT_EXPORTED:
            if name in table:
                errors.append(
                    f"{name}: listed in NOT_EXPORTED but present in the app-local table"
                )
            continue
        if name not in table:
            errors.append(
                f"{name}: in api_symbols.csv but missing from the app-local table"
            )
            continue
        t_ret, t_args = table[name]
        if (t_ret, t_args) != (ret, params):
            errors.append(
                f"{name}: signature differs\n"
                f"    csv:   {ret} ({params})\n"
                f"    table: {t_ret} ({t_args})"
            )

    for name in sorted(set(table) - set(api)):
        errors.append(f"{name}: in the app-local table but not in api_symbols.csv")

    if errors:
        print(
            f"JS app-local API table is out of sync with targets/{args.target}/api_symbols.csv:"
        )
        for e in errors:
            print(f"  {e}")
        return 1

    exported = len(table)
    print(
        f"JS app-local API table OK: {exported} mjs_* entries match api_symbols.csv "
        f"({len(NOT_EXPORTED)} intentionally not exported)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
