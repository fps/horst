import horst
import time 

h = horst.horst()

uri = "http://fps.io/plugins/clipping.tanh"

p = h.lv2_unit(uri, "p", False)

def idx(h, p, name):
  return h.get_port_index(p, name)

b = horst.midi_binding(True, 0, 0)
h.set_midi_binding(p, idx(h, p, "pregain"), b)

cs = horst.connections()
cs.add("system:capture_1", "p:in")
cs.add("p:out", "system:playback_1")
cs.add("p:out", "system:playback_2")
h.connect(cs)

