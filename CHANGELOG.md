# 23.11.1

 - src/decoder/decoderbase.c: Zero-fill filter tables (f514690)
 - src/converter/converter.c: Use calloc to initialise converter slot array (60f71c9)
 - src/reader/dff.c: Drop unused code (0cd0fa3)
 - src/reader/disc.c: Check return value when ellipsizing track performer (fa7b383)
 - Build using CMake (0948cd7)

# 23.8.1

 - Merge pull request #6 from plstonge/doc-filters (6c27f3e)
 - Analysis of the pcm2pcm FIR filters (c6e6627)
 - Added pcm2pcm decimation constants (74c42b8)
 - DSD to PCM filters analysis (822cdad)

# 23.4.1

 - Makefile: Add LIBDIR path to ldconfig invocation (69ca90c)
 - Merge branch 'sodface-master' (edfd28c)
 - Makefile: support custom CFLAGS (fcf7052)

# 23.1.31

 - Makefile: Swap library and symlink file names (42c3c1d)

# 22.6.16

 - Makefile: Disable format truncation warning (16c1d29)
 - src/reader/disc.c: Trim whitespaces from destination file names (bde61b9)

# 21.8.30

 - Allow decoding through errors (adb013d)

# 21.4.23

 - src/reader/disc.c: Make filenames FAT32-friendly (1d82059)

# 21.3.28

 - Makefile: install into lib64 if it exists (f120146)

# 21.3.22

 - reader/sacd.h: Fix MasterToc packing (22b20d7)
 - reader/disc.c: Fix reading raw blocks with sector size of 2064 (f7808f3)

# 21.1.19

 - Fix internal track count (d56c8a1)
 - Update Debian changelog (96464c6)
 - Try to use the album artist if track performer is missing (f4a1beb)
 - Make sure the track artist is never NULL (73b95dc)
 - Fix missing de-referencing for some pointers (1d58b58)
 - Add binary dir (45fd2eb)
 - Version bump (7bd8025)
 - Initial port from Launchpad (c6e2589)

