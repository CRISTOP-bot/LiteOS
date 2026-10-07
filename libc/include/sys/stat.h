#ifndef LITEOS_SYS_STAT_H
#define LITEOS_SYS_STAT_H

/* Mode bits present in the newc initramfs; stat() is not exposed yet. */
#define S_IFMT   0170000
#define S_IFREG  0100000
#define S_IFDIR  0040000
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)

#endif
