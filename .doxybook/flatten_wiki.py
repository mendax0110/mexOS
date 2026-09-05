"""
Flatten a wiki directory by converting all markdown files to a single directory structure.
Usage: python flatten_wiki.py <source_directory> <destination_directory>
"""
import os
import re
import shutil
import sys

SRC = sys.argv[1] if len(sys.argv) > 1 else "wiki-content"
DST = sys.argv[2] if len(sys.argv) > 2 else "wiki-flat"

os.makedirs(DST, exist_ok=True)

LINK_RE = re.compile(r'(\]\()([^)#\s]+)(#[^)]*)?(\))')

def flat_name(rel_path):
    """
    Returns the flat name from a given relative path
    :param rel_path: The relative path of the file
    :return: The flat name
    """
    rel_path = rel_path.replace("\\", "/")
    name = "-".join(rel_path.split("/"))
    name = name.removesuffix(".md")
    return name

mapping = {}
md_files = []
for root, dirs, files in os.walk(SRC):
    for f in files:
        if f.endswith(".md"):
            full = os.path.join(root, f)
            rel = os.path.relpath(full, SRC).replace("\\", "/")
            md_files.append(rel)
            mapping[rel] = flat_name(rel)
            mapping[rel[:-3]] = flat_name(rel)  # without .md, root-relative

def repl(m):
    """
    Replacement function for re.sub to rewrite links in markdown files.
    :param m: The match object from the regex search
    :return: The rewritten link if applicable, otherwise the original match
    """
    prefix, target, anchor, suffix = m.groups()
    anchor = anchor or ""
    if target.startswith(("http://", "https://", "#")):
        return m.group(0)
    normalized = os.path.normpath(target).replace("\\", "/")
    if normalized in mapping:
        return f"{prefix}{mapping[normalized]}{anchor}{suffix}"
    if target.lower().endswith((".png", ".jpg", ".jpeg", ".gif", ".svg")):
        return f"{prefix}{normalized}{anchor}{suffix}"
    return m.group(0)

for rel in md_files:
    full = os.path.join(SRC, rel)
    with open(full, "r", encoding="utf-8", errors="ignore") as fh:
        content = fh.read()
    new_content = LINK_RE.sub(repl, content)
    new_name = mapping[rel] + ".md"
    with open(os.path.join(DST, new_name), "w", encoding="utf-8") as fh:
        fh.write(new_content)

for item in ["images", "UML"]:
    src_path = os.path.join(SRC, item)
    if os.path.isdir(src_path):
        shutil.copytree(src_path, os.path.join(DST, item), dirs_exist_ok=True)

print(f"Flattened {len(md_files)} markdown files into {DST}/")