import lv2_horsting as h
import sys
import time

# plugins = h.lv2_plugins()
units = [h.plugin(uri) for uri in sys.argv[1:]]

h.connect(h.system, *units, h.system)
