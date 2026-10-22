"""
Flatten a wiki directory by converting all markdown files to a single directory structure.
Usage: python flatten_wiki.py <source_directory> <destination_directory>
"""
from __future__ import annotations

import argparse
import posixpath
import re
import shutil
from collections.abc import Callable
from pathlib import Path
from urllib.parse import quote, unquote, urlsplit

LINK_RE = re.compile(r'(\]\()([^)#\s]+)(#[^)\s]*)?(\s+"[^"]*")?(\))')
FENCE_RE = re.compile(r'(```.*?```)', re.DOTALL)
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
    return "-".join(rel_path.with_suffix("").parts)

def collect_markdown_files(src: Path) -> list[Path]:
    """
    Collects the  markdown files from a given path
    :param src: The path to collect the files from
    :return: A list with the relative paths of the markdown files
    """
    files = (p.relative_to(src) for p in src.rglob("*md") if p.is_file())
    return sorted(p for p in files if not any(part.startswith(".") for part in p.parts))

def collect_assets(src: Path) -> set[str]:
    """
    Collects all files inside the extra directories so links to them can be validated
    :param src: The source directory
    :return: A set of root-relative posix paths
    """
    return {
        p.relative_to(src).as_posix()
        for name in COPY_ALSO
        if (src / name).is_dir()
        for p in (src / name).rglob("*")
        if p.is_file()
    }

def build_mapping(md_files: list[Path]) -> dict[str, str]:
    """
    Builds a mapping from relative paths to flat names for the given markdown files
    :param md_files: The list of markdown files to build the mapping for
    :return: A dictionary mapping relative paths to flat names
    """
    mapping: dict[str, str] = {}
    owners: dict[str, str] = {}
    for relative in md_files:
        flat = flat_name(relative)
        key = flat.casefold()
        if key in owners:
            raise SystemExit(
                f"Name collision: '{owners[key]}' and '{relative.as_posix()}' both flatten to '{flat}.md'"
            )
        owners[key] = relative.as_posix()
        mapping[relative.as_posix()] = flat
        mapping[relative.with_suffix("").as_posix()] = flat
    return mapping

def resolve_target(target: str, current_dir: str) -> str:
    """
    Resolve a link target to a root-relative posix path
    :param target: The raw link target
    :param current_dir: The directory of the file containing the link, relative to the root
    :return: The normalized root-relative path
    """
    target = unquote(target)
    if target.startswith("/"):
        return posixpath.normpath(target.lstrip("/"))
    return posixpath.normpath(posixpath.join(current_dir, target))

def make_link_rewriter(
        mapping: dict[str, str], assets: set[str], source_file: Path, broken: list[str]
) -> Callable[[re.Match[str]], str]:
    """
    Builds a re.sub replacement function bound to the given path mapping
    :param mapping: The mapping of relative paths to flat names
    :param assets: The set of known asset paths to flat names
    :param source_file: The root-relative path of the file being processed
    :param broken: Collects descriptions of links that could not be resolved
    :return: A function that can be used with re.sub to rewrite links
    """
    current_dir = source_file.parent.as_posix()
    def repl(m: re.Match[str]) -> str:
        """
        Rewrite a single link match using the given mapping
        :param m: The match object
        :return: The rewritten link
        """
        prefix, target, anchor, title, suffix = m.groups()

        if urlsplit(target).scheme:
            return m.group(0)

        resolved = resolve_target(target, current_dir)
        if resolved in mapping:
            new_target = mapping[resolved]
        elif resolved in assets:
            new_target = resolved
        else:
            broken.append(f"{source_file.as_posix()}: {target}")
            return m.group(0)

        return f"{prefix}{quote(new_target)}{anchor or ''}{title or ''}{suffix}"

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
    content = (src / rel).read_text(encoding="utf-8")
    parts = FENCE_RE.split(content)
    new_content = "".join(
        part if i % 2 else LINK_RE.sub(rewrite_links, part)
        for i, part in enumerate(parts)
    )
    (dst / f"{flat_stem}.md").write_text(new_content, encoding="utf-8")

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
    """
    Main function to flatten the wiki directory
    :return: None
    """
    args = parse_args()
    src: Path = args.source
    dst: Path = args.destination

    if not src.is_dir():
        raise SystemExit(f"Source directory not found: {src}")
    if dst.resolve().is_relative_to(src.resolve()):
        raise SystemExit("Destination must not be inside the source directory")

    dst.mkdir(partents=True, exist_ok=True)

    md_files = collect_markdown_files(src)
    mapping = build_mapping(md_files)
    assets = collect_assets(src)
    broken: list[str] = []

    for relative in md_files:
        rewrite_links = make_link_rewriter(mapping, assets, relative, broken)
        flatten_file(src, relative, dst, mapping[relative.as_posix()], rewrite_links)

    copy_extra_dirs(src, dst)

    print(f"Flattened {len(md_files)} markdown files into {dst}/")
    if broken:
        print(f"\n{len(broken)} unresolved link(s):")
        for entry in broken:
            print(f"    {entry}")

if __name__ == "__main__":
    main()