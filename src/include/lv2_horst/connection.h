#include <jack/jack.h>
#include <lv2_horst/dbg.h>
#include <lv2_horst/error.h>

#include <vector>
#include <utility>
#include <algorithm>

namespace lv2_horst
{
  struct connection_manager
  {
    jack_client_t *m_jack_client;

    connection_manager
    (
      const std::string &jack_client_name = "lv2_horst_connection_manager"
    ) :
      m_jack_client (jack_client_open (jack_client_name.c_str(), JackNullOption, 0))
    {
      DBG_ENTER

      if (m_jack_client == 0)
      {
        THROW(std::string("Failed to open jack client: ") + jack_client_name);
      }

      DBG_EXIT
    }

    std::vector<std::string> get_ports
    (
      const std::string &port_name_patttern = "",
      const std::string &port_type_pattern = "",
      unsigned long flags = 0
    )
    {
      std::vector<std::string> ports;

      const char **jack_ports = jack_get_ports (m_jack_client, port_name_patttern.c_str (), port_type_pattern.c_str (), flags);

      if (0 == jack_ports) return ports;

      const char **jack_ports_iterator = jack_ports;
      while (*jack_ports_iterator != 0)
      {
        DBG("port: " << (*jack_ports_iterator))
        ports.push_back (*jack_ports_iterator);

        // free (*jack_ports);
        ++jack_ports_iterator;
      }

      if (0 != jack_ports) free (jack_ports);

      return ports;
    }

    void connect
    (
      std::vector<std::pair<std::string, std::string>> &the_connections,
      bool throw_on_error = false
    )
    {
      for (size_t index = 0; index < the_connections.size (); ++index)
      {
        DBG("Connecting: \"" << the_connections[index].first << "\" -> \"" << the_connections[index].second << "\"")
        int ret = jack_connect (m_jack_client, the_connections[index].first.c_str (), the_connections[index].second.c_str ());

        if (0 != ret && throw_on_error)
        {
          THROW(std::string("Failed to connect: \"") + the_connections[index].first + "\" -> \"" + the_connections[index].second)
        }
      }
    }

    void disconnect
    (
      std::vector<std::pair<std::string, std::string>> &the_connections
    )
    {
      for (size_t index = 0; index < the_connections.size (); ++index)
      {
        DBG("Disonnecting: \"" << the_connections[index].first << "\" -> \"" << the_connections[index].second << "\"")
        int ret = jack_disconnect (m_jack_client, the_connections[index].first.c_str (), the_connections[index].second.c_str ());

        if (0 != ret)
        {
          THROW(std::string("Failed to disconnect: \"") + the_connections[index].first + "\" -> \"" + the_connections[index].second)
        }
      }
    }
  };

  typedef std::shared_ptr<connection_manager> connection_manager_ptr;
}
