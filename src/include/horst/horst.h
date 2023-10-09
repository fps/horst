#pragma once

#include <horst/connection.h>
#include <horst/unit.h>
#include <horst/dbg.h>

namespace horst 
{
  struct horst 
  {
    jack_client_t *m_jack_client;
    lilv_world_ptr m_lilv_world;
    lilv_plugins_ptr m_lilv_plugins;
    std::string m_horst_dli_fname;
    std::string m_jack_dli_fname;

    horst () :
      m_jack_client (jack_client_open ("horst", JackNullOption, 0)),
      m_lilv_world (new lilv_world),
      m_lilv_plugins (new lilv_plugins (m_lilv_world))
    {
      DBG_ENTER

      if (m_jack_client == 0)
      {
        throw std::runtime_error ("horst: horst: Failed to open jack client: horst");
      }

      DBG_EXIT
    }

    ~horst ()
    {
      DBG_ENTER
      jack_client_close (m_jack_client);
      DBG_EXIT
    }

    std::vector<std::string> lv2_uris ()
    {
      std::vector<std::string> uris;

      LILV_FOREACH (plugins, i, m_lilv_plugins->m)
      {
        const LilvPlugin* p = lilv_plugins_get(m_lilv_plugins->m, i);
        uris.push_back(lilv_node_as_uri(lilv_plugin_get_uri(p)));
      }

      return uris;
    }

    unit_ptr lv2
    (
      const std::string &uri,
      const std::string &jack_client_name,
      bool expose_control_ports
    )
    {
      plugin_ptr p (new plugin (m_lilv_world, m_lilv_plugins, uri));

      std::string final_jack_client_name = (jack_client_name != "") ? jack_client_name : p->get_name ();

      return unit_ptr (new unit (p, final_jack_client_name, expose_control_ports));
    }

    void connect
    (
      const connections& cs
    )
    {
      for (size_t index = 0; index < cs.m.size(); ++index)
      {
        jack_connect (m_jack_client, cs.m[index].m_from.c_str (), cs.m[index].m_to.c_str ());
      }
    }

    void disconnect
    (
      const connections& cs
    )
    {
      for (size_t index = 0; index < cs.m.size(); ++index)
      {
        jack_disconnect (m_jack_client, cs.m[index].m_from.c_str (), cs.m[index].m_to.c_str ());
      }
    }
  };

  typedef std::shared_ptr<horst> horst_ptr;
}


