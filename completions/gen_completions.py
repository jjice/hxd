from pathlib import Path

base = Path('.')
files = {}

files['hxed.bash'] = r'''# bash completion for hxed

_hxed()
{
    local cur prev words cword
    _init_completion -n = || return

    local opts modes heatmaps
    opts="-f --file -m --mode -hm --heatmap -w --width -g --grouping -a --ascii -c --color -s --string -e --entropy -th --toggle-header -sz --skip-zero -o --offset -l --limit -r --read-size -se --search -p --pager -ro --raw -h --help -v --version"
    modes="0 1 2 3 hex bin oct dec"
    heatmaps="adaptiv fixed none"

    case "$prev" in
        -f|--file)
            _filedir
            return
            ;;
        -m|--mode)
            COMPREPLY=( $(compgen -W "$modes" -- "$cur") )
            return
            ;;
        -hm|--heatmap)
            COMPREPLY=( $(compgen -W "$heatmaps" -- "$cur") )
            return
            ;;
        -w|--width|-g|--grouping|-o|--offset|-l|--limit|-r|--read-size)
            return
            ;;
        -se|--search)
            COMPREPLY=( $(compgen -W "a: x: d: b:" -- "$cur") )
            return
            ;;
    esac

    if [[ "$cur" == -* ]]; then
        COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
        return
    fi

    _filedir
}

complete -F _hxed hxed
'''

files['_hxed'] = r'''#compdef hxed

_hxed() {
  local -a opts
  opts=(
    '-f[Input file]:file:_files'
    '--file[Input file]:file:_files'
    '-m[Output mode]:mode:(0 1 2 3 hex bin oct dec)'
    '--mode[Output mode]:mode:(0 1 2 3 hex bin oct dec)'
    '-hm[Heatmap mode]:heatmap:(adaptiv fixed none)'
    '--heatmap[Heatmap mode]:heatmap:(adaptiv fixed none)'
    '-w[Bytes per line (0 only with --raw)]:width:'
    '--width[Bytes per line (0 only with --raw)]:width:'
    '-g[Grouping size]:grouping:'
    '--grouping[Grouping size]:grouping:'
    '-a[Toggle ASCII column]'
    '--ascii[Toggle ASCII column]'
    '-c[Toggle color output]'
    '--color[Toggle color output]'
    '-s[Toggle string highlighting]'
    '--string[Toggle string highlighting]'
    '-e[Toggle entropy graph]'
    '--entropy[Toggle entropy graph]'
    '-th[Toggle header/footer and magic-byte detection]'
    '--toggle-header[Toggle header/footer and magic-byte detection]'
    '-sz[Toggle skip-zero lines]'
    '--skip-zero[Toggle skip-zero lines]'
    '-o[Start offset (supports k/M/G)]:offset:'
    '--offset[Start offset (supports k/M/G)]:offset:'
    '-l[Stop at byte position (supports k/M/G)]:limit:'
    '--limit[Stop at byte position (supports k/M/G)]:limit:'
    '-r[Read at most N bytes (supports k/M/G)]:read-size:'
    '--read-size[Read at most N bytes (supports k/M/G)]:read-size:'
    '-se[Search pattern]:pattern:(a: x: d: b:)'
    '--search[Search pattern]:pattern:(a: x: d: b:)'
    '-p[Toggle pager output]'
    '--pager[Toggle pager output]'
    '-ro[Raw output mode]'
    '--raw[Raw output mode]'
    '-h[Show help]'
    '--help[Show help]'
    '-v[Show version]'
    '--version[Show version]'
    '*:filename:_files'
  )
  _arguments -s $opts
}

_hxed "$@"
'''

files['hxed.fish'] = r'''# fish completion for hxed

complete -c hxed -s f -l file -r -d 'Input file'
complete -c hxed -s m -l mode -r -f -a '0 1 2 3 hex bin oct dec' -d 'Output mode'
complete -c hxed -s hm -l heatmap -r -f -a 'adaptiv fixed none' -d 'Heatmap mode'
complete -c hxed -s w -l width -r -d 'Bytes per line (0 only with --raw)'
complete -c hxed -s g -l grouping -r -d 'Grouping size'
complete -c hxed -s a -l ascii -d 'Toggle ASCII column'
complete -c hxed -s c -l color -d 'Toggle color output'
complete -c hxed -s s -l string -d 'Toggle string highlighting'
complete -c hxed -s e -l entropy -d 'Toggle entropy graph'
complete -c hxed -s th -l toggle-header -d 'Toggle header/footer and magic-byte detection'
complete -c hxed -s sz -l skip-zero -d 'Toggle skip-zero lines'
complete -c hxed -s o -l offset -r -d 'Start offset (supports k/M/G)'
complete -c hxed -s l -l limit -r -d 'Stop at byte position (supports k/M/G)'
complete -c hxed -s r -l read-size -r -d 'Read at most N bytes (supports k/M/G)'
complete -c hxed -s se -l search -r -f -a 'a: x: d: b:' -d 'Search pattern'
complete -c hxed -s p -l pager -d 'Toggle pager output'
complete -c hxed -s ro -l raw -d 'Raw output mode'
complete -c hxed -s h -l help -d 'Show help'
complete -c hxed -s v -l version -d 'Show version'
'''

files['completion_install_snippets.txt'] = r'''Bash:
  mkdir -p ~/.local/share/bash-completion/completions
  cp hxed.bash ~/.local/share/bash-completion/completions/hxed

Zsh:
  mkdir -p ~/.zsh/completions
  cp _hxed ~/.zsh/completions/_hxed
  # add this once to ~/.zshrc:
  fpath=(~/.zsh/completions $fpath)
  autoload -Uz compinit && compinit

Fish:
  mkdir -p ~/.config/fish/completions
  cp hxed.fish ~/.config/fish/completions/hxed.fish

PowerShell:
  . "$PWD\hxed.ps1"
'''

for name, content in files.items():
    (base / name).write_text(content, encoding='utf-8')

print("Created:", ", ".join(files.keys()))
