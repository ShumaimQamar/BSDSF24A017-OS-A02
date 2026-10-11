
## Feature 2 Report Questions

### 1. Difference between stat() and lstat()
The `stat()` system call follows symbolic links and returns metadata about the target file that the link points to. In contrast, `lstat()` does NOT follow symbolic links; it returns metadata about the symbolic link file itself. For `ls`, `lstat()` is more appropriate because `ls` needs to display information about the link file itself (such as link size and link permissions) rather than the target file it points to.

### 2. Extracting file type and permissions using bitwise operators
The `st_mode` field in `struct stat` is a bitmask containing file type and permission flags. 
- Bitwise AND (`&`) with bitwise masks/macros is used to inspect specific bits.
- File types are extracted using macros like `S_ISDIR(mode)` or `(mode & S_IFMT) == S_IFDIR`.
- Permissions are checked using predefined bitwise masks, for example: `mode & S_IRUSR` checks if the owner has read permissions, and `mode & S_IXGRP` checks group execution permissions.
