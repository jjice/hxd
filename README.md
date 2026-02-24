# 📂 hxd - A modern cross-platform hexdumper

hxd is a small command-line tool written in C that prints binary data in a classic hex dump layout with optional coloring, heatmaps, and ASCII preview. No extern dependencies!

[![Arch Linux](https://img.shields.io/github/actions/workflow/status/jjice/hxd/ci.yml?label=Arch%20Linux&logo=arch-linux&logoColor=white)](https://github.com/jjice/hxd/actions)
[![Ubuntu](https://img.shields.io/github/actions/workflow/status/jjice/hxd/ci.yml?label=Ubuntu&logo=ubuntu&logoColor=white)](https://github.com/jjice/hxd/actions)
[![Windows](https://img.shields.io/github/actions/workflow/status/jjice/hxd/ci.yml?label=Windows&logo=windows&logoColor=white)](https://github.com/jjice/hxd/actions)
[![macOS](https://img.shields.io/github/actions/workflow/status/jjice/hxd/ci.yml?label=macOS&logo=apple&logoColor=white)](https://github.com/jjice/hxd/actions)

# ✨ Key Features

 * 📟 **Classic Layout:** Perfectly aligned hex bytes, addresses, and ASCII preview. -->
 * 🌈 **Adaptive Heatmaps:** Visualize data intensity with 16-color modes (`adaptive` or `fixed`).
 * 🔍 **Semantic Coloring:** Instantly distinguish between printable text, null bytes, and control characters.
 * 🧵 **String Highlighting:** Specialized mode to make embedded strings pop.
 * 📊 **Entropy Meter:** Real-time Shannon entropy bar per line (spot encryption/compression instantly! 🧪).
 * ⚡ **Ultra Flexible:** Custom widths, offsets, and limits for deep surgical inspections.
 * 🌊 **Pipe Ready:** Works flawlessly with `stdin` and includes built-in pager support (`less`/`more`).
 * 📦 **Cross-Platform:** Native performance on Linux, macOS, and Windows.

<br>

# 🏗️ Build / Install

### Requirements: a C compiler (clang or gcc) and CMake.

```
make
```

or 

```bash
cmake -S . -B build
cmake --build build
```

### If you want to install (optional):

```bash
cmake --install build
```

>

Hint: 'Windows' You can add a PATH inside Environmental-variables to */build/bin to run in terminal 

### Manual remove:

```bash
(sudo) xargs rm < build/install_manifest.txt                <- Linux / MacOS
```
```Powershell
get-content build\install_manifest.txt | remove-item        <- Windows
```

<br>

# 🕹️ Usage

```bash
hxd [options] [filename]
cat file.bin | hxd [options]
```

<br>

# ✨ Options

| Option | Description | Default |
| --- | --- | --- |
| -f, --file <filename> | Input file (optional if stdin is used) | - |
| -hm, --heatmap <adaptiv\|fixed\|none> | Heatmap mode with 16 colors | none |
| -w, --width <num> | Bytes per line | 16 |
| -a, --ascii <on\|off> | Show ASCII column | on |
| -o, --offset <num> | Start reading at this byte offset | 0 |
| -l, --limit <num> | Stop after this many bytes | EOF |
| -c, --color <on\|off> | Enable colored output | on |
| -s, --string <on\|off> | Highlight strings | off |
| -p, --pager <on\|off> | Pipe output through pager (more/less) | off |
| -e, --entropy <on\|off> | Show entropy bar per line | off |
| -h, --help | Show help and exit | - |

Notes:

- Offsets and limits must be positive integers.
- Limit must be greater than or equal to offset.
- When reading from stdin, the filename is optional.

<br>

# 🔎 Examples

Basic hex dump:

```bash
hxd example.txt
```

Smaller width:

```bash
hxd -w 8 example.txt
```

Adaptive heatmap on a binary file:

```bash
hxd -hm adaptiv image.png
```

Fixed heatmap and no ASCII column:

```bash
hxd -hm fixed -a off sample.bin
```

Read a slice of a file:

```bash
hxd -o 1024 -l 2048 sample.bin
```

Use stdin:

```bash
cat sample.bin | hxd -w 32
```

## Output example

```
           00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
-----------------------------------------------------------+----------------
00000000 | 48 65 6c 6c 6f 20 57 6f 72 6c 64 21 0a          | Hello World!.
```

<br>

# 🤝 Contributing & Ideas
- hxd is an open-source journey! 🗺️ I’m always looking for cool ideas to make binary analysis even better.

- **Got a Bug? 🐛 Open an Issue.**

- **Have a Feature Idea?** 💡 Let's discuss it in the Discussions tab!

- **Wanna Code? 💻** PRs are always welcome! Whether it's performance optimization or a new color theme, feel free to jump in.

<br>

### ***"In binary we trust, in hex we dump." — hxd Team***
