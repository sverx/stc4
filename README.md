# Simple Tile Compression Four

## BMP2Tile plugin and Z80 decompression routine

#### The Simple Tile Compression Four compression algorithm is a very simple tile compression algorithm based on the Simple Tile Compression Zero algorithm.
The Simple Tile Compression Zero exploits the fact that SEGA Master System mode-4 tile data typically includes lots of bytes having either value 0x00 or 0xFF. It works on groups of 4 bytes (8 groups are needed for each tile) and in a best case scenario it compresses them up to 75%, that's when a whole group of 4 uncompressed bytes turns into a single compressed byte. Beside that, it compresses sequences of same bytes in the group too.

The Simple Tile Compression Four extends this (also making any stc0 compressed data still valid) by adding a way to store differences from the previous group of four bytes and reruns of the same sequence of bytes.

The Simple Tile Compression Four also supports multiple repetitions of the same 4 bytes (thus basically RLE) and also supports differences from the previous group of 4 bytes, even complemented.

An additional feature of the format is that is possible to merge compressed tilesets into a single set by simply removing the last byte of each one, except the last, and concatenatenating the files.

