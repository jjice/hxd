# 🛠 Project Roadmap & TODOs

This document tracks the progress of **hxed**. Key focus areas include performance parity with `xxd`, robust testing, and advanced binary analysis features.

---

## 🚀 GitHub & Presence
  - [ ] **Benchmark Section:** Performance comparison against `xxd` and `hexdump` (using `hyperfine` or `time`).

## 🛠 Features & Core Logic


### 🛠 Functionality
- [ ] **Live-Preview (`--follow`)**: Monitor file changes in real-time.
				
	inotify e.q. and byte highlighting of changed bytes -> 
	buffer before compare with latest buffer -> only output changed lines.

- [ ] **Comparison Mode**: Side-by-side diffing of two binary files.

- [ ] **Default settings**: Create a setup file to save flag presets

- [ ] **Reverse Mode**: Reverse hex to bin

- [ ] **C-Style Array**: Export output as c-style array

- [ ] **Analysis Toollit**: Top 10 distribution -> or even moore with flag, 

    --> 0x00 Blocks strings count utf 8 blocks entropie blocks "high" compressed blocks decrypted blocks

---

## 📦 Distribution & Releases
- [ ] **Package Manager Integration**
  - [ ] Submit to AUR (Arch User Repository).
  - [ ] Add a binary .deb
  - [ ] Winget manifest for Windows.

---

## 🧪 Testing & Quality Assurance


---

## ✅ Completed
- [x] Overhaul CLI argument parsing.
- [x] Redesign help menu for better readability.
- [x] Multi-platform CI/CD (Windows, Linux, macOS).
- [x] Entropy Visualization
- [x] Heatmap (fixed | adaptiv)
- [x] **Magic Bytes Recognition**
- [x] Implement `MagicTable` lookup logic.
- [x] Output detected file format in the header.
- [x] **Search function**
- [x] **String Highlighting**
- [x] CLI arguments and options added.
- [x] Implement detection logic 
- [x] More advanced detection
- [x] **Automated Testing**
- [x] Implement **Unit Tests** for core utility functions.
- [x] **Memory & Security**
- [x] Run **Valgrind** to ensure zero memory leaks.
- [x] Implement **Fuzzing** (using `AFL++` or `libFuzzer`) to harden the parser against corrupt binaries.
- [x] **RAW Output-Mode**: Output in raw data 
- [x] **Rewrite limit**: Right now its not a limit, more a read_size
- [x] **Byte-Size Suffixes**: Support offsets like `+1K`, `+2M` in arguments.
- [x] **Repository Polishing**
- [x] Refine repository description with key USPs.
- [x] Add relevant topics (`hex-viewer`, `c`, `cli`, `security`, `reverse-engineering`).
- [x] Set up automated **Changelog** generation via GitHub Actions.
- [x] **Documentation (README.md)**
- [x] Add a high-quality **Demo GIF** showing adaptive colors.
- [x] Detailed installation guide for all platforms.
- [x] **Auto skip**: Skip null lines
- [x] **Byte Grouping**: Setting for advanced bytegrouping `g`
- [x] **Advanced Mode**: Octal, decimal, binary

## 🐞 Bugs
- [ ] hxed.exe -f (file) -l 100 -hm adaptiv : fixed adaptiv heatmap while limit is set
