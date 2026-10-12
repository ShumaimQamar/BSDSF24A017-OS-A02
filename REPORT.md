
## Feature 2 Report Questions

### 1. Difference between stat() and lstat()
The `stat()` system call follows symbolic links and returns metadata about the target file that the link points to. In contrast, `lstat()` does NOT follow symbolic links; it returns metadata about the symbolic link file itself. For `ls`, `lstat()` is more appropriate because `ls` needs to display information about the link file itself (such as link size and link permissions) rather than the target file it points to.

### 2. Extracting file type and permissions using bitwise operators
The `st_mode` field in `struct stat` is a bitmask containing file type and permission flags. 
- Bitwise AND (`&`) with bitwise masks/macros is used to inspect specific bits.
- File types are extracted using macros like `S_ISDIR(mode)` or `(mode & S_IFMT) == S_IFDIR`.
- Permissions are checked using predefined bitwise masks, for example: `mode & S_IRUSR` checks if the owner has read permissions, and `mode & S_IXGRP` checks group execution permissions.

## Feature 3 Report Questions

### 1. General logic for printing items in a "down then across" columnar format
In "down then across" order, items are filled vertically column by column. A single sequential loop fails because standard terminal output prints line by line (horizontally row by row). Therefore, to print row `r`, we must calculate and jump across the array to access elements from each column for that row using `index = col * total_rows + row`, padding each item with spaces up to the column width.

### 2. Purpose of ioctl system call and limitations of fixed-width fallback
The `ioctl()` system call with `TIOCGWINSZ` queries the terminal driver to obtain the current terminal window dimensions (`ws_col`). Without it, relying solely on an 80-column fallback would cause output wrapping or ugly truncation when terminal windows are widened or narrowed by the user.
