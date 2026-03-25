# bash completion for hxed

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
