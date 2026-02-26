/*
 * hxed - A modern hex dumper
 * Copyright (c) 2026 Joshua Jallow
 * Licensed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */

#include "MagicBytes.h"

static const uint8_t sig_0000[] = { 0x4D, 0x5A };
static const uint8_t sig_0001[] = { 0x7F, 0x45, 0x4C, 0x46 };
static const uint8_t sig_0002[] = { 0xCF, 0xFA, 0xED, 0xFE };
static const uint8_t sig_0003[] = { 0xCE, 0xFA, 0xED, 0xFE };
static const uint8_t sig_0004[] = { 0xCA, 0xFE, 0xBA, 0xBE };
static const uint8_t sig_0005[] = { 0x00, 0x61, 0x73, 0x6D };
static const uint8_t sig_0006[] = { 0xED, 0xAB, 0xEE, 0xDB };
static const uint8_t sig_0007[] = { 0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E };
static const uint8_t sig_0008[] = { 0x4D, 0x53, 0x43, 0x46 };
static const uint8_t sig_0009[] = { 0x49, 0x53, 0x63, 0x28 };
static const uint8_t sig_0010[] = { 0x25, 0x50, 0x44, 0x46, 0x2D };
static const uint8_t sig_0011[] = { 0x50, 0x4B, 0x03, 0x04 };
static const uint8_t sig_0012[] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };
static const uint8_t sig_0013[] = { 0x7B, 0x5C, 0x72, 0x74, 0x66, 0x31 };
static const uint8_t sig_0014[] = { 0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20 };
static const uint8_t sig_0015[] = { 0xEF, 0xBB, 0xBF };
static const uint8_t sig_0016[] = { 0xFE, 0xFF };
static const uint8_t sig_0017[] = { 0xFF, 0xFE };
static const uint8_t sig_0018[] = { 0x00, 0x00, 0xFE, 0xFF };
static const uint8_t sig_0019[] = { 0xFF, 0xFE, 0x00, 0x00 };
static const uint8_t sig_0020[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
static const uint8_t sig_0021[] = { 0xFF, 0xD8, 0xFF, 0xE0 };
static const uint8_t sig_0022[] = { 0xFF, 0xD8, 0xFF, 0xE1 };
static const uint8_t sig_0023[] = { 0xFF, 0xD8, 0xFF, 0xEE };
static const uint8_t sig_0024[] = { 0xFF, 0xD8, 0xFF, 0xDB };
static const uint8_t sig_0025[] = { 0x47, 0x49, 0x46, 0x38, 0x39, 0x61 };
static const uint8_t sig_0026[] = { 0x47, 0x49, 0x46, 0x38, 0x37, 0x61 };
static const uint8_t sig_0027[] = { 0x42, 0x4D };
static const uint8_t sig_0028[] = { 0x49, 0x49, 0x2A, 0x00 };
static const uint8_t sig_0029[] = { 0x4D, 0x4D, 0x00, 0x2A };
static const uint8_t sig_0030[] = { 0x52, 0x49, 0x46, 0x46 };
static const uint8_t sig_0031[] = { 0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x68, 0x65, 0x69, 0x63 };
static const uint8_t sig_0032[] = { 0x38, 0x42, 0x50, 0x53 };
static const uint8_t sig_0033[] = { 0x67, 0x69, 0x6D, 0x70, 0x20, 0x78, 0x63, 0x66, 0x20, 0x76 };
static const uint8_t sig_0034[] = { 0x00, 0x00, 0x01, 0x00 };
static const uint8_t sig_0035[] = { 0x00, 0x00, 0x02, 0x00 };
static const uint8_t sig_0036[] = { 0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49 };
static const uint8_t sig_0037[] = { 0x23, 0x46, 0x49, 0x47 };
static const uint8_t sig_0038[] = { 0x46, 0x4F, 0x52, 0x4D };
static const uint8_t sig_0039[] = { 0x2E, 0x72, 0x61, 0xFD };
static const uint8_t sig_0040[] = { 0x49, 0x44, 0x33 };
static const uint8_t sig_0041[] = { 0xFF, 0xFB };
static const uint8_t sig_0042[] = { 0x66, 0x4C, 0x61, 0x43 };
static const uint8_t sig_0043[] = { 0x4F, 0x67, 0x67, 0x53 };
static const uint8_t sig_0044[] = { 0x46, 0x4F, 0x52, 0x4D, 0x00 };
static const uint8_t sig_0045[] = { 0x52, 0x49, 0x46, 0x46 };
static const uint8_t sig_0046[] = { 0x4D, 0x54, 0x68, 0x64 };
static const uint8_t sig_0047[] = { 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11 };
static const uint8_t sig_0048[] = { 0x2E, 0x73, 0x6E, 0x64 };
static const uint8_t sig_0049[] = { 0x1A, 0x45, 0xDF, 0xA3 };
static const uint8_t sig_0050[] = { 0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D };
static const uint8_t sig_0051[] = { 0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70, 0x71, 0x74, 0x20, 0x20 };
static const uint8_t sig_0052[] = { 0x52, 0x49, 0x46, 0x46 };
static const uint8_t sig_0053[] = { 0x46, 0x4C, 0x56, 0x01 };
static const uint8_t sig_0054[] = { 0x00, 0x00, 0x01, 0xBA };
static const uint8_t sig_0055[] = { 0x00, 0x00, 0x01, 0xB3 };
static const uint8_t sig_0056[] = { 0x50, 0x4B, 0x03, 0x04 };
static const uint8_t sig_0057[] = { 0x50, 0x4B, 0x05, 0x06 };
static const uint8_t sig_0058[] = { 0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C };
static const uint8_t sig_0059[] = { 0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00 };
static const uint8_t sig_0060[] = { 0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00 };
static const uint8_t sig_0061[] = { 0x1F, 0x8B };
static const uint8_t sig_0062[] = { 0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00 };
static const uint8_t sig_0063[] = { 0x42, 0x5A, 0x68 };
static const uint8_t sig_0064[] = { 0x75, 0x73, 0x74, 0x61, 0x72 };
static const uint8_t sig_0065[] = { 0x1F, 0x9D };
static const uint8_t sig_0066[] = { 0x4C, 0x5A, 0x49, 0x50 };
static const uint8_t sig_0067[] = { 0x28, 0xB5, 0x2F, 0xFD };
static const uint8_t sig_0068[] = { 0x7F, 0x10, 0xDA, 0xBE };
static const uint8_t sig_0069[] = { 0x23, 0x20, 0x44, 0x69, 0x73, 0x6B, 0x20, 0x44 };
static const uint8_t sig_0070[] = { 0x43, 0x4F, 0x4E, 0x45, 0x43, 0x54, 0x49, 0x58 };
static const uint8_t sig_0071[] = { 0x51, 0x46, 0x49, 0xFB };
static const uint8_t sig_0072[] = { 0x43, 0x44, 0x30, 0x30, 0x31 };
static const uint8_t sig_0073[] = { 0x6B, 0x6F, 0x6C, 0x79 };
static const uint8_t sig_0074[] = { 0xE9 };
static const uint8_t sig_0075[] = { 0x53, 0x51, 0x4C, 0x69, 0x74, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x20, 0x33, 0x00 };
static const uint8_t sig_0076[] = { 0x00, 0x01, 0x00, 0x00, 0x53, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x72, 0x64, 0x20, 0x4A, 0x65, 0x74 };
static const uint8_t sig_0077[] = { 0x21, 0x42, 0x44, 0x4E };
static const uint8_t sig_0078[] = { 0x00, 0x01, 0x00, 0x00, 0x00 };
static const uint8_t sig_0079[] = { 0x4F, 0x54, 0x54, 0x4F };
static const uint8_t sig_0080[] = { 0x77, 0x4F, 0x46, 0x46 };
static const uint8_t sig_0081[] = { 0x77, 0x4F, 0x46, 0x32 };
static const uint8_t sig_0082[] = { 0x23, 0x21 };
static const uint8_t sig_0083[] = { 0x49, 0x57, 0x41, 0x44 };
static const uint8_t sig_0084[] = { 0x50, 0x57, 0x53, 0x33 };
static const uint8_t sig_0085[] = { 0x72, 0x65, 0x67, 0x65, 0x64, 0x69, 0x74 };
static const uint8_t sig_0086[] = { 0x4C, 0x00, 0x00, 0x00, 0x01, 0x14, 0x02, 0x00 };
static const uint8_t sig_0087[] = { 0x21, 0x12 };
static const uint8_t sig_0088[] = { 0x49, 0x4E, 0x44, 0x58 };
static const uint8_t sig_0089[] = { 0x00, 0x00, 0x00, 0x0C, 0x6A, 0x50, 0x20, 0x20 };
static const uint8_t sig_0090[] = { 0x00, 0x00, 0x00, 0x00, 0x62, 0x31, 0x05, 0x00 };
static const uint8_t sig_0091[] = { 0x64, 0x65, 0x78, 0x0A, 0x30, 0x33, 0x35, 0x00 };
static const uint8_t sig_0092[] = { 0x1A, 0x00, 0x00, 0x04, 0x00, 0x00 };
static const uint8_t sig_0093[] = { 0x0A, 0x0D, 0x0D, 0x0A };
static const uint8_t sig_0094[] = { 0xD4, 0xC3, 0xB2, 0xA1 };
static const uint8_t sig_0095[] = { 0xA1, 0xB2, 0xC3, 0xD4 };
static const uint8_t sig_0096[] = { 0x7B, 0x22 };
static const uint8_t sig_0097[] = { 0x5B, 0x22 };
static const uint8_t sig_0098[] = { 0x3C, 0x21, 0x44, 0x4F, 0x43, 0x54, 0x59, 0x50, 0x45, 0x20, 0x68, 0x74, 0x6D, 0x6C };
static const uint8_t sig_0099[] = { 0x49, 0x54, 0x53, 0x46 };

const MagicSignature magic_signatures[] = {
    { sig_0000, 2, 0, "exe, dll, sys, drv", "Windows PE Executable / DOS MZ executable" },  // 4D 5A  offset 0  exe, dll, sys, drv
    { sig_0001, 4, 0, "elf, bin, so, o", "Linux Executable and Linkable Format (ELF)" },  // 7F 45 4C 46  offset 0  elf, bin, so, o
    { sig_0002, 4, 0, "mach-o", "macOS Mach-O Executable (64-bit)" },  // CF FA ED FE  offset 0  mach-o
    { sig_0003, 4, 0, "mach-o", "macOS Mach-O Executable (32-bit)" },  // CE FA ED FE  offset 0  mach-o
    { sig_0004, 4, 0, "class", "Java Class File" },  // CA FE BA BE  offset 0  class
    { sig_0005, 4, 0, "wasm", "WebAssembly binary format" },  // 00 61 73 6D  offset 0  wasm
    { sig_0006, 4, 0, "rpm", "RedHat Package Manager (RPM) package" },  // ED AB EE DB  offset 0  rpm
    { sig_0007, 7, 0, "deb, a, lib", "Debian package oder Unix static library" },  // 21 3C 61 72 63 68 3E  offset 0  deb, a, lib
    { sig_0008, 4, 0, "cab", "Microsoft Cabinet file" },  // 4D 53 43 46  offset 0  cab
    { sig_0009, 4, 0, "cab", "InstallShield Cabinet file" },  // 49 53 63 28  offset 0  cab
    { sig_0010, 5, 0, "pdf", "Adobe Portable Document Format" },  // 25 50 44 46 2D  offset 0  pdf
    { sig_0011, 4, 0, "docx, xlsx, pptx, jar", "ZIP-basiertes Office Open XML / Java Archive" },  // 50 4B 03 04  offset 0  docx, xlsx, pptx, jar
    { sig_0012, 8, 0, "doc, xls, ppt, msi", "Microsoft Office Legacy (Compound File Binary)" },  // D0 CF 11 E0 A1 B1 1A E1  offset 0  doc, xls, ppt, msi
    { sig_0013, 6, 0, "rtf", "Rich Text Format" },  // 7B 5C 72 74 66 31  offset 0  rtf
    { sig_0014, 6, 0, "xml", "eXtensible Markup Language" },  // 3C 3F 78 6D 6C 20  offset 0  xml
    { sig_0015, 3, 0, "txt", "UTF-8 Byte Order Mark (BOM)" },  // EF BB BF  offset 0  txt
    { sig_0016, 2, 0, "txt", "UTF-16 Big Endian BOM" },  // FE FF  offset 0  txt
    { sig_0017, 2, 0, "txt", "UTF-16 Little Endian BOM" },  // FF FE  offset 0  txt
    { sig_0018, 4, 0, "txt", "UTF-32 Big Endian BOM" },  // 00 00 FE FF  offset 0  txt
    { sig_0019, 4, 0, "txt", "UTF-32 Little Endian BOM" },  // FF FE 00 00  offset 0  txt
    { sig_0020, 8, 0, "png", "Portable Network Graphics" },  // 89 50 4E 47 0D 0A 1A 0A  offset 0  png
    { sig_0021, 4, 0, "jpg, jpeg", "JPEG image (JFIF)" },  // FF D8 FF E0  offset 0  jpg, jpeg
    { sig_0022, 4, 0, "jpg, jpeg", "JPEG image (Exif)" },  // FF D8 FF E1  offset 0  jpg, jpeg
    { sig_0023, 4, 0, "jpg, jpeg", "JPEG image (Adobe)" },  // FF D8 FF EE  offset 0  jpg, jpeg
    { sig_0024, 4, 0, "jpg, jpeg", "JPEG image (Digital Camera)" },  // FF D8 FF DB  offset 0  jpg, jpeg
    { sig_0025, 6, 0, "gif", "Graphics Interchange Format (GIF89a)" },  // 47 49 46 38 39 61  offset 0  gif
    { sig_0026, 6, 0, "gif", "Graphics Interchange Format (GIF87a)" },  // 47 49 46 38 37 61  offset 0  gif
    { sig_0027, 2, 0, "bmp, dib", "Windows Bitmap" },  // 42 4D  offset 0  bmp, dib
    { sig_0028, 4, 0, "tif, tiff", "TIFF image (Little Endian)" },  // 49 49 2A 00  offset 0  tif, tiff
    { sig_0029, 4, 0, "tif, tiff", "TIFF image (Big Endian)" },  // 4D 4D 00 2A  offset 0  tif, tiff
    { sig_0030, 4, 0, "webp", "WebP image (RIFF container)" },  // 52 49 46 46  offset 0  webp
    { sig_0031, 12, 0, "heic", "High Efficiency Image File Format" },  // 00 00 00 18 66 74 79 70 68 65 69 63  offset 0  heic
    { sig_0032, 4, 0, "psd", "Adobe Photoshop Document" },  // 38 42 50 53  offset 0  psd
    { sig_0033, 10, 0, "xcf", "GIMP image project file" },  // 67 69 6D 70 20 78 63 66 20 76  offset 0  xcf
    { sig_0034, 4, 0, "ico", "Windows Icon file" },  // 00 00 01 00  offset 0  ico
    { sig_0035, 4, 0, "cur", "Windows Cursor file" },  // 00 00 02 00  offset 0  cur
    { sig_0036, 8, 0, "tga", "Truevision Targa image (Trailer)" },  // 54 52 55 45 56 49 53 49  offset 0  tga
    { sig_0037, 4, 0, "fig", "XFig vector graphic" },  // 23 46 49 47  offset 0  fig
    { sig_0038, 4, 0, "iff", "EA Interchange File Format" },  // 46 4F 52 4D  offset 0  iff
    { sig_0039, 4, 0, "ra, rm", "RealMedia/RealAudio file" },  // 2E 72 61 fd  offset 0  ra, rm
    { sig_0040, 3, 0, "mp3", "MP3 file with ID3v2 container" },  // 49 44 33  offset 0  mp3
    { sig_0041, 2, 0, "mp3", "MPEG-1 Layer 3 (kein ID3 Tag)" },  // FF FB  offset 0  mp3
    { sig_0042, 4, 0, "flac", "Free Lossless Audio Codec" },  // 66 4C 61 43  offset 0  flac
    { sig_0043, 4, 0, "ogg, oga, ogv", "Ogg media container" },  // 4F 67 67 53  offset 0  ogg, oga, ogv
    { sig_0044, 5, 0, "aiff, aif", "Audio Interchange File Format" },  // 46 4F 52 4D 00  offset 0  aiff, aif
    { sig_0045, 4, 0, "wav", "Waveform Audio File Format" },  // 52 49 46 46  offset 0  wav
    { sig_0046, 4, 0, "mid, midi", "Standard MIDI file" },  // 4D 54 68 64  offset 0  mid, midi
    { sig_0047, 8, 0, "wma, wmv, asf", "Advanced Systems Format" },  // 30 26 B2 75 8E 66 CF 11  offset 0  wma, wmv, asf
    { sig_0048, 4, 0, "au, snd", "Sun/NeXT audio file" },  // 2E 73 6E 64  offset 0  au, snd
    { sig_0049, 4, 0, "mkv, webm", "Matroska / WebM container" },  // 1A 45 DF A3  offset 0  mkv, webm
    { sig_0050, 12, 0, "mp4", "MPEG-4 video (ISO Media)" },  // 00 00 00 18 66 74 79 70 69 73 6F 6D  offset 0  mp4
    { sig_0051, 12, 0, "mov", "QuickTime Movie" },  // 00 00 00 20 66 74 79 70 71 74 20 20  offset 0  mov
    { sig_0052, 4, 0, "avi", "Audio Video Interleave (RIFF)" },  // 52 49 46 46  offset 0  avi
    { sig_0053, 4, 0, "flv", "Flash Video file" },  // 46 4C 56 01  offset 0  flv
    { sig_0054, 4, 0, "mpg, mpeg, vob", "MPEG-2 Program Stream" },  // 00 00 01 BA  offset 0  mpg, mpeg, vob
    { sig_0055, 4, 0, "mpg, mpeg", "MPEG-1/2 Video Sequence" },  // 00 00 01 B3  offset 0  mpg, mpeg
    { sig_0056, 4, 0, "zip", "Standard ZIP Archive" },  // 50 4B 03 04  offset 0  zip
    { sig_0057, 4, 0, "zip", "ZIP Archive (Leer/Ende)" },  // 50 4B 05 06  offset 0  zip
    { sig_0058, 6, 0, "7z", "7-Zip compressed archive" },  // 37 7A BC AF 27 1C  offset 0  7z
    { sig_0059, 8, 0, "rar", "RAR archive v5.0+" },  // 52 61 72 21 1A 07 01 00  offset 0  rar
    { sig_0060, 7, 0, "rar", "RAR archive v1.50+" },  // 52 61 72 21 1A 07 00  offset 0  rar
    { sig_0061, 2, 0, "gz, tgz", "GZIP compressed file" },  // 1F 8B  offset 0  gz, tgz
    { sig_0062, 6, 0, "xz, txz", "XZ compression utility" },  // FD 37 7A 58 5A 00  offset 0  xz, txz
    { sig_0063, 3, 0, "bz2", "BZIP2 compressed file" },  // 42 5A 68  offset 0  bz2
    { sig_0064, 5, 257, "tar", "POSIX tar archive" },  // 75 73 74 61 72  offset 257  tar
    { sig_0065, 2, 0, "z", "Unix compress LZW file" },  // 1F 9D  offset 0  z
    { sig_0066, 4, 0, "lz", "Lzip compressed file" },  // 4C 5A 49 50  offset 0  lz
    { sig_0067, 4, 0, "zst", "Zstandard (zstd) compressed file" },  // 28 B5 2F FD  offset 0  zst
    { sig_0068, 4, 0, "vdi", "VirtualBox Disk Image (VDI)" },  // 7F 10 DA BE  offset 0  vdi
    { sig_0069, 8, 0, "vmdk", "VMWare Virtual Disk" },  // 23 20 44 69 73 6B 20 44  offset 0  vmdk
    { sig_0070, 8, 0, "vhd", "Microsoft Virtual Hard Disk" },  // 43 4F 4E 45 43 54 49 58  offset 0  vhd
    { sig_0071, 4, 0, "qcow2", "QEMU Copy On Write v2" },  // 51 46 49 FB  offset 0  qcow2
    { sig_0072, 5, 32769, "iso", "ISO-9660 CD-ROM Image" },  // 43 44 30 30 31  offset 32769  iso
    { sig_0073, 4, 0, "dmg", "Apple Disk Image (Trailer signature)" },  // 6B 6F 6C 79  offset 0  dmg
    { sig_0074, 1, 0, "bin", "Allgemeines x86 Boot-Image (JMP Instruk.)" },  // E9  offset 0  bin
    { sig_0075, 16, 0, "sqlite, db", "SQLite Database v3" },  // 53 51 4C 69 74 65 20 66 6F 72 6D 61 74 20 33 00  offset 0  sqlite, db
    { sig_0076, 16, 0, "mdb, accdb", "MS Access Database (Standard Jet)" },  // 00 01 00 00 53 74 61 6E 64 61 72 64 20 4A 65 74  offset 0  mdb, accdb
    { sig_0077, 4, 0, "ost, pst", "Outlook Personal Information Store" },  // 21 42 44 4E  offset 0  ost, pst
    { sig_0078, 5, 0, "ttf", "TrueType Font" },  // 00 01 00 00 00  offset 0  ttf
    { sig_0079, 4, 0, "otf", "OpenType Font" },  // 4F 54 54 4F  offset 0  otf
    { sig_0080, 4, 0, "woff", "Web Open Font Format" },  // 77 4F 46 46  offset 0  woff
    { sig_0081, 4, 0, "woff2", "Web Open Font Format 2" },  // 77 4F 46 32  offset 0  woff2
    { sig_0082, 2, 0, "sh, py, pl, rb, php", "Unix Shebang (Skript-Header)" },  // 23 21  offset 0  sh, py, pl, rb, php
    { sig_0083, 4, 0, "wad", "Doom WAD resource file" },  // 49 57 41 44  offset 0  wad
    { sig_0084, 4, 0, "psafe3", "Password Gorilla Database" },  // 50 57 53 33  offset 0  psafe3
    { sig_0085, 7, 0, "reg", "Windows Registry file" },  // 72 65 67 65 64 69 74  offset 0  reg
    { sig_0086, 8, 0, "lnk", "Windows Shortcut file" },  // 4C 00 00 00 01 14 02 00  offset 0  lnk
    { sig_0087, 2, 0, "ain", "AIN Compressed Archive" },  // 21 12  offset 0  ain
    { sig_0088, 4, 0, "idx", "AmiBack Amiga Backup index" },  // 49 4E 44 58  offset 0  idx
    { sig_0089, 8, 0, "jp2", "JPEG 2000 JP2 image" },  // 00 00 00 0C 6A 50 20 20  offset 0  jp2
    { sig_0090, 8, 8, "dat", "Bitcoin Core wallet.dat" },  // 00 00 00 00 62 31 05 00  offset 8  dat
    { sig_0091, 8, 0, "dex", "Android Dalvik Executable" },  // 64 65 78 0a 30 33 35 00  offset 0  dex
    { sig_0092, 6, 0, "nsf", "Lotus Notes Database" },  // 1A 00 00 04 00 00  offset 0  nsf
    { sig_0093, 4, 0, "pcapng", "PCAP Next Generation Capture" },  // 0A 0D 0D 0A  offset 0  pcapng
    { sig_0094, 4, 0, "pcap", "Libpcap Capture (Little Endian)" },  // D4 C3 B2 A1  offset 0  pcap
    { sig_0095, 4, 0, "pcap", "Libpcap Capture (Big Endian)" },  // A1 B2 C3 D4  offset 0  pcap
    { sig_0096, 2, 0, "json", "JSON (beginnt mit '{')" },  // 7B 22  offset 0  json
    { sig_0097, 2, 0, "json", "JSON (beginnt mit '[')" },  // 5B 22  offset 0  json
    { sig_0098, 14, 0, "html", "HTML5 DTD" },  // 3C 21 44 4F 43 54 59 50 45 20 68 74 6D 6c  offset 0  html
    { sig_0099, 4, 0, "chm", "Microsoft Compiled HTML Help" },  // 49 54 53 46  offset 0  chm
};

const size_t magic_signatures_count = sizeof(magic_signatures) / sizeof(magic_signatures[0]);

