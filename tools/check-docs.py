#!/usr/bin/env python3
"""Check repository documentation links/anchors and current-page index coverage.

Standard library only. Supports inline/image links (balanced destinations),
explicit/collapsed reference links, ATX/setext headings and HTML id/name anchors.
External URLs are not fetched. Fenced/indented code, inline code and comments are
excluded from link checks. Legacy changes and archives are historical, not
current-index entries. This is a link checker, not a complete Markdown renderer.
"""

import argparse
from collections import deque
import html
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit


ROOT_PAGES = ("README.md", "Todo.md", "CHANGELOG.md", "AGENTS.md")
HISTORICAL = ("docs/archive/", "docs/changes/")


def prose(text):
    """Preserve line numbers while masking block code and HTML comments."""
    text = re.sub(r"<!--.*?-->", lambda m: "\n" * m[0].count("\n"), text, flags=re.S)
    lines = []
    fence = None
    list_indents = []
    for line in text.splitlines():
        line = line.expandtabs(4)
        if fence:
            relative = line[fence[2]:] if line.startswith(" " * fence[2]) else line
            marker = re.match(r"^ {0,3}(`{3,}|~{3,})", relative)
            if marker and marker[1][0] == fence[0] and len(marker[1]) >= fence[1] and not relative[marker.end():].strip():
                fence = None
            lines.append("")
            continue
        if not line.strip():
            lines.append("")
            continue

        # List content is indented prose; only four *additional* columns
        # start an indented code block. Keep containers across blank lines.
        indent = len(line) - len(line.lstrip(" "))
        while list_indents and indent < list_indents[-1]:
            list_indents.pop()
        base = list_indents[-1] if list_indents else 0
        relative = line[base:]
        item = re.match(r"^ {0,3}(?:[-+*]|\d{1,9}[.)])( +)", relative)
        if item:
            # More than four spaces after a marker count as one; the
            # remaining indentation may introduce code within the item.
            padding = len(item[1]) if len(item[1]) <= 4 else 1
            base += item.start(1) + padding
            list_indents.append(base)
            relative = line[base:]
        marker = re.match(r"^ {0,3}(`{3,}|~{3,})", relative)
        if marker:
            fence = (marker[1][0], len(marker[1]), base)
            lines.append("")
        elif relative.startswith("    "):
            lines.append("")
        else:
            lines.append(relative)
    return "\n".join(lines)


def without_inline_code(text):
    return re.sub(r"(`+).*?\1", "", text)


def heading_slug(text):
    text = re.sub(r"!?\[([^]]+)\]\([^)]*\)", r"\1", text)
    text = re.sub(r"<[^>]*>", "", text)
    text = html.unescape(text).lower()
    # GitHub-style anchors for the heading syntax used in this repository.
    text = re.sub(r"[^\w\-\s]", "", text, flags=re.UNICODE)
    return re.sub(r"\s", "-", text.strip())


def anchors(text):
    clean = prose(text)
    result = set(re.findall(r'<[^>]+\b(?:id|name)=[\"\']([^\"\']+)[\"\']', clean))
    counts = {}
    lines = clean.splitlines()
    for i, line in enumerate(lines):
        match = re.match(r"^ {0,3}#{1,6}\s+(.+?)\s*#*\s*$", line)
        heading = match[1] if match else None
        if not match and i + 1 < len(lines) and line.strip() and re.fullmatch(r" {0,3}(?:=+|-+)\s*", lines[i + 1]):
            heading = line.strip()
        if heading is not None:
            slug = heading_slug(heading)
            number = counts.get(slug, 0)
            candidate = slug if not number else f"{slug}-{number}"
            while candidate in result:
                number += 1
                candidate = f"{slug}-{number}"
            result.add(candidate)
            counts[slug] = number + 1
    return result


def destination(value):
    value = value.strip()
    if value.startswith("<"):
        end = value.find(">")
        return value[1:end] if end >= 0 else value
    # An optional quoted title starts after whitespace; spaces in paths must
    # be percent-encoded or enclosed in angle brackets.
    return re.split(r"\s+", value, maxsplit=1)[0]


