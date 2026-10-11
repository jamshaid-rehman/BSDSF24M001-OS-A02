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

Feature 6, Q1. How ANSI escape codes work

ANSI escape codes are special character sequences that the terminal interprets as commands instead of text to display. Each one begins with the escape character (ESC, octal \033) followed by [, then one or more numbers, then a letter. The letter m means “set text attributes (graphics mode)”.
The number sets the colour or style: 30 to 37 are the colours (31 red, 32 green, 34 blue, 35 magenta) and 7 is reverse video. 0 resets everything.
To print text in green you print the start code, the text, and then the reset code:
c
  printf("\033[0;32m%s\033[0m\n", "hello");

\033[0;32m switches the terminal to green, and \033[0m returns it to the default colour. Without the reset, all later output would stay green.

These codes take up bytes but no screen space, so alignment must be calculated using the visible length of the name, not the length of the coloured string.

Feature 6, Q2. Checking whether a file is executable

The execute permission is stored in three separate bits in st_mode:
S_IXUSR (0100): executable by the owner
S_IXGRP (0010): executable by the group
S_IXOTH (0001): executable by others
To test one, AND st_mode with the macro: if (st_mode & S_IXUSR) is true when the owner can execute the file. To check whether anyone can execute it, use st_mode & (S_IXUSR | S_IXGRP | S_IXOTH), which is what my get_color() does.
Directories also have the execute bit set (it means “can enter the directory”). So I check for symlinks and directories first and apply the green colour only after the other types have been ruled out.
