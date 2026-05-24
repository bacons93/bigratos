# BigRatOS rat package tools

Early shell-based package manager tools for BigRatOS.

## Scripts

- `rat-add` - fetches, builds, and installs a package
- `rat-remove` - removes an installed package
- `rat-search` - searches available packages
- `rat-info` - shows package information
- `rat-update` - updates package/repo info

## Experimental C helpers

Some experimental C helpers are in `c-src/`.

Current helpers:

- `rat-version`
- `rat-info-c`
- `rat-search-c`

Build them with:

    make

Run examples:

    ./rat-version --help
    ./rat-info-c --help
    ./rat-search-c --help

Clean built binaries with:

    make clean

## Note

This project is early and should be tested in a VM or chroot before real hardware.
