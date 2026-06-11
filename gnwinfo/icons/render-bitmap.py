#!/usr/bin/env python3
#
# Render one full-color bitmap source SVG to a sibling 64x64 PNG.
#

import argparse
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


TARGET_SIZE = 64
DEFAULT_SOURCE_SIZE = 32
INKSCAPE_NS = "http://www.inkscape.org/namespaces/inkscape"
INKSCAPE_LABEL = f"{{{INKSCAPE_NS}}}label"
INKSCAPE_GROUPMODE = f"{{{INKSCAPE_NS}}}groupmode"


def die(message):
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def local_name(name):
    return name.rsplit("}", 1)[-1]


def numeric_attr(element, name):
    value = element.get(name)
    if value is None:
        return None
    try:
        return float(value)
    except ValueError:
        return None


def find_executable():
    for candidate in ("inkscape", "inkscape.com"):
        path = shutil.which(candidate)
        if path:
            return path

    die("cannot find Inkscape in PATH")


def resolve_svg(value):
    svg_file = Path(value).expanduser().resolve()
    if not svg_file.is_file():
        die(f"cannot find SVG source: {value}")
    if svg_file.suffix.lower() != ".svg":
        die(f"input must be an .svg file: {value}")
    return svg_file


def parse_baseplate(svg_file):
    tree = ET.parse(svg_file)
    root = tree.getroot()

    baseplate = None
    for element in root.iter():
        if local_name(element.tag) != "g":
            continue
        if element.get(INKSCAPE_GROUPMODE) != "layer":
            continue
        label = element.get(INKSCAPE_LABEL, "")
        if label.startswith("Baseplate"):
            baseplate = element
            break

    if baseplate is None:
        die(f"{svg_file} does not contain a Baseplate layer")

    rects = []

    for element in baseplate.iter():
        tag = local_name(element.tag)

        if tag == "rect":
            rect_id = element.get("id")
            width = numeric_attr(element, "width")
            height = numeric_attr(element, "height")
            if rect_id and width and height:
                rects.append(
                    {
                        "id": rect_id,
                        "width": width,
                        "height": height,
                    }
                )

    if not rects:
        die(f"{svg_file} Baseplate does not contain export rectangles")

    return rects


def choose_rect(rects):
    for rect in rects:
        if rect["width"] == DEFAULT_SOURCE_SIZE and rect["height"] == DEFAULT_SOURCE_SIZE:
            return rect
    sizes = ", ".join(format_size(rect) for rect in rects)
    die(f"no {DEFAULT_SOURCE_SIZE}x{DEFAULT_SOURCE_SIZE} Baseplate rect found; available: {sizes}")


def format_size(rect):
    width = int(rect["width"]) if rect["width"].is_integer() else rect["width"]
    height = int(rect["height"]) if rect["height"].is_integer() else rect["height"]
    return f"{width}x{height}:{rect['id']}"


def default_output(svg_file):
    return svg_file.with_suffix(".png")


def run(command):
    try:
        subprocess.run(command, check=True)
    except FileNotFoundError:
        die(f"cannot execute: {command[0]}")
    except subprocess.CalledProcessError as error:
        raise error


def render_with_inkscape(inkscape, svg_file, rect, output_file):
    output_file.parent.mkdir(parents=True, exist_ok=True)

    command = [
        inkscape,
        str(svg_file),
        f"--export-id={rect['id']}",
        f"--export-filename={output_file}",
        f"--export-width={TARGET_SIZE}",
        f"--export-height={TARGET_SIZE}",
        "--export-overwrite",
    ]

    try:
        run(command)
    except subprocess.CalledProcessError:
        dpi = 96 * (TARGET_SIZE / rect["width"])
        fallback = [
            inkscape,
            str(svg_file),
            "--export-dpi",
            str(dpi),
            "-i",
            rect["id"],
            "-e",
            str(output_file),
        ]
        run(fallback)

    optipng = shutil.which("optipng")
    if optipng:
        subprocess.run([optipng, "-quiet", "-o7", str(output_file)], check=False)


def main():
    parser = argparse.ArgumentParser(
        description="Render SVG to PNG"
    )
    parser.add_argument("svg", nargs="?", help="Input SVG path")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Output PNG path. Defaults to the input SVG path with a .png extension",
    )
    args = parser.parse_args()

    inkscape = find_executable()

    if args.svg is None:
        if args.output:
            parser.error("cannot specify -o/--output when batch rendering all SVGs")
        script_dir = Path(__file__).parent.resolve()
        svg_files = sorted(script_dir.glob("*.svg"))
        if not svg_files:
            print("No SVG files found in script directory.")
            return
        for svg_file in svg_files:
            rects = parse_baseplate(svg_file)
            rect = choose_rect(rects)
            output_file = default_output(svg_file)
            render_with_inkscape(inkscape, svg_file, rect, output_file)
            print(f"Rendered {svg_file} [{format_size(rect)}] -> {output_file}")
    else:
        svg_file = resolve_svg(args.svg)
        rects = parse_baseplate(svg_file)
        rect = choose_rect(rects)
        output_file = args.output.resolve() if args.output else default_output(svg_file)

        render_with_inkscape(inkscape, svg_file, rect, output_file)
        print(f"Rendered {svg_file} [{format_size(rect)}] -> {output_file}")


if __name__ == "__main__":
    main()

