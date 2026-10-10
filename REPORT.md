Q1. stat() vs lstat()

stat() follows a symbolic link and gives information about the file it points to.
lstat() gives information about the link itself.
ls -l should use lstat(). It needs to show a symlink as type l, with the link’s own size and date. If it used stat(), it would report the target’s details and couldn’t tell the entry was a link.

Q2. st_mode and bitwise operators

st_mode holds both the file type and the permission bits in one integer.
For the type, AND it with S_IFMT and compare the result, for example (st_mode & S_IFMT) == S_IFDIR. The macros S_ISDIR() and S_ISREG() do this for you.
For permissions, AND with one bit macro such as S_IRUSR. A non-zero result means that permission is set, so print r, otherwise -. The same works for S_IWUSR and S_IXUSR, and for the group (S_IRGRP...) and others (S_IROTH...) macros.
The special bits S_ISUID, S_ISGID and S_ISVTX are tested the same way. They change the x position to s, S, t or T.     
Feature 3, Q1. “Down then across” logic

All filenames are first stored in an array. With num_cols = terminal_width / (max_len + 2) and num_rows = ceil(total_files / num_cols), the files are laid out so each column is filled from top to bottom before moving to the next column.
The program prints row by row. For row r and column c, the item is filenames[r + c * num_rows]. So row 0 prints filenames[0], filenames[num_rows], filenames[2*num_rows], and so on.
A single loop is not enough because it would print filenames[0], filenames[1], filenames[2] one after another, which gives either one column or an “across then down” layout. The next item on the same row is not the next array element but one that is num_rows positions further on, so you need a nested loop (rows, then columns) with an index calculation. You also need to know the total count and longest name before printing anything.

Feature 3, Q2. ioctl and the fixed-width fallback

ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) asks the terminal driver for the current window size, so the program gets the actual number of columns (ws.ws_col) and can fit as many columns as the window allows.
With only a fixed 80-column width, the output would not adapt. In a wider terminal, space would be wasted and the listing would be longer than needed. In a narrower terminal, lines longer than the window would wrap and break the alignment. Resizing the window would have no effect on the layout.
