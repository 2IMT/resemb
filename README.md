# resemb

Tool to bake assets into your executable.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Usage](#usage)
    - [Installation](#installation)
    - [Basic Usage](#basic-usage)
    - [CMake Integration](#cmake-integration)
    - [Using Without CMake](#using-without-cmake)
- [API Overview](#api-overview)
- [CLI Overview](#cli-overview)
- [License](#license)

## Overview

`resemb` is a lightweight asset embedding tool for C projects. It allows you to
bake binary and text assets directly into your executable or static library,
removing the need to ship external files. Ideal for small games, demos, and
single-binary utilities.

## Features

- Embeds any file (binary or text) into your program
- Zero runtime dependencies (no `libc` required)
- Minimal API
- Cross-platform (works on any C99 compiler)
- Optional CMake integration for automatic regeneration

## Usage

This tool supplies a `resemb_gen` utility, and `resemb.h` header with interface
to access the baked data. To use the library, you will need to generate a
`resemb` implementation source file using `resemb_gen`, link the generated
source to your application, and access the data using the interface defined in
`resemb.h`.

The generated implementation has no dependecies, even `libc` is not
needed for runtime. But note that you need `libc` to run `resemb_gen`.

### Installation

```bash
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

### Basic Usage

```c
#include <stdio.h>

#include <resemb/resemb.h>

int main(void) {
  ResembRes res;
  if (!resemb_find(&res, "assets/text.txt")) return 1;
  printf("assets/text.txt:\n%s", res.data);
  return 0;
}
```

Building your application:

```bash
# Generate `resemb` implementation with baked assets
resemb_gen -o resemb_impl.c "assets/text.txt" "assets/player.png" "assets/music.mp3"

# Build your executable and link the `resemb` implementation
gcc -o main main.c resemb_impl.c
```

This will install `resemb.h` header and `resemb_gen` utility.

### CMake Integration

```cmake
cmake_minimum_required(VERSION 3.12)
project(my_app)

add_executable("main.c");

find_package(resemb REQUIRED)

resemb_embed(
    TARGET resemb_test
    ASSETS "assets/text.txt" "assets/player.png"
    STRIP_PREFIX "assets/" # Optional, strips prefixes from resource names
)
```

This will automatically regenerate and link the `resemb` implementation every
time any of the assets changes.

### Using Without CMake

`resemb`'s build process is extremely simple:

```bash
# Build `resemb_gen`
gcc -o resemb_gen src/resemb_gen.c

# Generate `resemb` implementation with baked assets
./resemb_gen -o resemb_impl.c "assets/text.txt" "assets/player.png" "assets/music.mp3"

# Build your executable and link the `resemb` implementation
gcc -o main main.c resemb_impl.c
```

## API Overview

```c
typedef struct {
  unsigned int id;
  const char* name;
  const unsigned char* data; // null-terminated
  unsigned int len;          // excludes null terminator
} ResembRes;

int resemb_find(ResembRes* res, const char* name); // find resource by name
int resemb_get(ResembRes* res, unsigned int id); // get resource by id
ResembRes resemb_get_unchecked(unsigned int id); // get resource by id (no bound-checks)

const char* const* resemb_list(void); // get the array of all resource names
unsigned int resemb_count(void); // get the total number of resources
unsigned int resemb_size(void); // get the total size the resources occupy
```

For the full API documentaion refer to [`resemb.h`](include/resemb/resemb.h)

## CLI Overview

```bash
# Print help text
resemb_gen --help

# Generate to stdout
resemb_gen assets/foo assets/bar

# Generate to file
# This will generate to `resemb_impl.c` file
resemb_gen -o resemb_impl.c assets/foo assets/bar

# Strip prefixes
# This will strip `assets/` prefix from all resource names
resemb_gen -o resemb_impl.c -s assets/ assets/foo assets/bar
```

## License

MIT License. See [LICENSE](LICENSE) for details.
