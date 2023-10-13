# Import the generated python bindings directly
import lv2_horst as h

# Scan the system for lv2 plugins
plugins = h.lv2_plugins()

# And create a jack client for a particular plugin
horsted = h.jacked_horst(plugins, "http://calf.sourceforge.net/plugins/Reverb")
