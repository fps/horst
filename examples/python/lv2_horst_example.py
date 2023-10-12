import lv2_horst

plugins = lv2_horst.lv2_plugins()

first_uri = plugins.get_uris()[0]

horsted = lv2_horst.jacked_horst(plugins, first_uri)


