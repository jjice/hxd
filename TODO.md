# 🛠 Project Roadmap & TODOs

This document tracks the progress of **hxd**. Key focus areas include performance parity with `xxd`, robust testing, and advanced binary analysis features.

---

## 🚀 GitHub & Presence
- [ ] **Repository Polishing**
  - [x] Refine repository description with key USPs.
  - [x] Add relevant topics (`hex-viewer`, `c`, `cli`, `security`, `reverse-engineering`).
  - [x] Set up automated **Changelog** generation via GitHub Actions.
- [ ] **Documentation (README.md)**
  - [ ] Add a high-quality **Demo GIF** showing adaptive colors.
  - [x] Detailed installation guide for all platforms.
  - [ ] **Benchmark Section:** Performance comparison against `xxd` and `hexdump` (using `hyperfine`).

## 🛠 Features & Core Logic
### 🔍 Analysis
- [ ] **Magic Bytes Recognition**
  - [x] Implement `MagicTable` lookup logic.
  - [ ] Output detected file format in the header.

- [ ] **Search function**
  
- [x] **String Highlighting**
  - [x] CLI arguments and options added.
  - [x] Implement detection logic 
  - [ ] More advanced detection

### 🛠 Functionality
- [ ] **Live-Preview (`--follow`)**: Monitor file changes in real-time.
- [ ] **Byte-Size Suffixes**: Support offsets like `+1K`, `+2M` in arguments.
- [ ] **Comparison Mode**: Side-by-side diffing of two binary files.
- [ ] **Advanced Mode**: Octal, decimal, binary
- [ ] **Reverse Mode**: Reverse hex to bin
- [ ] **RAW Output-Mode**: Output in raw data 
---

## 📦 Distribution & Releases
- [ ] **Package Manager Integration**
  - [ ] Submit to AUR (Arch User Repository).
  - [ ] Add a binary .deb
  - [ ] Scoop/Winget manifest for Windows.

---

## 🧪 Testing & Quality Assurance
- [ ] **Automated Testing**
  - [ ] Implement **Unit Tests** for core utility functions.
  - [ ] Integrate **cppcheck** into CI pipeline for static analysis.
- [ ] **Memory & Security**
  - [ ] Run **Valgrind** to ensure zero memory leaks.
  - [ ] Implement **Fuzzing** (using `AFL++` or `libFuzzer`) to harden the parser against corrupt binaries.

---

## ✅ Completed
- [x] Overhaul CLI argument parsing.
- [x] Redesign help menu for better readability.
- [x] Multi-platform CI/CD (Windows, Linux, macOS).
- [x] Entropy Visualization
- [x] Heatmap (fixed | adaptiv)

## 🐞 Bugs
- [ ] hxed.exe test -l 100 -hm adaptiv : fixed adaptiv heatmap while limit is set