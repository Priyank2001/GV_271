#!/usr/bin/env python3
"""
Package the assignment for submission.

Run from anywhere:
    python3 make_submission.py --roll 12345 --name John_Doe

Produces  12345_John_Doe.zip  containing the source, shaders, build script
and report PDF, with no build artefacts.
"""

import argparse
import os
import sys
import zipfile

# (glob-ish relative path, list of directories to look in, required?)
# Shaders may sit in the source dir or in build/ depending on how CMake
# stages them, so each entry carries a search list rather than one path.
WANTED = [
    ("main.cpp",      [".", "..", "../.."], True),
    ("benchmark.h",   [".", "..", "../.."], True),
    ("CMakeLists.txt", [".", "..", "../.."], True),
    ("shader.vs",     [".", "build", "..", "../build"], True),
    ("shader.fs",     [".", "build", "..", "../build"], True),
    ("slice.vs",      [".", "build", "..", "../build"], True),
    ("slice.gs",      [".", "build", "..", "../build"], True),
    ("shader_utils.h", [".", "..", "../.."], False),
    ("shader_utils.cpp", [".", "..", "../.."], False),
    ("report/abc.pdf", [".", "..", "../.."], True),
    # ("report/abc.tex", [".", "..", "../.."], False),
    ("report/benchmark_table.tex", [".", "..", "../.."], False),
]


def find(name, search_dirs):
    for d in search_dirs:
        p = os.path.normpath(os.path.join(d, name))
        if os.path.isfile(p):
            return p
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--roll", required=True, help="your roll number")
    ap.add_argument("--name", required=True,
                    help="MS Teams name, underscores instead of spaces")
    ap.add_argument("--out", default=None, help="output zip path")
    args = ap.parse_args()

    zip_name = args.out or f"{args.roll}_{args.name}.zip"

    found, missing_required = [], []
    for name, dirs, required in WANTED:
        path = find(name, dirs)
        if path:
            found.append((path, os.path.basename(name)))
        elif required:
            missing_required.append(name)
        else:
            print(f"  skipping optional (not found): {name}")

    if missing_required:
        print("\nERROR: required files not found:")
        for m in missing_required:
            print(f"  {m}")
        print("\nRun this from the project root (the directory holding "
              "main.cpp), or from build/.")
        sys.exit(1)

    # Flatten into the archive root, but keep the report in its own folder so
    # the PDF is obvious to whoever opens the zip.
    with zipfile.ZipFile(zip_name, "w", zipfile.ZIP_DEFLATED) as z:
        for src, base in found:
            arc = f"report/{base}" if base.endswith((".pdf", ".tex")) else base
            z.write(src, arc)
            print(f"  added {src:<40} -> {arc}")

    size_kb = os.path.getsize(zip_name) / 1024.0
    print(f"\nWrote {zip_name}  ({size_kb:.1f} KB, {len(found)} files)")

    # Read the archive back: a zip that lists correctly is one the grader can open.
    with zipfile.ZipFile(zip_name) as z:
        bad = z.testzip()
        if bad:
            print(f"WARNING: corrupt entry {bad}")
        else:
            print("Archive verified. Contents:")
            for n in sorted(z.namelist()):
                print(f"  {n}")


if __name__ == "__main__":
    main()