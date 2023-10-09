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
}
