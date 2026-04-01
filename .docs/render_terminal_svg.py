#!/usr/bin/env python3
import argparse
import html
import math
import os
import re


ESC_RE = re.compile(r"\x1b\[([0-9;]*)m")


def xterm_to_hex(index: int) -> str:
    base16 = [
        "#000000", "#800000", "#008000", "#808000",
        "#000080", "#800080", "#008080", "#c0c0c0",
        "#808080", "#ff0000", "#00ff00", "#ffff00",
        "#0000ff", "#ff00ff", "#00ffff", "#ffffff",
    ]
    if 0 <= index < 16:
        return base16[index]
    if 16 <= index <= 231:
        index -= 16
        r = index // 36
        g = (index % 36) // 6
        b = index % 6
        scale = [0, 95, 135, 175, 215, 255]
        return "#{:02x}{:02x}{:02x}".format(scale[r], scale[g], scale[b])
    if 232 <= index <= 255:
        level = 8 + (index - 232) * 10
        return "#{:02x}{:02x}{:02x}".format(level, level, level)
    return "#d8dee9"


def ansi_basic_to_hex(code: int) -> str | None:
    if 30 <= code <= 37:
        return xterm_to_hex(code - 30)
    if 90 <= code <= 97:
        return xterm_to_hex(code - 90 + 8)
    if 40 <= code <= 47:
        return xterm_to_hex(code - 40)
    if 100 <= code <= 107:
        return xterm_to_hex(code - 100 + 8)
    return None


def parse_ansi_line(line: str, default_color: str):
    segments = []
    color = default_color
    background = None
    pos = 0

    for match in ESC_RE.finditer(line):
        if match.start() > pos:
            text = line[pos:match.start()]
            if text:
                segments.append((text, color, background))

        codes = [c for c in match.group(1).split(";") if c != ""]
        if not codes:
            codes = ["0"]

        i = 0
        while i < len(codes):
            code = int(codes[i])
            if code == 0:
                color = default_color
                background = None
            elif code == 39:
                color = default_color
            elif code == 49:
                background = None
            elif code == 38 and i + 2 < len(codes) and codes[i + 1] == "5":
                color = xterm_to_hex(int(codes[i + 2]))
                i += 2
            elif code == 48 and i + 2 < len(codes) and codes[i + 1] == "5":
                background = xterm_to_hex(int(codes[i + 2]))
                i += 2
            elif (mapped := ansi_basic_to_hex(code)) is not None:
                if 30 <= code <= 37 or 90 <= code <= 97:
                    color = mapped
                else:
                    background = mapped
            i += 1

        pos = match.end()

    if pos < len(line):
        text = line[pos:]
        if text:
            segments.append((text, color, background))

    return segments


def sanitize_lines(text: str):
    lines = []
    for raw in text.replace("\r", "").splitlines():
        line = raw.rstrip("\n")
        if line.strip("\x1b[0m") == "":
            continue
        lines.append(line)
    return lines


def visible_length(line: str) -> int:
    return len(ESC_RE.sub("", line))


def render_svg(lines, output_path, title=""):
    bg = "#0f1117"
    fg = "#d8dee9"
    border = "#2a2f3a"
    accent = "#7aa2b8"
    font_size = 14
    char_width = 8.4
    line_height = 20
    pad_x = 18
    pad_top = 18
    pad_bottom = 18
    title_gap = 10 if title else 0
    title_height = 22 if title else 0

    max_chars = max((visible_length(line) for line in lines), default=1)
    width = int(math.ceil((max_chars * char_width) + pad_x * 2))
    height = int(math.ceil((len(lines) * line_height) + pad_top + pad_bottom + title_height + title_gap))

    body = []
    if title:
        body.append(
            f'<text x="{pad_x}" y="{pad_top}" fill="{accent}" '
            f'font-size="13" font-family="DejaVu Sans Mono, Consolas, monospace">{html.escape(title)}</text>'
        )

    base_y = pad_top + title_height + title_gap
    for row, line in enumerate(lines):
        y = base_y + row * line_height
        x = pad_x
        for text, color, background in parse_ansi_line(line, fg):
            if not text:
                continue
            esc_text = html.escape(text)
            text_width = len(text) * char_width
            if background is not None:
                body.append(
                    f'<rect x="{x:.1f}" y="{y - font_size + 2}" width="{text_width:.1f}" height="{line_height - 4}" '
                    f'rx="3" fill="{background}"/>'
                )
            body.append(
                f'<text x="{x:.1f}" y="{y}" fill="{color}" '
                f'font-size="{font_size}" font-family="DejaVu Sans Mono, Consolas, monospace" '
                f'xml:space="preserve">{esc_text}</text>'
            )
            x += text_width

    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" role="img" aria-label="{html.escape(title or "terminal output")}">
  <rect x="0.5" y="0.5" width="{width - 1}" height="{height - 1}" rx="10" fill="{bg}" stroke="{border}"/>
  {"".join(body)}
</svg>
'''

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(svg)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file")
    parser.add_argument("output_file")
    parser.add_argument("--title", default="")
    args = parser.parse_args()

    with open(args.input_file, "r", encoding="utf-8", errors="replace") as f:
        content = f.read()

    lines = sanitize_lines(content)
    render_svg(lines, args.output_file, args.title)


if __name__ == "__main__":
    main()