def links(text):
    clean = without_inline_code(prose(text))
    definitions = {}
    for line in clean.splitlines():
        match = re.match(r"^ {0,3}\[([^]]+)\]:\s*(.+)", line)
        if match:
            definitions[match[1].strip().casefold()] = destination(match[2])
    for line_number, line in enumerate(clean.splitlines(), 1):
        if re.match(r"^ {0,3}\[[^]]+\]:", line):
            continue
        # Labels can contain nested brackets (e.g. an inline image).
        for match in re.finditer(r"(?<!\\)!?\[(?:[^\[\]]|\[[^]]*\])*\]\(", line):
            start = match.end()
            depth = 1
            escaped = False
            quote = None
            angle = False
            for end in range(start, len(line)):
                char = line[end]
                if escaped:
                    escaped = False
                    continue
                if char == "\\":
                    escaped = True
                elif quote:
                    if char == quote:
                        quote = None
                elif angle:
                    if char == ">":
                        angle = False
                elif char == "<" and not line[start:end].strip():
                    angle = True
                elif char in "\"'" and depth == 1 and end > start and line[end - 1].isspace():
                    # Parentheses in optional quoted titles are not part of
                    # the destination's balanced-parenthesis syntax.
                    quote = char
                elif char == "(":
                    depth += 1
                elif char == ")":
                    depth -= 1
                    if depth == 0:
                        yield line_number, re.sub(r"\\([()])", r"\1", destination(line[start:end]))
                        break
        for match in re.finditer(r"(?<!\\)!?\[([^]]+)\]\[([^]]*)\]", line):
            key = (match[2] or match[1]).strip().casefold()
            if key not in definitions:
                yield line_number, f"!undefined-reference:{key}"
            else:
                yield line_number, definitions[key]
        # Defined shortcut references only; task boxes are not references.
        for match in re.finditer(r"(?<![\\\]])\[([^]]+)\](?![\[(])", line):
            key = match[1].strip().casefold()
            if key in definitions:
                yield line_number, definitions[key]


def check(root):
    root = Path(root).resolve()
    pages = set((root / "docs").rglob("*.md"))
    pages.update(root / name for name in ROOT_PAGES if (root / name).exists())
    texts = {p: p.read_text(encoding="utf-8") for p in pages}
    heading_ids = {p: anchors(text) for p, text in texts.items()}
    graph = {p: set() for p in pages}
    errors = []
    for page, text in texts.items():
        for line, raw in links(text):
            prefix = f"{page.relative_to(root)}:{line}"
            if raw.startswith("!undefined-reference:"):
                errors.append(f"{prefix}: {raw[1:]}")
                continue
            try:
                parsed = urlsplit(html.unescape(raw))
            except ValueError:
                errors.append(f"{prefix}: malformed link: {raw}")
                continue
            if parsed.scheme or parsed.netloc:
                continue
            local = unquote(parsed.path)
            target = ((root / local.lstrip("/")) if local.startswith("/") else (page.parent / local)).resolve() if local else page
            if not target.is_relative_to(root):
                errors.append(f"{prefix}: link leaves repository: {raw}")
                continue
            if not target.exists():
                errors.append(f"{prefix}: missing target: {raw}")
                continue
            if target in pages:
                graph[page].add(target)
                fragment = unquote(parsed.fragment)
                if fragment and fragment not in heading_ids[target]:
                    errors.append(f"{prefix}: missing anchor: {raw}")
    index = root / "docs/README.md"
    reached = {index: 0}
    queue = deque([index])
    while queue:
        page = queue.popleft()
        if reached[page] == 2:
            continue
        for target in graph.get(page, ()):
            # Do not let a history/migration table stand in for a current index.
            if target.relative_to(root).as_posix().startswith(HISTORICAL):
                continue
            if target not in reached:
                reached[target] = reached[page] + 1
                queue.append(target)
    for page, text in texts.items():
        relative = page.relative_to(root).as_posix()
        if not relative.startswith("docs/") or relative.startswith(HISTORICAL):
            continue
        if re.search(r"^(?:- )?Type: redirect\s*$", text, re.M):
            continue
        if page not in reached:
            errors.append(f"{relative}: current page not reachable within two links from docs/README.md")
    if not index.exists():
        errors.append("docs/README.md: documentation index is missing")
    return sorted(set(errors)), len(pages)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    errors, count = check(args.root)
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        print(f"Documentation check failed: {len(errors)} errors", file=sys.stderr)
        return 1
    print(f"Documentation check passed ({count} Markdown pages)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
