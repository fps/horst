#pragma once

#include <vector>
#include <atomic>

#include <lv2_horst/dbg.h>
#include <lv2_horst/error.h>

namespace lv2_horst
{
    template<class T>
    struct ringbuffer
    {
        std::vector<T> m_buffer;
        
        std::atomic<std::size_t> m_head;
        std::atomic<std::size_t> m_tail;
        
        ringbuffer (std::size_t capacity) :
            m_buffer (capacity + 1),
            m_head (0),
            m_tail (0)
        {

        }
        
        void report_status ()
        {
            DBG("capacity: " << m_buffer.size() << ", m_head: " << m_head << ", m_tail: " << m_tail << ", read_available: " << read_available() << ", write_available: " << write_available())
        }
        
        std::size_t read_available ()
        {
            std::size_t effective_head = m_head;
            
            if (m_head < m_tail)
            {
                effective_head += m_buffer.size ();
            }
            
            return effective_head - m_tail;
        }
        
        std::size_t write_available ()
        {
            std::size_t effective_tail = m_tail;
            
            if (m_tail <= m_head)
            {
                effective_tail += m_buffer.size ();
            }
            
            return effective_tail - m_head;
        }
        
        void write (const T &m)
        {
            if (write_available () < 1)
            {
                THROW("No space left for writing")
            }
            
            m_buffer[m_head++] = m;
            m_head = m_head % m_buffer.size ();
        }
        
        const T read ()
        {
            if (read_available () < 1)
            {
                THROW("No space left for reading")
            }
            
            const T& m = m_buffer[m_tail];
            ++m_tail;
            m_tail = m_tail % m_buffer.size ();
            return m;
        }
    };
}
