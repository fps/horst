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
        
        void report_status ()
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
            if (write_available () < 1)
            {
                THROW("No space left for writing")
            }
            
            m_buffer[m_head++] = m;
            m_head = m_head % m_buffer.size ();
        }
        
        inline const T read ()
        {
            if (read_available () < 1)
            {
                THROW("No space left for reading")
            }
            
            const T& m = m_buffer[m_tail++];
            m_tail = m_tail % m_buffer.size ();
            
            return m;
        }
    };
}
