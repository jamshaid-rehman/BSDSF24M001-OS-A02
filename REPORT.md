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
Feature 4, Q1. Vertical vs horizontal complexity

The “down then across” (vertical) layout is more complex. Before printing anything you must know the total number of files and the longest name. You then calculate the number of columns and the number of rows (ceil(count / cols)), and every item’s position depends on that row count, using the index row + col * num_rows. This needs a nested loop and careful handling of the last, partly filled column.
The “across” (horizontal) layout needs very little pre-calculation. You only need the column width (longest name plus spacing) and the terminal width. The names are printed in their original order, and you keep a running position counter. When the next name would not fit on the line, you print a newline and reset the counter. No row or column counts are needed.
So the vertical layout needs more pre-calculation, because the position of each item depends on the number of rows, which depends on everything else.

Feature 4, Q2. Managing display modes

I used an enum display_mode with three values: MODE_DEFAULT, MODE_LONG and MODE_HORIZONTAL. It starts as MODE_DEFAULT.
In main, the getopt() loop sets the mode: -l sets MODE_LONG and -x sets MODE_HORIZONTAL. If both are given, the last one wins, as in the real ls.
The mode is passed to do_ls(). If the mode is MODE_LONG, it calls do_ls_long(). Otherwise it reads all filenames into a dynamic array, then checks the mode: MODE_HORIZONTAL calls print_horizontal(), and anything else calls print_vertical(), the default “down then across” display.
 
Feature 5, Q2. The qsort() comparison function

Purpose: qsort() is generic. It can sort any array but doesn’t know the element type, so it calls my function to decide which of two elements comes first.
Signature: int compare_names(const void *a, const void *b)
How it works: qsort() passes pointers to two array elements. Each element is a char *, so each argument is really a char **. The function casts a and b to const char * const *, dereferences them to get the two strings, and returns strcmp(s1, s2). The result is negative if the first string comes first, zero if they are equal, and positive if the first comes after.
Why const void *: qsort() must work with any data type, and void * is C’s generic pointer. const promises that the comparison will not modify the elements being compared. The function pointer type has to match exactly what qsort expects, so using any other argument types would not be valid.
