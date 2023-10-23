#pragma once

#include <vector>
#include <atomic>
#include <cassert>

#include <lv2_horst/dbg.h>
#include <lv2_horst/error.h>

namespace lv2_horst
{
  /*
   * Single producer / single consumer lockless ringbuffer
   */
  struct continuous_chunk_ringbuffer
  {
    int m_base_buffer_size;
    int m_required_chunk_size;

    std::vector<uint8_t> m_buffer;

    /*
     * The buffer is empty when m_head == m_tail.
     *
     * The buffer is full when m_head is one "behind"
     * m_tail.
     */
    std::atomic<int> m_head;
    std::atomic<int> m_tail;

    continuous_chunk_ringbuffer (int base_buffer_size, int required_chunk_size) :
      m_base_buffer_size (base_buffer_size),
      m_required_chunk_size (required_chunk_size),
      m_buffer (base_buffer_size + required_chunk_size),
      m_head (0),
      m_tail (0)
    {
      assert (required_chunk_size < (base_buffer_size - sizeof(int)));

      assert_invariants ();
    }

    continuous_chunk_ringbuffer (int base_buffer_size) :
      continuous_chunk_ringbuffer (base_buffer_size, base_buffer_size - 1 - sizeof(int))
    {

    }

    inline void assert_invariants ()
    {
      assert (m_tail < m_base_buffer_size);
      assert (m_head < m_base_buffer_size);
    }

    inline void report_status ()
    {
      DBG("capacity: " << m_base_buffer_size << ", m_head: " << m_head << ", m_tail: " << m_tail << ", read_available: " << read_available() << ", write_available: " << write_available())
    }

    inline bool isempty ()
    {
      return (m_head == m_tail);
    }

    /*
     * Returns the size of the next chunk available for reading
     */
    inline int read_available ()
    {
      assert_invariants ();

      if (isempty ()) return 0;

      const int chunk_size = *((int*)&m_buffer[m_tail]);

      assert (chunk_size <= m_required_chunk_size);

      return chunk_size;
    }

    /*
     * Returns the maximum size available for the next chunk
     * for writing.
     */
    inline int write_available ()
    {
      assert_invariants ();

      int effective_tail = m_tail;

      if (m_tail <= m_head)
      {
        effective_tail += m_base_buffer_size;
      }

      return std::min(effective_tail - m_head - 1 - (int)sizeof(int), m_required_chunk_size);
    }

    /*
     * Copies into user provided buffer. Returns the number of
     * items read. If you need to know this before hand, look
     * at read_available ().
     */
    inline int read (uint8_t* data)
    {
      assert_invariants ();

      int n = *((int*)&m_buffer[m_tail]);

      for (int index = 0; index < n; ++index)
      {
        data[index] = m_buffer[m_tail + index + sizeof(int)];
      }

      read_advance (n);

      return n;
    }

    inline void write (const uint8_t * const data, int n)
    {
      assert_invariants ();
      assert (n <= write_available ());

      DBG("write_available() : " << write_available())

      for (int index = 0; index < n; ++index)
      {
        m_buffer[m_head + index + (int)sizeof(int)] = data[index];
        // m_sizes[m_head + index] = n - index;
      }

      write_advance (n);
    }

    /*
     * NOTE: Use with great care!
     *
     * Returns a pointer where you can read up to read_available ()
     * items in a continuous fashion.
     *
     * Use read_advance () to advance the tail of the ringbuffer
     * after reading.
     *
     * See also read_available () and read_advance ()
     */
    inline uint8_t* read_pointer ()
    {
      assert_invariants ();
      assert (read_available () >= 1);

      return &m_buffer[m_tail] + sizeof(int);
    }

    /*
     * NOTE: Use with great care!
     *
     * Returns a pointer into the buffer where you can write up to
     * write_available () items in a continuous fashion.
     *
     * Use write_advance () to advance the head of the ringbuffer
     * after writing.
     *
     * See also write_available (), write_advance ()
     */
    inline uint8_t* write_pointer ()
    {
      assert_invariants ();
      assert (write_available () >= 1);

      return &m_buffer[m_head] + (int)sizeof(int);
    }

    inline void read_advance (int n)
    {
      assert_invariants ();
      assert (n <= read_available ());
      assert (m_sizes[m_tail] == n);

      m_tail += n + (int)sizeof(int);
      m_tail = m_tail % m_base_buffer_size;
    }

    /*
     * Advances the write pointer and updates the size field
     * for the last written chunk
     */
    inline void write_advance (int n)
    {
      assert_invariants ();
      assert (n <= write_available ());

      *((int*)&m_buffer[m_head]) = n;

      m_head += n + (int)sizeof(int);
      m_head = m_head % m_base_buffer_size;
    }
  };
}
