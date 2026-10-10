Q1. stat() vs lstat()

stat() follows a symbolic link and gives information about the file it points to.
lstat() gives information about the link itself.
ls -l should use lstat(). It needs to show a symlink as type l, with the link’s own size and date. If it used stat(), it would report the target’s details and couldn’t tell the entry was a link.

Q2. st_mode and bitwise operators

st_mode holds both the file type and the permission bits in one integer.
For the type, AND it with S_IFMT and compare the result, for example (st_mode & S_IFMT) == S_IFDIR. The macros S_ISDIR() and S_ISREG() do this for you.
For permissions, AND with one bit macro such as S_IRUSR. A non-zero result means that permission is set, so print r, otherwise -. The same works for S_IWUSR and S_IXUSR, and for the group (S_IRGRP...) and others (S_IROTH...) macros.
The special bits S_ISUID, S_ISGID and S_ISVTX are tested the same way. They change the x position to s, S, t or T.
