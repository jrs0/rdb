# Faster data preprocessing

The goal of this repository is to test out the idea of loading data in C++, doing the preprocessing in C++, and then passing it to R for the rest.


## Windows Development

You must currently install from source. First, obtain R 4.3, and install devtools as follows:

```r
install.packages("devtools")
```

Next, install the C++ library dependencies by installing Rtools43. Open `Rtools43 Bash` and run

```bash
pacman -Sy mingw-w64-x86_64-toolchain mingw-w64-x86_64-unixodbc mingw-w64-x86_64-yaml-cpp
```

The `toolchain` gets you basic build tools (make, etc.). The other two are library dependencies for this package. If you installed Rtools43 in a custom location (not `c:\rtools43`), you must add the following line to your `.Renviron` (create it wherever `path.expand(~)` returns):

```bash
PATH=c:\\Users\\john.scott\\rtools43\\x86_64-w64-mingw32.static.posix\\bin:${PATH}
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
