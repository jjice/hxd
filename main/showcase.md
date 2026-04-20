Perfekt, hier sind konkrete Beispiel-Commands mit erwarteter Ausgabe für `--reverse` (gekürzt, ohne Farben):

1. Hex-Stream (`-m 0`) via Pipe
```bash
printf '48656c6c6f20576f726c64\n' | hxed -re -m 0
```
Erwarteter Kern-Output:
```text
dump for <pipe>:
offset   | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | ascii
---------------------------------------------------------------------
00000000 | 48 65 6c 6c 6f 20 57 6f 72 6c 64                | Hello World
---------------------------------------------------------------------
summary  | size 11 B ; zero 0 (0.0%) ; magic 0
```

2. Binär-Stream (`-m 1`) via Pipe
```bash
printf '01001000 01100101 01101100 01101100 01101111\n' | hxed -re -m 1
```
Erwarteter Kern-Output:
```text
00000000 | 01001000 01100101 01101100 01101100 01101111 ... | Hello
summary  | size 5 B ; ...
```

3. Oktal-Stream (`-m 2`) via Pipe
```bash
printf '110,145,154,154,157\n' | hxed -re -m 2
```
Erwarteter Kern-Output:
```text
00000000 | 110 145 154 154 157 ... | Hello
summary  | size 5 B ; ...
```

4. Dezimal-Stream (`-m 3`) via Pipe
```bash
printf '72,101,108,108,111\n' | hxed -re -m 3
```
Erwarteter Kern-Output:
```text
00000000 | 072 101 108 108 111 ... | Hello
summary  | size 5 B ; ...
```

5. Datei statt Pipe (Hex)
```bash
printf '48656c6c6f0a' > /tmp/rev.txt
hxed -re -m 0 /tmp/rev.txt
```
Erwarteter Kern-Output:
```text
00000000 | 48 65 6c 6c 6f 0a ... | Hello.
summary  | size 6 B ; ...
```
Hinweis: `0a` wird als Zeilenumbruch in ASCII als `.` dargestellt.

6. Fehlerfall (ungerade Hex-Länge)
```bash
printf 'ABC\n' | hxed -re -m 0
```
Erwartete Fehlermeldung:
```text
Reverse hex parse error: odd number of hex digits
```

