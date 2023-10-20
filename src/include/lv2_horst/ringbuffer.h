#pragma once

#include <vector>
#include <atomic>
#include <cassert>

#include <lv2_horst/dbg.h>
#include <lv2_horst/error.h>

namespace lv2_horst
{
    template<class T>
    struct continuous_ringbuffer
    {
        size_t m_half_buffer_size;

        std::vector<T> m_buffer;
        std::vector<size_t> m_sizes;

        std::atomic<size_t> m_head;
        std::atomic<size_t> m_tail;

        /*
         * NOTE: A ringbuffer that makes reading and writing
         * continuous chunks of memory easier.
         *
         * NOTE: Maximum writable chunk size is (half_buffer_size - 1)
         *
         * NOTE: Maximum usable capacity is (half_buffer_size - 1)
         */
        continuous_ringbuffer (size_t half_buffer_size) :
            m_half_buffer_size (half_buffer_size),
            m_buffer (half_buffer_size * 2),
            m_sizes (half_buffer_size, 0),
            m_head (0),
            m_tail (0)
        {
            m_sizes[0] = 0;
            assert_invariants ();
        }

        inline void assert_invariants ()
        {
            assert (m_tail < m_half_buffer_size);
            assert (m_head < m_half_buffer_size);
            assert (m_tail == m_head ? true : m_sizes[m_tail] < (m_half_buffer_size - 1));
        }

        inline void report_status ()
        {
            DBG("m_buffer[m_tail]: " << m_buffer[m_tail] << ", m_sizes[m_tail]: " << m_sizes[m_tail] << ", capacity: " << m_half_buffer_size << ", m_head: " << m_head << ", m_tail: " << m_tail << ", read_available: " << read_available() << ", write_available: " << write_available())
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
                effective_tail += m_half_buffer_size;
            }

            return effective_tail - m_head - 1;
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
            assert (n < m_half_buffer_size);
            assert_invariants ();
            assert (m_sizes[m_tail] == n);

            m_tail += n;
            m_tail = m_tail % m_half_buffer_size;
        }

        /*
         * Advances the write pointer and updates the size field
         * for the last written chunk
         */
        inline void write_advance (size_t n)
        {
            assert_invariants ();
            m_sizes[m_head] = n;
            m_head += n;
            m_head = m_head % m_half_buffer_size;
        }
    };

    template<class T>
    struct ringbuffer
    {
        std::vector<T> m_buffer;
        
        std::atomic<size_t> m_head;
        std::atomic<size_t> m_tail;
        
        /*
         * NOTE: Usable capacity is (buffer_size - 1)
         */
        ringbuffer (size_t buffer_size) :
            m_buffer (buffer_size),
            m_head (0),
            m_tail (0)
        {

        }
        
        inline void report_status ()
        {
            DBG("capacity: " << m_buffer.size() << ", m_head: " << m_head << ", m_tail: " << m_tail << ", read_available: " << read_available() << ", write_available: " << write_available())
        }
        
        inline size_t read_available ()
        {
            size_t effective_head = m_head;
            
            if (m_head < m_tail)
            {
                effective_head += m_buffer.size ();
            }
            
            return effective_head - m_tail;
        }
        
        inline size_t write_available ()
        {
            size_t effective_tail = m_tail;
            
            if (m_tail <= m_head)
            {
                effective_tail += m_buffer.size ();
            }
            
            return effective_tail - m_head - 1;
        }
        
        inline void write (const T &m)
        {
            m_head = m_head % m_buffer.size ();

            if (write_available () == 0)
            {
                THROW("No space left for writing")
            }
            
            m_buffer[m_head++] = m;
        }
        
        inline const T read ()
        {
            m_tail = m_tail % m_buffer.size ();

            if (read_available () == 0)
            {
                THROW("No space left for reading")
            }
            
            return m_buffer[m_tail++];
        }
    };
}
