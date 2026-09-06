"""
Flatten a wiki directory by converting all markdown files to a single directory structure.
Usage: python flatten_wiki.py <source_directory> <destination_directory>
"""
from __future__ import annotations

import argparse
import re
import shutil
from collections.abc import Callable
from pathlib import Path

LINK_RE = re.compile(r'(\]\()([^)#\s]+)(#[^)]*)?(\))')
IMAGE_SUFFIXES = (".png", ".jpg", ".jpeg", ".gif", ".svg")
COPY_ALSO = ("images", "UML")

def parse_args() -> argparse.Namespace:
    """
    Helper to parse the given arguments for the script
    :return: The parsed arguments
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", nargs="?", default="wiki-content", type=Path)
    parser.add_argument("destination", nargs="?", default="wiki-flat", type=Path)
    return parser.parse_args()

def flat_name(rel_path: Path) -> str:
    """
    Collapse a relative path into a single flat filename stem
    :param rel_path: The relative path to flatten
    :return: The flattened name
    """
    name = "-".join(rel_path.as_posix().split("/"))
    return name.removesuffix(".md")

def collect_markdown_files(src: Path) -> list[Path]:
    """
    Collects the  markdown files from a given path
    :param src: The path to collect the files from
    :return: A list with the relative paths of the markdown files
    """
    return [p.relative_to(src) for p in src.rglob("*.md") if p.is_file()]

def build_mapping(md_files: list[Path]) -> dict[str, str]:
    """
    Builds a mapping from relative paths to flat names for the given markdown files
    :param md_files: The list of markdown files to build the mapping for
    :return: A dictionary mapping relative paths to flat names
    """
    mapping: dict[str, str] = {}
    for relative in md_files:
        flat = flat_name(relative)
        mapping[relative.as_posix()] = flat
        mapping[relative.with_suffix("").as_posix()] = flat
    return mapping

def make_link_rewriter(mapping: dict[str, str]) -> Callable[[re.Match[str]], str]:
    """
    Builds a re.sub replacement function bound to the given path mapping
    :param mapping: The mapping of relative paths to flat names
    :return: A function that can be used with re.sub to rewrite links
    """
    def repl(m: re.Match[str]) -> str:
        prefix, target, anchor, suffix = m.groups()
        anchor = anchor or ""

        if target.startswith(("http://", "https://", "#")):
            return m.group(0)

        normalized = Path(target).as_posix()
        if normalized in mapping:
            return f"{prefix}{mapping[normalized]}{anchor}{suffix}"
        if target.lower().endswith(IMAGE_SUFFIXES):
            return f"{prefix}{target}{anchor}{suffix}"
        return m.group(0)

    return repl

def flatten_file(src: Path, rel: Path, dst: Path, flat_stem: str, rewrite_links: Callable[[re.Match[str]], str]) -> None:
    """
    Flatten a single markdown file by rewriting its links and saving it to the destination directory
    :param src: The source directory
    :param rel: The relative path to the markdown file
    :param dst: The destination directory
    :param flat_stem: The flat name for the markdown file
    :param rewrite_links: A function to rewrite the links in the markdown file
    """
    content = (src / rel).read_text(encoding="utf-8", errors="ignore")
    new_content = LINK_RE.sub(rewrite_links, content)
    (dst / f"{flat_stem}.md").write_text(new_content, encoding="utf-8", errors="ignore")

def copy_extra_dirs(src: Path, dst: Path) -> None:
    """
    Copy extra directories (like images) from the source to the destination
    :param src: The source directory
    :param dst: The destination directory
    :return: None
    """
    for name in COPY_ALSO:
        src_dir = src / name
        if src_dir.is_dir():
            shutil.copytree(src_dir, dst / name, dirs_exist_ok=True)

def main() -> None:
    args = parse_args()
    src: Path = args.source
    dst: Path = args.destination
    dst.mkdir(parents=True, exist_ok=True)

    md_files = collect_markdown_files(src)
    mapping = build_mapping(md_files)
    rewrite_links = make_link_rewriter(mapping)

    for relative in md_files:
        flatten_file(src, relative, dst, mapping[relative.as_posix()], rewrite_links)

    copy_extra_dirs(src, dst)

    print(f"Flattened {len(md_files)} markdown files into {dst}/")

if __name__ == "__main__":
    main()