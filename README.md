# hxd - A Simple Hex Dump Utility 📂

`hxd` is a small command-line tool written in C that displays the content of a file in a classic hex dump format. For each line, it outputs the offset, the hexadecimal representation of the bytes, and the corresponding printable ASCII characters.

## ✨ Features

*   **Offset Display**: Shows the file offset in hexadecimal.
*   **Hex View**: Renders the bytes in hexadecimal notation (16 bytes per line).
*   **ASCII View**: Outputs the printable characters. Non-printable characters are replaced with a period (`.`).
*   **Simplicity**: No external dependencies and easy to compile.
*   **Offset and Limit**: Set start byte to read an count of read bytes

## 🛠️ Compiling

To compile the program, you need a C compiler like `gcc` or `clang`.

```bash
# Using cmake
cmake

cd build/

make

sudo make install

# Or just as a standalone bin using Clang / GCC
mkdir build

clang src/hxd.c -o build/hxd
# or
gcc src/hxd.c -o build/hxd
```

## 🚀 Usage

The utility is run from the command line and takes exactly one argument.

```bash
./hxd <file_path>
```

### Arguments

| Argument      | Description                                                                                                 | Required |
|---------------|-------------------------------------------------------------------------------------------------------------|----------|
| `<-f file_path>` | The path to the file you want to inspect. This can be relative (e.g., `example.txt`) or absolute. | Yes      |
| `<-a (on/off)>` | If you want a ASCII Table `-a off`, default is on | No      |
| `<-h>` | Shows help | No      |
| `<-o>` | Set start byte to read | No      |
| `<-l>` | Set count of bytes to read  | No      |
| `<-b (int)>` | Declare buffer size -> how many bytes are in a row. `-b 8`, default is 16 Bytes | No      |

If no file path is provided, or if more than one argument is given, the program will print a usage message to the standard error stream and exit.

### Example

Assume you have a file named `example.txt` with the content `Hello World!`.

The command:
```bash
./hxd example.txt
```

Produces the following output:

```
           00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
---------------------------------------------------------------+--------------
00000000 | 48 65 6c 6c 6f 20 57 6f 72 6c 64                    |   Hello World
```
The command:
```bash
./hxd example.txt -b 8
```
Produces the following output:


```
           00 01 02 03 04 05 06 07
---------------------------------------+-----------
00000000 | 48 65 6c 6c 6f 20 57 6f     |   Hello Wo
00000008 | 72 6c 64                    |   rld
```