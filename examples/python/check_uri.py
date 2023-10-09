import horsting as h
import sys
import time

try:
  print(f"check_uri.py: Creating unit from URI: {sys.argv[1]}")
  p = h.unit(sys.argv[1])
  print("check_uri.py: Sleeping...")
  time.sleep(1)
  print("check_uri.py: Destroying unit...")
  p = None
except Exception as e:
  print(e)

print ("check_uri.py: Done.")
