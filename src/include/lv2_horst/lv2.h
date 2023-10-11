#pragma once

#include <lilv/lilv.h>

#include <memory>
#include <vector>
#include <string>

#include <lv2_horst/dbg.h>
#include <lv2_horst/error.h>

namespace lv2_horst
{
  struct lilv_world 
  {
    LilvWorld *m;

    lilv_world () :
      m(lilv_world_new())
    {
      DBG_ENTER
      if (m == 0) THROW("Failed to create lilv world");
      DBG("m: " << (void*)m)
      lilv_world_load_all (m);
      DBG_EXIT
    }

    ~lilv_world () 
    {
      DBG_ENTER
      DBG("m: " << (void*)m)
      lilv_world_free (m);
      DBG_EXIT
    }
  };

  typedef std::shared_ptr<lilv_world> lilv_world_ptr;

  struct lilv_uri_node 
  {
    const std::string m_uri;
    lilv_world_ptr m_world;
    LilvNode *m;

    lilv_uri_node
    (
      lilv_world_ptr world,
      const std::string &uri
    ) :
      m_uri (uri),
      m_world (world),
      m (lilv_new_uri (world->m, uri.c_str ())) 
    {
      if (m == 0) THROW("Failed to create lilv uri node. URI: " + uri);
    }

    ~lilv_uri_node () 
    {
      lilv_node_free (m);
    }
  };

  typedef std::shared_ptr<lilv_uri_node> lilv_uri_node_ptr;

  struct lilv_plugins 
  {
    const LilvPlugins *m;
    lilv_world_ptr m_world;
    std::vector<lilv_uri_node_ptr> m_plugin_uri_nodes;
    std::vector<std::string> m_plugin_uris;

    lilv_plugins
    (
      lilv_world_ptr world = lilv_world_ptr (new lilv_world)
    ) :
      m (lilv_world_get_all_plugins (world->m)),
      m_world (world) 
    {
      DBG_ENTER
      LILV_FOREACH (plugins, i, m)
      {
        const LilvPlugin* p = lilv_plugins_get(m, i);
        DBG("Plugin URI: " << std::string(lilv_node_as_uri(lilv_plugin_get_uri(p))))
        m_plugin_uris.push_back(lilv_node_as_uri(lilv_plugin_get_uri(p)));
      }
      DBG_EXIT
    }

    std::vector<std::string> get_uris()
    {
      return m_plugin_uris;
    }

    ~lilv_plugins () 
    {
      DBG_ENTER_EXIT
    }
  };

  typedef std::shared_ptr<lilv_plugins> lilv_plugins_ptr;

  struct lilv_plugin 
  {
    const LilvPlugin *m;
    lilv_uri_node_ptr m_uri_node;
    lilv_plugins_ptr m_plugins;

    lilv_plugin
    (
      lilv_plugins_ptr plugins,
      lilv_uri_node_ptr node
    ) :
      m (lilv_plugins_get_by_uri (plugins->m, node->m)),
      m_uri_node (node),
      m_plugins (plugins) 
    {
      DBG_ENTER
      if (m == 0) THROW("Plugin not found. URI: " + m_uri_node->m_uri);
      DBG_EXIT
    }
  };

  typedef std::shared_ptr<lilv_plugin> lilv_plugin_ptr;

  struct lilv_plugin_instance 
  {
    LilvInstance *m;
    LV2_Handle m_handle;
    lilv_plugin_ptr m_plugin;

    std::vector<std::vector<float>> m_initial_port_buffers;

    lilv_plugin_instance
    (
      lilv_plugin_ptr plugin,
      double sample_rate,
      LV2_Feature *const *supported_features
    ) :
      m (lilv_plugin_instantiate (plugin->m, sample_rate, supported_features)),
      m_plugin (plugin),
      m_initial_port_buffers (lilv_plugin_get_num_ports (m_plugin->m), std::vector<float>(128))
    {
      DBG_ENTER
      if (m == 0) THROW("Failed to instantiate plugin");

      m_handle = lilv_instance_get_handle (m);
      DBG(m)

      for (size_t port_index = 0; port_index < m_initial_port_buffers.size (); ++port_index) 
      {
        lilv_instance_connect_port (m, port_index, &m_initial_port_buffers[port_index][0]);
      }

      lilv_instance_activate (m);
      DBG_EXIT
    }

    ~lilv_plugin_instance () 
    {
      DBG_ENTER
      lilv_instance_deactivate (m);
      lilv_instance_free (m);
      DBG_EXIT
    }
  };

  typedef std::shared_ptr<lilv_plugin_instance> lilv_plugin_instance_ptr;
}
