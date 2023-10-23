import lv2_horst as h
import sys
import time

try:
  plugins = h.lv2_plugins ()
  print(f"check_uri.py: Creating unit from URI: {sys.argv[1]}")
  p = h.horst(plugins, sys.argv[1])
  print("Instantiating plugin")
  p.instantiate (48000, 128)

  for n in range(0, 16):
    print("Running for 128 frames")
    p.run (128)
    time.sleep(0.01)

  print("check_uri.py: Destroying unit...")
  p = None
except Exception as e:
  print(e)

print ("check_uri.py: Done.")
