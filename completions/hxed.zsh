#compdef hxed

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
    '-re[Toggle reverse mode]'
    '--reverse[Toggle reverse mode]'
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
    '--show-config[Show current config and exit]'
    '-h[Show help]'
    '--help[Show help]'
    '-v[Show version]'
    '--version[Show version]'
    '*:filename:_files'
  )
  _arguments $opts
}

_hxed "$@"
