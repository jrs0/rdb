# Faster data preprocessing

The goal of this repository is to test out the idea of loading data in C++, doing the preprocessing in C++, and then passing it to R for the rest.

## Notes

You might need to install some packages using MSYS2 -- here is the cheat sheet (so far). Open Rtools42Bash (MSYS2) and use the following commands:

```bash
# Update the repository
pacman -Syyu
# Search for a particular package (link -lodbc32)
pacman -Ss unixodbc
# Install a package (the R toolchain uses mingw)
pacman -S mingw-w64-x86_64-unixodbc

# Other dependencies
pacman -S mingw-w64-x86_64-yaml-cpp
```
