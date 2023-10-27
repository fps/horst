import sys
import lv2_horst

plugins = lv2_horst.plugins()
plugin = lv2_horst.jacked_horst(plugins, sys.argv[1])

