#!/usr/bin/env bash

set -euo pipefail

REPO_URL="https://github.com/jjice/hxed"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

say() {
    printf '%s\n' "$*"
}

warn() {
    printf 'Warning: %s\n' "$*" >&2
}

die() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

expand_path() {
    case "$1" in
        "~")
            printf '%s\n' "$HOME"
            ;;
        "~/"*)
            printf '%s/%s\n' "$HOME" "${1#~/}"
            ;;
        *)
            printf '%s\n' "$1"
            ;;
    esac
}

prompt_with_default() {
    local prompt="$1"
    local default_value="$2"
    local value
    read -r -p "$prompt [$default_value]: " value
    if [[ -z "$value" ]]; then
        value="$default_value"
    fi
    printf '%s\n' "$value"
}

prompt_yes_no() {
    local prompt="$1"
    local default_answer="${2:-y}"
    local suffix="[Y/n]"

    if [[ "$default_answer" == "n" ]]; then
        suffix="[y/N]"
    fi

    while true; do
        local answer
        read -r -p "$prompt $suffix: " answer
        answer="${answer:-$default_answer}"
        case "${answer,,}" in
            y|yes) return 0 ;;
            n|no) return 1 ;;
        esac
        say "Please answer y or n."
    done
}

prompt_install_mode() {
    local allow_release="$1"

    while true; do
        if [[ "$allow_release" == "yes" ]]; then
            printf '%s\n' "Installation source:" >&2
            printf '%s\n' "  1) Build from source in this repository" >&2
            printf '%s\n' "  2) Download the latest release binary from GitHub" >&2
            read -r -p "Choose 1 or 2 [2]: " answer
            answer="${answer:-2}"
            case "$answer" in
                1) printf 'build\n'; return ;;
                2) printf 'release\n'; return ;;
            esac
        else
            printf '%s\n' "A matching prebuilt release asset is not available for this platform." >&2
            read -r -p "Build from source instead? [Y/n]: " answer
            answer="${answer:-y}"
            case "${answer,,}" in
                y|yes) printf 'build\n'; return ;;
                n|no) die "Installation cancelled." ;;
            esac
        fi
        printf '%s\n' "Please answer with a valid choice." >&2
    done
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

download_file() {
    local url="$1"
    local target="$2"

    if command -v curl >/dev/null 2>&1; then
        curl -fL "$url" -o "$target"
        return
    fi

    if command -v wget >/dev/null 2>&1; then
        wget -O "$target" "$url"
        return
    fi

    die "Please install curl or wget to download release binaries."
}

append_if_missing() {
    local file="$1"
    local line="$2"

    mkdir -p "$(dirname "$file")"
    touch "$file"
    if ! grep -Fqx "$line" "$file"; then
        printf '\n%s\n' "$line" >> "$file"
    fi
}

install_binary_from_build() {
    require_cmd cmake

    [[ -f "${REPO_ROOT}/CMakeLists.txt" ]] || die "Build mode requires a repository checkout."

    say "Configuring Release build..."
    cmake -S "$REPO_ROOT" -B "${REPO_ROOT}/build" -DCMAKE_BUILD_TYPE=Release
    say "Building hxed..."
    cmake --build "${REPO_ROOT}/build" --config Release

    if [[ -x "${REPO_ROOT}/build/bin/hxed" ]]; then
        install -m 0755 "${REPO_ROOT}/build/bin/hxed" "${INSTALL_DIR}/hxed"
        return
    fi

    if [[ -x "${REPO_ROOT}/build/bin/Release/hxed" ]]; then
        install -m 0755 "${REPO_ROOT}/build/bin/Release/hxed" "${INSTALL_DIR}/hxed"
        return
    fi

    die "Build finished, but no hxed binary was found in build/bin."
}

install_binary_from_release() {
    local temp_file
    temp_file="$(mktemp)"
    trap 'rm -f "$temp_file"' RETURN

    say "Downloading latest release binary..."
    download_file "${RELEASE_URL}" "$temp_file"
    install -m 0755 "$temp_file" "${INSTALL_DIR}/hxed"
}

