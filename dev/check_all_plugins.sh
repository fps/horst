for n in `lv2ls`; do timeout 5s ./dev/pywrap.sh examples/python/load_plugin.py "$n"; sleep 0.1 || break; done
# for n in `lv2ls`; do timeout 5s ./src/horst_cli "$n"; sleep 0.1 || break; done
# echo `lv2ls` | xargs ./pywrap.sh check_uri.py
