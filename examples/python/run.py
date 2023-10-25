import sys
import lv2_horst

ps = lv2_horst.lv2_plugins()
jh = lv2_horst.jacked_horst(plugins, sys.argv[1])

