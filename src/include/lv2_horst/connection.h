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

    void connect
    (
      std::vector<std::pair<std::string, std::string>> &the_connections,
      bool throw_on_error = false
    )
    {
      for (size_t index = 0; index < the_connections.size (); ++index)
      {
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
