# C helper roadmap

This branch is experimenting with small C helpers for BigRatOS rat package tools.

## Current helpers

- `rat-version` - prints rat package tool version/help
- `rat-info-c` - read-only package info helper
- `rat-search-c` - read-only repository search helper

## Current build commands

    make
    make test
    make clean

## Safety plan

The C helpers are currently separate from the main shell tools.

Read-only tools are being tested first:

1. version/help helper
2. package info helper
3. package search helper

Install/remove logic such as `rat-add` and `rat-remove` should be studied carefully before any C porting attempt.

## Notes

This branch avoids replacing the existing shell scripts until the C helpers are tested more.
