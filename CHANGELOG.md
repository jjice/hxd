# Changelog

All notable changes to this project will be documented in this file.

## [1.0.0] - 2026-02-24

### Changed
- Initial public release of `hxed`.
- Added the core hex dump viewer with limit and offset support.
- Introduced CMake-based builds, CI groundwork, and the first cross-platform release preparation.
- Improved file reading performance and fixed an early memory leak.
- Added colorized output, pipeline support, tests, and general project setup improvements.

## [1.1.0] - 2026-02-26

### Changed
- **Features:** Added raw output mode and improved raw formatting.
- **Features:** Renamed the project from `hxd` to `hxed`.
- **Features:** Improved argument parsing and overall search behavior.
- **Fixes:** Fixed several search-related bugs and datatype issues.
- **Fixes:** Cleaned up code and corrected minor typos.
- **Docs:** Improved installation notes and refreshed project naming in the documentation.

## [1.1.1] - 2026-02-26

### Changed
- Maintenance release for `v1.1.0`.
- Included final typo and packaging cleanup for the `1.1.x` release line.

## [1.1.2] - 2026-02-26

### Changed
- **Fixes:** Added further bug fixes and improved release build reliability.
- **Build:** Refined the release packaging workflow.

## [1.2.0] - 2026-02-27

### Changed
- **Features:** Improved suffix support for numeric arguments.
- **Fixes:** Included additional minor bug fixes across the CLI flow.
- **Docs and Tests:** Expanded README documentation for raw output mode.
- **Docs and Tests:** Added more tests and general documentation updates.

## [1.2.1] - 2026-03-02

### Changed
- **Features:** Added byte-size suffix support more broadly to argument handling.
- **Fixes:** Included minor follow-up bug fixes after `v1.2.0`.
- **Docs and Tests:** Updated README examples and usage notes.
- **Docs and Tests:** Added another test case.

## [1.3.1] - 2026-03-18

### Changed
- **Features:** Refreshed the UI and output presentation.
- **Fixes:** Applied a general bug-fix pass and code cleanup.
- **Docs:** Improved project documentation and corrected minor typos.

## [1.3.2] - 2026-03-21

### Changed
- **Features:** Added more dump modes.
- **Features:** Added byte grouping support.
- **Features:** Added skip-zero-line behavior.
- **Fixes:** Improved output formatting reliability.
- **Docs:** Updated roadmap and README cleanup items.

## [1.4.1] - 2026-03-24

### Changed
- **Features:** Added analysis-related functionality and improved analysis output.
- **Features:** Added more application features and internal source updates.
- **Features:** Improved argument handling and the internal search option structure.
- **Maintenance:** Refined versioning and repository helper scripts.
- **Docs:** Updated project assets and related documentation material.

## [1.4.2] - 2026-03-25

### Changed
- **Features:** Added an install script, bash completion support, and a man page.
- **Build:** Improved compile options and installation workflow.
- **Build:** Refined the install script.
- **Docs:** Improved README formatting and shell usage documentation.
- **Docs:** Fixed pattern-search documentation and minor typos.

## [1.4.3] - 2026-03-26

### Changed
- **Features:** Improved terminal colors and scripting support.
- **Fixes:** Fixed the stats output.
- **Fixes:** Fixed adaptive heatmap behavior by applying it per line instead of per file.
- **Fixes:** Included an additional general bug fix.
- **Maintenance:** Switched the changelog file to UTF-8 with BOM.

## [1.5.0] - 2026-04-20
- **Features:** Added default config file with parser
- **Features:** Added reverse feature
- **Features:** Added CPack (Windows installer)
- **Fixes:** Improved CMakeLists
- **Fixes:** Fixed type and bugs in completions
- **Fixes:** Fixed minor bugs
- **Maintenance:** Update MAN page