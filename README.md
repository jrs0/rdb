[![CMake Build/Test](https://github.com/jrs0/rdb/actions/workflows/cmake.yml/badge.svg)](https://github.com/jrs0/rdb/actions/workflows/cmake.yml)
[![R-CMD-check](https://github.com/jrs0/rdb/actions/workflows/R-CMD-CHECK.yaml/badge.svg)](https://github.com/jrs0/rdb/actions/workflows/R-CMD-CHECK.yaml)

**This code is an early prototype.** Test coverage is quite sparse, and building has only properly been tested on Windows (using the R build toolchain) and Ubuntu, and the library is not in an easily usable state.

# Spells/Episodes Pre-processing Library

This repository contains a preprocessing library for cleaning the data in Hospital Episode Statistics tables, organising it into spells of episodes, and counting the occurances of different categories of ICD-10 and OPCS-4 codes in each patient's spells. 

One of the main goals of the C++ library is to parse ICD-10 in a way that is immune to code errors in the tables. There are two types of errors:
- Codes that are invalid, such as N18.0, D46.3, M72.58
- Codes that are valid, but in a non-standard format, such as I210 instead of I21.0

Many codes that are invalid contain a valid root part, with incorrect trailing material. For example, A09.X is invalid[^1] (because A09 has sub-categories), but it most likely means "some unknown code in the A09 category". Since the root A09 is a valid code, the trailing matter .X can be ignored, and the code can be interpreted as a partial code meaning "some code in this category".

[^1]: The national clinical coding standards state "Where a three character category code is not subdivided into four character subdivisions the ‘X’ filler must be assigned in the fourth character field so the codes are of a standard length for data processing and validation. The code is still considered a three character code from a classification perspective". A09 has subcategories, so X cannot be used.

Codes that are invalid such as D46.3 cannot easily be fixed, because it is not know what valid number (e.g. 2 or 4) the 3 should have been, although they could be treated the same way as above (i.e. as some code in the category D46).

This repository contains code that identifies valid/invalid codes using the following procedure:
1. Strip all leading and trailing whitespace from the code
2. Remove dots (.) from the code
3. Compare the code with all valid codes that exist (input to the program as )

The final step ensures that codes which are invalid are always identified. It is difficult to shortcut an exhaustive search over valid codes. For example, it is not possible to write a simple regular expression for valid codes, because of the gaps in the code listings (for example, many code categories allow 0, 8 and 9 in the final position, but some do not).

The main code parsing programs is written is C++, and uses a binary search tree to identify whether a given code is valid. Valid codes are cached, to improve the lookup speed for commonly occurring codes. The code performs adequately well for large tables (parsing about 10,000,000 episode rows, each containg about 50 codes that need looking up, takes about 15 minutes). There is scope for further optimisation.

Both ICD-10 and OPCS-4 codes are treated in the same way -- the only difference is the input code definition file (listing valid ICD-10 and OPCS-4 codes).

Currently, codes are only allowed if they exactly match a valid code (i.e. none of the logic described above for reinterpreting A09.X or D46.3 is performed). This choice optimises for correctness of codes, at the expense of including some codes that could probably be interpreted correctly.

## Windows Development

You must currently install from source. First, obtain R 4.3, and install devtools as follows:

```r
install.packages("devtools")
```

Next, install the C++ library dependencies by installing Rtools43. Open `Rtools43 Bash` and run

```bash
pacman -Sy mingw-w64-x86_64-unixodbc mingw-w64-x86_64-yaml-cpp
```

The other two are library dependencies for this package. If you installed Rtools43 in a custom location (not `c:\rtools43`), you must add the following line to your `.Renviron` (create it wherever `path.expand(~)` returns). For example:

```bash
PATH=c:\\Users\\your.name\\rtools43\\x86_64-w64-mingw32.static.posix\\bin;c:\\Users\\your.name\\rtools43\\usr\\bin;${PATH}
```

This will allow R to find the right toolchain (and the libraries installed above).

## Linux Development

On a blank Ubuntu 22.04 operating system (e.g. `docker run -it ubuntu`), install the following:

```bash
## Leave out the sudo on docker
sudo apt install gcc g++ cmake odbcinst unixodbc unixodbc-dev libyamlcpp-dev
```

Install ODBC Driver 18 as described [here](https://learn.microsoft.com/en-us/sql/connect/odbc/linux-mac/installing-the-microsoft-odbc-driver-for-sql-server?view=sql-server-ver15&tabs=ubuntu18-install%2Calpine17-install%2Cdebian8-install%2Credhat7-13-install%2Crhel7-offline):

```bash
sudo apt install curl gnupg
curl https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
curl https://packages.microsoft.com/config/ubuntu/22.04/prod.list > /etc/apt/sources.list.d/mssql-release.list
sudo apt-get update
sudo ACCEPT_EULA=Y apt-get install -y msodbcsql18
```

Make a file of credentials `db.secret.yaml` with the following format:

```yaml
driver: "ODBC Driver 18 for SQL Server"
server: "server_hostname"
uid: "username"
pwd: "password"
```

### Building 

To compile the project, run the following from the src/ directory

```bash
mkdir build/
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . 
```

You will now have a main executable in the build folder. 

### Profiling

To get the profile information from gprof, run (from the top level of the repository)
 

```bash
./src/build/main
gprof -b ./src/build/main gmon.out > profile.txt
```

To use perf, follow these steps

```bash
# Pick the package for your kernel
apt install linux-tools-generic linux-tools-5.15.0-60-generic
```

Then run the following command to profile the program:

```bash
perf record -g /src/build/main
perf script > out.perf
```

(Note: if you are using a docker container, you will need to make sure you pass --privileged when you create it. If you already have a container, commit the container to an image and recreate it with the --privileged flag.), Next, create a flame graph by cloning [this](https://github.com/brendangregg/FlameGraph) repository, and follow the instructions:


```bash
./stackcollapse-perf.pl out.perf > out.folded
./flamegraph.pl out.folded > kernel.svg
```


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

To compile this properly (during development, with optimisations turned on), run this:

```bash
pkgbuild::compile_dll(debug=FALSE)
```

This will remove the -O0 flags. You may also need to do a load_all afterwords to get it to use the updated functions (the key is not making any cpp changes in between so that load_all does not recompile).

## Profiling

### On linux


```bash
sudo apt install unixodbc-dev libyaml-cpp-dev
```

You can profile using kcachegrind by running 

```bash
R -d "valgrind --tool=callgrind" -f script.R
```

### MSYS attempt to make gperftools work

*This did not work, due to the lack of libprofiler. However, the notes are here for posterity anyway*.

It is preferable to use the MSYS installation that is bundled with R installations on windows (look for a program like `Rtools42 Bash`). First, install some prerequisities for building:

```bash
pacman -Syuu
pacman -S base-devel mingw-w64-x86_64-toolchain libtool
```

Next, download and build gperftools from source:

```bash
# Clone the gperftools repository, and build it as follows
# (see the INSTALL folder)
git clone https://github.com/gperftools/gperftools.git
cd gperftools


# As per the documentation, should "just work" on msys
./autogen.sh
./configure

# The configure step determines not to build libprofiler, with
# the following error:
#
# 'configure: WARNING: Could not find the PC.
#  Will not try to compile libprofiler...'
#
# This means it is not possible to use ProfilerStart, ProfilerStop.
# However, the pprof tool will still be available, so the only option
# is to profile the full Rscript process.

make
make install

# The make install step gives the following warning:
#
# 'libtool: warning: undefined symbols not allowed in
#  x86_64-pc-msys shared libraries; building static only'
#
# The make install fails with linker errors to
# tcmalloc::* functions. However, the pprof executable
# is built successfully, at src/pprof.

# To manually install the pprof executable, run
mkdir /usr/local/bin
cp src/pprof /usr/local/bin

# To check the install works, run
pprof -v
```

To profile Rcpp code, you need to use the MSYS shell. R is not present by default in that environment, so install it

### Using gperf with MSYS

If you follow the steps above (installing binutils), you will have gprof already:

```bash
gprof -v
```

Compiling and linking with -pg does work, and Rscript invocations attempt to write to gmon.out, but get permission denied. It is unclear where the process is attempting to write the file -- strace shows not chdir from the (writeable) working directory. 

## Other misc issues

If you are having issues with rmarkdown finding pandoc or texlive (in emacs), you may need to add the path to either of them in your `~/.Renviron`:

```bash
PATH="c:/texlive/2022/bin:${PATH}"
# Replace with whatever Sys.getenv("RSTUDIO_PANDOC") returns in rstudio
RSTUDIO_PANDOC="c:/Users/Administrator/AppData/Local/Programs/RStudio/bin/quarto/bin/tools"
```

Go careful trying to debug a c++ function through an R call -- load_all does not know how to reload R functions properly that depend on a c++ function, so you can be changing the c++ and wondering why nothing is happening -- either call the c++ function directly to debug, or make a change to this file to force it to reload.

## Python development

To use pybind (in Windows, VS Code), create a python virtual environment, and run:

```bash
pip install pybind11
```

## Windows Development Take 2

Based on [this page](https://github.com/HO-COOH/CPPDevOnWindows). Install MSYS2 and put it in the default place `C:\msys64`. In the `MSYS2 (MSYS)` terminal, run

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-gdb
pacman -S mingw-w64-x86_64-python
pacman -S mingw-w64-x86_64-python-pip
```

If you can add `C:\msys64\mingw64\bin` to your path, do it. If not (due to lack of admin rights), there are alternative methods to fix the path. To temporarily fix the path inside a given MSYS2 terminal, run `export PATH=/mingw64/bin:$PATH` (the change will not persist, but you can use this to check what programs are present). Do not use the binaries in `C:\msys64\bin`; the compiler there does not produce native binaries for windows.

You can compile pybind11 fine with cmake inside MSYS2. It will find the native-windows python installation (if it exists -- the one at, e.g., `c:/users/your.user/AppData/Local/Programs/Python/Python311`), provided you use the following line in CMake (as explained in the pybind documentation):

```cmake
find_package(Python 3.6 COMPONENTS Interpreter Development REQUIRED)
```

The resulting dll for the module `example` is called `example.cp311-win_amd64.pyd`. It is not `example.cp310-mingw_x86_64.pyd`; if you get that, then cmake incorrectly used the MSYS2-distributed python.

You should then be able to load the module `example` from the native-windows installation of python. To test, navigate to the same directory as the `.pyd` file and run python. You will need to add the MSYS2 shared library path to the dll search path, otherwise python will complain about DLLs not found:

```python
import os
os.add_dll_directory("C:/msys64/mingw64/bin")
import example
```

If you still get missing DLL errors, use [this tool](https://github.com/lucasg/Dependencies) to look at the DLL dependencies of `example.cp311-win_amd64.pyd`, and add any paths that are missing using `os.add_dll_directory`.