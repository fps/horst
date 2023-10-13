#!/usr/bin/env/python3

import lv2_horst as h

import weakref
import subprocess
import re
from collections import namedtuple

lv2_plugins = h.lv2_plugins()

def string_to_identifier(varStr): return re.sub('\W|^(?=\d)','_', varStr)

class uris_info:
  def __init__(self):
    self.uris = lv2_plugins.get_uris()
    self.identifiers = list(map(string_to_identifier, self.uris))
    self.identifiers_to_uris = {}

    for uri in self.uris:
      self.identifiers_to_uris[string_to_identifier(uri)] = uri

  def __dir__(self):
    return list(self.__dict__.keys()) + self.identifiers

  def __getattr__(self, name):
    if self.identifiers_to_uris[name]:
      return self.identifiers_to_uris[name]

uris = uris_info()

class props:
  def __init__(self, unit, index):
    self.unit = weakref.ref(unit)
    self.p = unit.get_port_properties(index)
    self.p.index = index

  def __getattr__(self, name):
    return getattr(self.p, name)

  def __dir__(self):
    return list(self.__dict__.keys()) + dir(self.p)

  # def __call__(self):
  #   return self.get_value()

  def get_value(self):
    return self.unit().unit.get_control_port_value(self.p.index)

  def set_value(self, v):
    self.unit().unit.set_control_port_value(self.p.index, v)

  value = property(get_value, set_value)

  def bind_midi(self, *args):
    self.unit().bind_midi(self.p.index, *args)

  def unbind_midi(self, *args):
    self.unit().unbind_midi(self.p.index, *args)

class with_ports:
  pass

class dict_with_attributes:
  def __init__(self):
    self.__d = {}

  def __getitem__(self, name):
    return self.__d[name]

  def __setitem__(self, name, value):
    self.__d[name] = value

  def __len__(self):
    return len(self.__d)

class horst(with_ports):
  def __init__(self, uri, jack_client_name = "", expose_control_ports = False):
    self.h = h.jacked_horst(lv2_plugins, uri, jack_client_name, expose_control_ports)
    self.jack_client_name = self.h.get_jack_client_name()

    self.ports = dict_with_attributes()

    self.audio = []
    self.audio_in = []
    self.audio_out = []

    for index in range (self.h.get_number_of_ports ()):
      p = props (self, index)
      p.jack_name = self.jack_client_name + ":" + p.name
      setattr(self, p.name + '_', p)
      self.ports[index] = p

      if p.is_audio and not p.is_side_chain:
        self.audio.append(p)

        if p.is_input:
          self.audio_in.append(p)

        if p.is_output:
          self.audio_out.append(p)

  def __getitem__(self, index):
    return self.ports[index]

  def __getattr__(self, name):
    return getattr(self.h, name)

  def __dir__(self):
    return list(self.__dict__.keys()) + dir(self.h)

  def bind_midi(self, port_index, channel, cc, factor = 1.0, offset = 0.0):
    b = horst.midi_binding(True, channel, cc, factor, offset)
    self.h.set_midi_binding(port_index, b)

  def unbind_midi(self, port_index):
    b = horst.midi_binding(False, 0, 0, 0, 0)
    self.h.set_midi_binding(port_index, b)

# class lv2(unit):
#   def __init__(self, uri, jack_client_name = "", expose_control_ports = False):
#     if uri in lv2.blacklisted_uris:
#       raise RuntimeError("blacklisted uri: " + uri)
#     unit.__init__(self, the_horst.lv2 (uri, jack_client_name, expose_control_ports), expose_control_ports)
#
#   blacklisted_uris = [
#     'http://github.com/blablack/ams-lv2/fftvocoder'
#   ]

class system_ports(with_ports):
  def __init__(self):
    self.audio_in = [namedtuple('system_port', 'jack_name', defaults=["system:playback_" + str(n)])() for n in range(1,256)]
    self.audio_out = [namedtuple('system_port', 'jack_name', defaults=["system:capture_" + str(n)])() for n in range(1,256)]

system = system_ports()

def connect2(source, sink):
  # print('connect2: ' + str(source) + ' ' + str(sink))
  if isinstance(source, with_ports) and isinstance(sink, with_ports):
    count = min(len(source.audio_out), len(sink.audio_in))
    # print(count)
    return [(source.audio_out[n].jack_name, sink.audio_in[n].jack_name) for n in range(count)]

  if isinstance(source, with_ports):
    return [connect2(source.audio_out[n].jack_name, sink) for n in range(len(source.audio_out))]

  if isinstance(sink, with_ports):
    return [connect2(source, sink.audio_in[n].jack_name) for n in range(len(sink.audio_in))]

  if isinstance(sink, props):
    return connect2(source, sink.jack_name)

  if isinstance(source, props):
    return connect2(source.jack_name, sink)

  print('base: ' + str(source) + ' ' + str(sink))
  return [(source, sink)]

def connect1(l):
  r = []
  # print('connect1: ' + str(l))
  for c in l:
    r = r + connect2(c[0], c[1])
  return r

def connect(*args):
  # print('connect: ' + str(args))
  cs = []

  # We assume this is a list of connections
  if len(args) == 1:
    cs = connect1(*args)

  # In this case we assume these are two things to connect
  if len(args) == 2:
    cs = connect2(*args)

  # This case describes a serial chain of things to connect
  if len(args) > 2:
    for n in range(1,len(args)):
      cs = cs + connect2(args[n-1], args[n])
  # print('final connections: ' + str(cs))

  hcs = []
  for c in cs:
    hcs.append((c[0], c[1]))

  h.connection_manager().connect(hcs)

from itertools import chain

class parallel (with_ports):
  def __init__(self, units):
    self.units = units
    self.audio_in = list(chain.from_iterable([u.audio_in for u in units]))
    self.audio_out = list(chain.from_iterable([u.audio_out for u in units]))

class serial (with_ports):
  def __init__(self, units):
    self.units = units
    self.audio_in = units[0].audio_in
    self.audio_out = units[-1].audio_out

if __name__ == '__main__':
  import os
  import argparse

  parser = argparse.ArgumentParser(
    prog="hing.py",
    description="A command line tool to host lv2 plugins and a python library as well",
    epilog="You probably want to run this python script in interactive mode (\"python -i hing.py\")! Otherwise it just exits after loading all plugins.")

  parser.add_argument('-u', '--uri', help='A plugin URI. This argument can be given more than once. The loaded plugins are available in the global variable \"plugins\" ', nargs="*")
  args = parser.parse_args()

  plugins = list(map(instantiate, args.uri))
