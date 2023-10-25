import time

# Import the generated python bindings directly
import lv2_horst as h

# And the convenience library as well
import lv2_horsting as hg

from lv2_horsting import system
from lv2_horsting import connect

# And create a jack client for a particular plugin
plugin = hg.plugin("http://calf.sourceforge.net/plugins/Reverb")

# Connect the input and output ports to system ports
connect(system, plugin, system)

plugin.dry_ = 0
plugin.amount_ = 1.0

# Enable control output updates so the following outputs values
plugin.set_control_output_updates_enabled(True)

print("Reading out port values:")

# Read out port values
for n in range(0,1):
    print(f"index: {19}, name {plugin.get_port_properties(19).name}, {plugin.get_control_port_value(19)}, index: {20}, name {plugin.get_port_properties(20).name},  {plugin.get_control_port_value(20)}")
    time.sleep(0.1)