install_completions() {
    local completion_root="${REPO_ROOT}/completions"
    [[ -d "$completion_root" ]] || die "Completion files were not found under ${completion_root}."

    local bash_target="${HOME}/.local/share/bash-completion/completions/hxed"
    local zsh_target="${HOME}/.zsh/completions/_hxed"
    local fish_target="${HOME}/.config/fish/completions/hxed.fish"

    mkdir -p "$(dirname "$bash_target")" "$(dirname "$zsh_target")" "$(dirname "$fish_target")"
    install -m 0644 "${completion_root}/hxed.bash" "$bash_target"
    install -m 0644 "${completion_root}/_hxed" "$zsh_target"
    install -m 0644 "${completion_root}/hxed.fish" "$fish_target"

    append_if_missing "${HOME}/.zshrc" 'fpath=(~/.zsh/completions $fpath)'
    append_if_missing "${HOME}/.zshrc" 'autoload -Uz compinit && compinit'

    say "Installed Bash, Zsh, and Fish completions for the current user."
}

ensure_path_entry() {
    if [[ ":$PATH:" == *":${INSTALL_DIR}:"* ]]; then
        say "PATH already contains ${INSTALL_DIR}."
        return
    fi

    if ! prompt_yes_no "Add ${INSTALL_DIR} to your PATH?" "y"; then
        say "Skipped PATH update. Add ${INSTALL_DIR} manually if needed."
        return
    fi

    case "$(basename "${SHELL:-}")" in
        bash)
            append_if_missing "${HOME}/.bashrc" "export PATH=\"${INSTALL_DIR}:\$PATH\""
            append_if_missing "${HOME}/.profile" "export PATH=\"${INSTALL_DIR}:\$PATH\""
            ;;
        zsh)
            append_if_missing "${HOME}/.zshrc" "export PATH=\"${INSTALL_DIR}:\$PATH\""
            append_if_missing "${HOME}/.profile" "export PATH=\"${INSTALL_DIR}:\$PATH\""
            ;;
        fish)
            append_if_missing "${HOME}/.config/fish/config.fish" "fish_add_path \"${INSTALL_DIR}\""
            ;;
        *)
            append_if_missing "${HOME}/.profile" "export PATH=\"${INSTALL_DIR}:\$PATH\""
            ;;
    esac

    export PATH="${INSTALL_DIR}:$PATH"
    say "PATH updated for future shells."
}

UNAME_S="$(uname -s)"
UNAME_M="$(uname -m)"
ALLOW_RELEASE="yes"
RELEASE_ASSET=""

case "$UNAME_S" in
    Linux)
        if [[ "$UNAME_M" == "x86_64" ]]; then
            RELEASE_ASSET="hxed-linux-x64"
        else
            ALLOW_RELEASE="no"
        fi
        ;;
    Darwin)
        if [[ "$UNAME_M" == "arm64" ]]; then
            RELEASE_ASSET="hxed-macos-arm64"
        else
            ALLOW_RELEASE="no"
        fi
        ;;
    *)
        die "This installer supports Linux and macOS only."
        ;;
esac

DEFAULT_INSTALL_DIR="${HOME}/.local/bin"
INSTALL_DIR="$(expand_path "$(prompt_with_default "Install hxed into which directory?" "$DEFAULT_INSTALL_DIR")")"
mkdir -p "$INSTALL_DIR"

INSTALL_MODE="$(prompt_install_mode "$ALLOW_RELEASE")"
RELEASE_URL="${REPO_URL}/releases/latest/download/${RELEASE_ASSET}"

if [[ "$INSTALL_MODE" == "build" ]]; then
    install_binary_from_build
else
    install_binary_from_release
fi

ensure_path_entry

if prompt_yes_no "Install shell completions as well?" "y"; then
    install_completions
fi

say ""
say "hxed was installed to ${INSTALL_DIR}/hxed"
say "Open a new shell or reload your shell config before using the updated PATH."
