#include <jack/jack.h>
#include <horst/dbg.h>

namespace horst
{
  struct connection
  {
    const std::string m_from;
    const std::string m_to;

    connection
    (
      const std::string &from,
      const std::string &to
    ) :
      m_from (from), m_to (to)
    {

    }
  };

  struct connections
  {
    std::vector<connection> m;

    void add (const connection &c)
    {
      m.push_back (c);
    }

    void add (const std::string &from, const std::string &to)
    {
      m.push_back (connection (from, to));
    }
  };

  struct connection_manager
  {
    jack_client_t *m_jack_client;

    connection_manager
    (
      const std::string &jack_client_name
    ) :
      m_jack_client (jack_client_open (jack_client_name.c_str(), JackNullOption, 0))
    {
      DBG_ENTER

      if (m_jack_client == 0)
      {
        throw std::runtime_error ("horst: horst: Failed to open jack client: horst");
      }

      DBG_EXIT
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

  typedef std::shared_ptr<connection_manager> connection_manager_ptr;
}
