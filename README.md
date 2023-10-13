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
import time

# Import the generated python bindings directly
import lv2_horst as h

# Scan the system for lv2 plugins
plugins = h.lv2_plugins()

# And create a jack client for a particular plugin
horsted = h.jacked_horst(plugins, "http://calf.sourceforge.net/plugins/Reverb")

# Connect the input and output ports to system ports
cm = h.connection_manager()
cm.connect([
    ("system:capture_1", "Calf Reverb:in_l"),
    ("system:capture_2", "Calf Reverb:in_r"),
    ("Calf Reverb:out_l", "system:playback_1"),
    ("Calf Reverb:out_r", "system:playback_2")
])

print("Ports:")

# Print the names of all ports
for n in range(0, horsted.get_number_of_ports()):
    print(f"index: {n}, port: {horsted.get_port_properties(n).name}, value: {horsted.get_control_port_value(n)}")

# Set a particular control port value
horsted.set_control_port_value(11, 0.1)

# Enable control output updates so the following outputs values
horsted.set_control_output_updates_enabled(True)

print("Reading out port values:")

# Read out port values
for n in range(0,100):
    print(f"index: {19}, name {horsted.get_port_properties(19).name}, {horsted.get_control_port_value(19)}, index: {20}, name {horsted.get_port_properties(20).name},  {horsted.get_control_port_value(20)}")
    time.sleep(0.1)
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
