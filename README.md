# fusefatfs
mount FAT file systems using FUSE anf VUOS/vufuse

This repository generates both the `fusefatfs` command for 
[FUSE (Filesystem in Userspace)](https://github.com/libfuse/libfuse)
and the plug-in sumbodule for [VUOS](https://github.com/virtualsquare/vuos).

It supports FAT12, FAT16, FAT32 and exFAT formats.

## Example (FUSE):
Mount the diskimage `/tmp/myfatimage` on `/tmp/mnt`
```
mkdir /tmp/mnt
fusefatfs -o ro /tmp/myfatimage /tmp/mnt
```

then umount it using:
```
fusermount -u /tmp/mnt
```

Read write access is enabled using the option `-o rw+` or `-o rw,force`.
It is warmly suggested to create a backup copy of the disk image, especially if
the disk image contains valuable data.

## Example (VUOS/vufuse)

Start a umvu session. Then load the `vufuse` module:
```
vu_insmod vufuse
```
and mount your disk image:
```
vumount -t vufusefatfs -o ro  /tmp/myfatimage /tmp/mnt
```

then umount it using:
```
vuumount /tmp/mnt
```

## Credits:

`fusefatfs` is based on the FAT file system module for embedded systems [fatfs](http://elm-chan.org/fsw/ff/00index_e.html) 
by ChaN.

## VirtualSquare

This is a [VirtualSquare](http://wiki.virtualsquare.org) project!
