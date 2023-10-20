#pragma once

#include <vector>
#include <atomic>
#include <cassert>

#include <lv2_horst/dbg.h>
#include <lv2_horst/error.h>

namespace lv2_horst
{
  template<class T>
  struct continuous_chunk_ringbuffer
  {
    size_t m_base_buffer_size;
    size_t m_required_chunk_size;

    std::vector<T> m_buffer;

    /*
     * m_sizes[m_tail] holds the size of the chunk
     * at m_buffer[m_tail].
     */
    std::vector<size_t> m_sizes;

    /*
     * The buffer is empty when m_head == m_tail.
     *
     * The buffer is full when m_head is one "behind"
     * m_tail.
     */
    std::atomic<size_t> m_head;
    std::atomic<size_t> m_tail;

    /*
     * NOTE: A ringbuffer that enables reading and writing
     * continuous chunks of memory. It guarantees that
     * the number of items returned by read_available and
     * write_available can be read/written in a continuous
     * chunk of up to size required_chunk_size.
     *
     * NOTE: Maximum usable capacity is (base_buffer_size - 1)
     *
     * NOTE: There is another hack to increase the usable
     * capacity in certain situations. I.e. if m_tail and m_head
     * are both == 0, then the space available is actually
     * base_buffer_size + required_chunk_size, but this would
     * make things more complicated than possibly worth it.
     *
     * NOTE: Having m_sizes is a trade-off which trades
     * implementation simplicity for space. Other schemes are
     * definitely possible.
     */
    continuous_chunk_ringbuffer (size_t base_buffer_size, size_t required_chunk_size) :
      m_base_buffer_size (base_buffer_size),
      m_required_chunk_size (required_chunk_size),
      m_buffer (base_buffer_size + required_chunk_size),
      m_sizes (base_buffer_size, 0),
      m_head (0),
      m_tail (0)
    {
      assert (required_chunk_size < base_buffer_size);

      m_sizes[0] = 0;
      assert_invariants ();
    }

    continuous_chunk_ringbuffer (size_t base_buffer_size) :
      continuous_chunk_ringbuffer (base_buffer_size, base_buffer_size - 1)
    {

    }

    inline void assert_invariants ()
    {
      assert (m_tail < m_base_buffer_size);
      assert (m_head < m_base_buffer_size);
      assert (m_tail == m_head ? true : m_sizes[m_tail] <= m_required_chunk_size);
    }

    inline void report_status ()
    {
      DBG("m_buffer[m_tail]: " << m_buffer[m_tail] << ", m_sizes[m_tail]: " << m_sizes[m_tail] << ", capacity: " << m_base_buffer_size << ", m_head: " << m_head << ", m_tail: " << m_tail << ", read_available: " << read_available() << ", write_available: " << write_available())
    }

    /*
     * Returns the size of the next chunk available for reading
     */
    inline size_t read_available ()
    {
      assert_invariants ();

      if (m_head == m_tail)
      {
        return 0;
      }

      return m_sizes[m_tail];
    }

    /*
     * Returns the maximum size available for the next chunk
     * for writing.
     */
    inline size_t write_available ()
    {
      assert_invariants ();

      size_t effective_tail = m_tail;

      if (m_tail <= m_head)
      {
        effective_tail += m_base_buffer_size;
      }

      return std::min(effective_tail - m_head - 1, m_required_chunk_size);
    }

    /*
     * Copies into user provided buffer. Returns the number of
     * items read. If you need to know this before hand, look
     * at read_available ().
     */
    inline size_t read (T* data)
    {
      assert_invariants ();

      size_t n = m_sizes[m_tail];

      for (size_t index = 0; index < n; ++index)
      {
        data[index] = m_buffer[m_tail + index];
      }

      read_advance (n);

      return n;
    }

    inline void write (const T * const data, size_t n)
    {
      assert_invariants ();

      assert (n <= write_available ());

      DBG("write_available() : " << write_available())

      for (size_t index = 0; index < n; ++index)
      {
        m_buffer[m_head + index] = data[index];
        // m_sizes[m_head + index] = n - index;
      }

      write_advance (n);
    }

    inline void write (const T &data)
    {
      assert_invariants ();
      assert (write_available () >= 1);

      write (&data, 1);
    }

    inline T read ()
    {
      assert_invariants ();
      assert (m_sizes[m_tail] == 1);

      // report_status ();
      T data;

      read (&data);

      return data;
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
    inline T* read_pointer ()
    {
      assert_invariants ();
      assert (read_available () >= 1);

      return &m_buffer[m_tail];
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
    inline T* write_pointer ()
    {
      assert_invariants ();
      assert (write_available () >= 1);

      return &m_buffer[m_head];
    }

    inline void read_advance (size_t n)
    {
      assert_invariants ();
      assert (n <= read_available ());
      assert (m_sizes[m_tail] == n);

      m_tail += n;
      m_tail = m_tail % m_base_buffer_size;
    }

    /*
     * Advances the write pointer and updates the size field
     * for the last written chunk
     */
    inline void write_advance (size_t n)
    {
      assert_invariants ();
      assert (n <= write_available ());

      m_sizes[m_head] = n;
      m_head += n;
      m_head = m_head % m_base_buffer_size;
    }
  };
}
