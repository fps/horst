import time

# Import the generated python bindings directly
import lv2_horst as h

# Scan the system for lv2 plugins
plugins = h.lv2_plugins()

# And create a jack client for a particular plugin
horsted = h.jacked_horst(plugins, "http://calf.sourceforge.net/plugins/Reverb")

# Connect the input and output ports to system ports
cm = h.connection_manager()
# cm.connect([
#     ("system:capture_1", "Calf Reverb:in_l"),
#     ("system:capture_2", "Calf Reverb:in_r"),
#     ("Calf Reverb:out_l", "system:playback_1"),
#     ("Calf Reverb:out_r", "system:playback_2")
# ])

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
