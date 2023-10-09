# lv2-horst

A simple C++ library implementing an LV2 plugin host exposing plugins as jack clients. Python bindings are provided (these are actually the sole reason for the existance of `lv2-horst`).

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

You can find examples of using lv2-horst in the examples directory. But just to get a flavor here are two examples:

## `lv2_horst`

```python
import sys
import lv2_horst

world = lv2_horst.lilv_world()
plugins = lv2_horst.lilv_plugins(world)

plugin = lv2_horst.plugin(world, lv2_horst.lilv_plugin(plugins, lv2_horst.lilv_uri_node(world, sys.argv[1])))

instance = lv2_horst.jack_plugin_horst(plugin)
```

## `lv2_horsting`

```python
import sys
import lv2_horsting as h

p = h.instantiate(sys.argv[1])
```

This immediately looks a lot simpler than the low-level example above.

# Development scripts

The `dev/` folder contains some scripts that might be useful.

# Hacking

The file `src/lv2_horst.cc` contains the `pybind11` bindings for the library. It gets compiled to `lv2_horst.so`.
