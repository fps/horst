import horsting as h
import time 

units = [h.lv2(uri) for uri in [
    "http://calf.sourceforge.net/plugins/Gate",
    "http://calf.sourceforge.net/plugins/Compressor",
    "http://fps.io/plugins/clipping.tanh",
    "http://calf.sourceforge.net/plugins/EnvelopeFilter",
    "http://fps.io/plugins/state-variable-filter-v2",
    "http://guitarix.sourceforge.net/plugins/gxts9#ts9sim",
    "http://moddevices.com/plugins/mod-devel/BigMuffPi",
    "http://fps.io/plugins/state-variable-filter-v2",
    "http://guitarix.sourceforge.net/plugins/gx_cabinet#CABINET", 
    "http://drobilla.net/plugins/mda/Leslie",
    "http://calf.sourceforge.net/plugins/MultiChorus",
    "http://calf.sourceforge.net/plugins/Phaser",
    "http://calf.sourceforge.net/plugins/Reverb",
    "http://calf.sourceforge.net/plugins/Reverb",
    "http://drobilla.net/plugins/mda/DubDelay",
    "https://ca9.eu/lv2/bolliedelay",
    "http://calf.sourceforge.net/plugins/Saturator",
    "http://calf.sourceforge.net/plugins/Limiter",
  ]]

h.connect(h.system, *units, h.system)
