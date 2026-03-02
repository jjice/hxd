# 📂 hxed — A Modern Cross-Platform Hex Viewer

> A lightweight, zero-dependency command-line hex viewer written in C — with coloring, heatmaps, entropy analysis, search, and more.

[![Arch Linux](https://img.shields.io/github/actions/workflow/status/jjice/hxed/ci.yml?label=Arch%20Linux&logo=arch-linux&logoColor=white&style=flat-square)](https://github.com/jjice/hxed/actions)
[![Ubuntu](https://img.shields.io/github/actions/workflow/status/jjice/hxed/ci.yml?label=Ubuntu&logo=ubuntu&logoColor=white&style=flat-square)](https://github.com/jjice/hxed/actions)
[![Windows](https://img.shields.io/github/actions/workflow/status/jjice/hxed/ci.yml?label=Windows&logo=windows&logoColor=white&style=flat-square)](https://github.com/jjice/hxed/actions)
[![macOS](https://img.shields.io/github/actions/workflow/status/jjice/hxed/ci.yml?label=macOS&logo=apple&logoColor=white&style=flat-square)](https://github.com/jjice/hxed/actions)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue?style=flat-square)](LICENSE)

---

## ✨ Key Features

| | Feature | Description |
|---|---|---|
| 📟 | **Classic Layout** | Perfectly aligned hex bytes, addresses, and ASCII preview |
| 📦 | **Raw Output Mode** | Raw output to a file or to a pipe |
| 🌈 | **Adaptive Heatmaps** | Visualize data intensity with 16-color `adaptive` or `fixed` modes |
| 🔍 | **Semantic Coloring** | Instantly distinguish printable text, null bytes, and control characters |
| 🧵 | **String Highlighting** | Specialized mode to make embedded strings pop |
| 📊 | **Entropy Meter** | Real-time Shannon entropy bar per line — spot encryption/compression instantly |
| 🔎 | **Pattern Search** | Find ASCII (`-sa`) or hex (`-sh`) patterns across the entire dump |
| 🏷️ | **File Header** | Toggle file info and magic byte detection with `-th` |
| ⚡ | **Ultra Flexible** | Custom widths, offsets, and limits for surgical binary inspection |
| 🌊 | **Pipe Ready** | Seamless `stdin` support with built-in pager integration (`less`/`more`) |
| 📦 | **Cross-Platform** | Native performance on Linux, macOS, and Windows |

---

## 📸 Screenshots

### Header + magic byte detection &nbsp;·&nbsp; `hxed -th linux_elf`

<picture>
  <source media="(prefers-color-scheme: dark)" srcset=".docs/magic.svg">
  <img alt="Header output" src=".docs/magic.svg" style="background:transparent">
</picture>

### Normal output &nbsp;·&nbsp; `hxed <file>`

<picture>
  <source media="(prefers-color-scheme: dark)" srcset=".docs/normal.svg">
  <img alt="Normal output" src=".docs/normal.svg" style="background:transparent">
</picture>

### Heatmap &nbsp;·&nbsp; `hxed <file> -hm fixed`

<picture>
  <source media="(prefers-color-scheme: dark)" srcset=".docs/heat.svg">
  <img alt="Heatmap output" src=".docs/heat.svg" style="background:transparent">
</picture>

### ASCII search &nbsp;·&nbsp; `hxed -sa 'ABC' <file>`

<picture>
  <source media="(prefers-color-scheme: dark)" srcset=".docs/search.svg">
  <img alt="Search output" src=".docs/search.svg" style="background:transparent">
</picture>

---

## 🏗️ Build & Install

**Requirements:** a C compiler (`gcc` or `clang`) and `CMake`.

```bash
# Quick build via Makefile
make

# Or manually with CMake
cmake -S . -B build
cmake --build build
```

### Optional: install system-wide

```bash
cmake --install build
```

> **Windows tip:** Add `*/build/bin` to your `PATH` environment variable to use `hxed` from any terminal.

### Uninstall

```bash
# Linux / macOS
sudo xargs rm < build/install_manifest.txt

# Windows (PowerShell)
Get-Content build\install_manifest.txt | Remove-Item
```

---

## 🕹️ Usage

```bash
hxed [options] [filename]
cat file.bin | hxed [options]
```

---

## ⚙️ Options

| Option | Description | Default |
|--------|-------------|---------|
| `-f, --file <filename>` | Input file (optional when using stdin) | — |
| `-hm, --heatmap <adaptiv\|fixed>` | Heatmap mode with 16 colors | none |
| `-w, --width <num>` | Bytes per line (`0` = no newline) | `16` |
| `-a, --ascii` | Show ASCII column | on |
| `-na, --no-ascii` | Hide ASCII column | — |
| `-th, --toggle-header` | Show file info and magic byte detection | off |
| `-o, --offset <num>` | Start reading at this byte offset | `0` |
| `-r, --read-size <num>` | Stop reading after this many bytes | `0` |
| `-l, --limit <num\|hex>` | Stop after this bytes adress | EOF |
| `-c, --color` | Enable syntax coloring | on |
| `-nc, --no-color` | Disable syntax coloring | — |
| `-s, --string` | Highlight embedded strings | off |
| `-ns, --no-string` | Disable string highlighting | — |
| `-p, --pager` | Pipe output through `less`/`more` | off |
| `-np, --no-pager` | Disable pager | — |
| `-e, --entropy` | Show Shannon entropy bar per line | off |
| `-ne, --no-entropy` | Disable entropy bar | — |
| `-sa, --search-ascii '<str>'` | Show only lines matching ASCII pattern | — |
| `-sh, --search-hex '<hex>'` | Show only lines matching hex pattern | — |
| `-ro, --raw` | Raw output (no ANSI, for piping to files) | — |
| `-v, --version` | Show version and exit | — |
| `-h, --help` | Show help and exit | — |

**Notes:**
- Boolean flags (`-a`, `-c`, `-s`, `-p`, `-e`) toggle their feature.
- `--limit` must not be less than `--offset`.
- Magic byte detection is disabled when `--offset` is set.
- When reading from stdin, a filename is not required.

---

## 🔎 Examples

```bash
# Basic hex dump
hxed example.txt

# Smaller width
hxed -w 8 example.txt

# 32 bytes/line, hide ASCII column
hxed -w 32 -na secret.bin

# Adaptive heatmap on a binary
hxed -hm adaptiv image.png

# Fixed heatmap, no ASCII
hxed -hm fixed -na sample.bin

# Inspect a slice of a file (bytes 1024–2048)
hxed -o 1k -l 2k sample.bin

# String highlighting
hxed -s image.png

# Search for an ASCII pattern
hxed -sa 'ABC' sample.bin

# Search for a hex pattern
hxed -sh 'FF D8 FF' photo.jpg

# Raw output into a file (no newlines, no ANSI)
hxed -w 0 -ro binary > output.txt

# Pipe from stdin
cat sample.bin | hxed -w 32
echo 'Hello World' | hxed

# Output without colors, piped to less
hxed -nc file | less -R
```

---

## 🤝 Contributing
hxed is open-source and contributions are always welcome!

- **Found a bug? 🐛** — Open an [Issue](https://github.com/jjice/hxed/issues)
- **Have an idea? 💡** — Start a thread in [Discussions](https://github.com/jjice/hxed/discussions)
- **Want to contribute code? 💻** — PRs are welcome! Whether it's a performance tweak, new color theme, or a brand new feature — jump in.

---

## 📄 License

MIT License © 2026 Joshua Jallow — see [LICENSE](LICENSE) for details.

---

<div align="center">
  <sub><i>"In binary we trust, in hex we dump." — hxed Team</i></sub>
</div>
