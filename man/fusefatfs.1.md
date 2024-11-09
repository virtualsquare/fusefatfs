<!--
.\" Copyright (C) 2020 VirtualSquare. Project Leader: Renzo Davoli
.\"
.\" This is free documentation; you can redistribute it and/or
.\" modify it under the terms of the GNU General Public License,
.\" as published by the Free Software Foundation, either version 2
.\" of the License, or (at your option) any later version.
.\"
.\" The GNU General Public License's references to "object code"
.\" and "executables" are to be interpreted as the output of any
.\" document formatting or typesetting system, including
.\" intermediate and printed output.
.\"
.\" This manual is distributed in the hope that it will be useful,
.\" but WITHOUT ANY WARRANTY; without even the implied warranty of
.\" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
.\" GNU General Public License for more details.
.\"
.\" You should have received a copy of the GNU General Public
.\" License along with this manual; if not, write to the Free
.\" Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
.\" MA 02110-1301 USA.
.\"
-->

# NAME

fusefatfs, vufusefatfs - mount FAT file systems using FUSE and vufuse

# SYNOPSIS

`fusefatfs` [`-hVdfs`] [`-o` _options_ ] *disk_image* *mountpoint*

in a `umvu` session:

`mount -t vufusefatfs` [`-o` _options_ ] *disk_image* *mountpoint*

# DESCRIPTION

`fusefatfs` mounts the file tree contained in *disk_image* on the directory *mountpoint*.
It supports FAT12, FAT16, FAT32 and exFAT formats.

`vufusefatfs` is the VUOS/vufuse submodule of `fusefatfs`

# OPTIONS

`fusefatfs` is build upon FUSE (Filesystem in Userspace) library.
the  complete  set  of available options depends upon the specific
FUSE installed.  Execute `fusefatfs -h` to retrieve the actual complete
list.

### general options

  `-o` opt,[opt...]
: FUSE and file specific mount options.

  `-h`
: display a usage and options summary

  `-V` &nbsp; `--version`
: display version

### fusefatfs specific options

  `-o ro`
: mount the file system in read-only mode.

  `-o rw`
: mount the file system in read-write mode only if also `-o force` is present.
: Read-write mounting can potentially manage the file system structure so an
: extra option is required to test the awareness fo the user.

  `-o force`
: confirm the request of read-write access.

  `-o rw+`
: mount the file system in read-write mode, a shortcut of `-o rw,force`.

### main FUSE mount options

  These options are not valid in VUOS/vufuse.

  `-d` &nbsp; `-o debug`
: enable debug output (implies -f)

  `-f`
: foreground operation

  `-s`
: disable multi-threaded operation

# SEE ALSO
`fuse`(8), `umvu`(1)

# CREDITS
`fusefatfs` is based on the FAT file system module for embedded sytems `fatfs` 
developed by ChaN:
*http://elm-chan.org*.

# AUTHOR
VirtualSquare. Project leader: Renzo Davoli.

