# lv2-horst

A simple header-only C++ library implementing an LV2 plugin host exposing plugins as jack clients. Python bindings are provided (these are actually the sole reason for the existance of `lv2-horst`).

# Requirements

Check the `nix-shell.sh` file. It should list up-to-date dependencies. It possibly contains some extra things which are not strictly necessary.

# Usage

Since packaging python-extensions is a horrible experience I chose not to do that. The python extension library is installed to `$PREFIX/lib/horst`. So add `$PREFIX/lib/horst` to your `PYTHONPATH`.

## The `lv2_horst` and `lv2_horsting` modules

# `lv2_horst`

`lv2_horst` is the low-level module implemented in C++

# `lv2_horsting`

`lv2_horsting` is a python module implementing convenience functionality. This is the module you usually want to import.

# Examples

You can find examples of using lv2-horst in the examples directory.

# Development scripts

The `dev/` folder contains some scripts that might be useful.

# Hacking

The file `src/lv2_horst_python.cc` contains the `pybind11` bindings for the library. It gets compiled to `lv2_horst.so`.
