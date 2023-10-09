import lv2_horst

world = lv2_horst.lilv_world()
plugins = lv2_horst.lilv_plugins(world)

uris = plugins.get_uris()
plugin = lv2_horst.plugin(world, lv2_horst.lilv_plugin(plugins, lv2_horst.lilv_uri_node(world, uris[0])))

instance = lv2_horst.jack_plugin_horst(plugin)


