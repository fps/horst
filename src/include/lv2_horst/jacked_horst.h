#pragma once

#include <lv2_horst/horst.h>
#include <lv2_horst/midi_binding.h>
#include <lv2_horst/ringbuffer.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <cmath>

namespace lv2_horst
{
  const int cc_mask = 128 + 32 + 16;

  extern "C" 
  {
    int jacked_horst_sample_rate_callback
    (
      jack_nframes_t nframes,
      void *arg
    );

    int jacked_horst_buffer_size_callback
    (
      jack_nframes_t nframes,
      void *arg
    );

    int jacked_horst_process_callback
    (
      jack_nframes_t nframes,
      void *arg
    );

    void jacked_horst_thread_init_callback
    (
      void *arg
    );
  }

  struct jacked_horst
  {
    std::atomic<bool> m_atomic_enabled;
    std::atomic<bool> m_atomic_control_input_updates_enabled;
    std::atomic<bool> m_atomic_control_output_updates_enabled;
    std::atomic<bool> m_atomic_audio_input_monitoring_enabled;
    std::atomic<bool> m_atomic_audio_output_monitoring_enabled;

    horst_ptr m_horst;

    jack_client_t *m_jack_client;
    jack_nframes_t m_sample_rate;
    jack_nframes_t m_buffer_size;

    const bool m_expose_control_ports;

    std::vector<jack_port_t *> m_jack_ports;
    std::vector<float *> m_jack_port_buffers;

    std::vector<std::vector<float>> m_zero_buffers;
    std::vector<float *> m_port_data_locations;

    std::vector<size_t> m_jack_input_port_indices;
    std::vector<size_t> m_jack_output_port_indices;

    jack_port_t *m_jack_midi_port;
    std::vector<std::atomic<float>> m_atomic_port_values;
    std::vector<float> m_port_values;

    // TODO: allow more than one binding per port:
    std::vector<std::atomic<midi_binding>> m_atomic_midi_bindings;

    jacked_horst
    (
      lv2_plugins_ptr plugins,
      const std::string &uri,
      const std::string &jack_client_name,
      bool expose_control_ports
    ) :
      m_atomic_enabled (true),
      m_atomic_control_input_updates_enabled (true),
      m_atomic_control_output_updates_enabled (false),
      m_atomic_audio_input_monitoring_enabled (false),
      m_atomic_audio_output_monitoring_enabled (false),
      m_horst (new horst (plugins, uri)),
      m_jack_client (jack_client_open ((jack_client_name == "" ? m_horst->m_name : jack_client_name).c_str (), JackNullOption, 0)),
      m_expose_control_ports (expose_control_ports),
      m_jack_ports (m_horst->m_port_properties.size (), 0),
      m_jack_port_buffers (m_horst->m_port_properties.size (), 0),
      m_port_data_locations (m_horst->m_port_properties.size (), 0),
      m_atomic_port_values (m_horst->m_port_properties.size ()),
      m_port_values (m_horst->m_port_properties.size (), 0),
      m_atomic_midi_bindings (m_horst->m_port_properties.size ())
    {
      DBG_ENTER

      if (m_jack_client == 0) THROW("Failed to open jack client: " + jack_client_name);

      m_buffer_size = jack_get_buffer_size (m_jack_client);
      m_sample_rate = jack_get_sample_rate (m_jack_client);
      m_zero_buffers = std::vector<std::vector<float>> (m_horst->m_port_properties.size (), std::vector<float> (m_buffer_size, 0));

      m_horst->instantiate (m_sample_rate, m_buffer_size);

      m_jack_midi_port = jack_port_register (m_jack_client, "midi-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      if (m_jack_midi_port == 0) THROW("Failed to register midi port: " + m_horst->m_name + ":midi-in");

      for (size_t index = 0; index < m_horst->m_port_properties.size(); ++index)
      {
        port_properties &p = m_horst->m_port_properties[index];

        DBG("port: index: " << index << " \"" << p.m_symbol << "\"" << " min: " << p.m_minimum_value << " default: " << p.m_default_value << " max: " << p.m_maximum_value << " log: " << p.m_is_logarithmic << " input: " << p.m_is_input << " output: " << p.m_is_output << " audio: " << p.m_is_audio << " control: " << p.m_is_control << " cv: " << p.m_is_cv << " side_chain: " << p.m_is_side_chain)

        if (p.m_is_control)
        {
          if (p.m_is_input)
          {
            DBG("setting default: " << p.m_default_value)

            m_atomic_port_values[index] = p.m_default_value;
            m_port_values[index] = p.m_default_value;
          }

          m_port_data_locations[index] = &m_port_values[index];
        }

        if ((p.m_is_control && m_expose_control_ports) || p.m_is_audio || p.m_is_cv) 
        {
          if (p.m_is_input) 
          {
            DBG("port: index: " << index << " registering jack input port")
            m_jack_ports[index] = jack_port_register (m_jack_client, p.m_symbol.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

            if (m_jack_ports[index] == 0) THROW(std::string("Failed to register port: ") + m_horst->m_name + ":" + p.m_symbol);
            m_jack_input_port_indices.push_back (index);
          } 
          else 
          {
            DBG("port: index: " << index << " registering jack output port")
            m_jack_ports[index] = jack_port_register (m_jack_client, p.m_symbol.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

            if (m_jack_ports[index] == 0) THROW(std::string("Failed to register port: ") + m_horst->m_name + ":" + p.m_symbol);

            m_jack_output_port_indices.push_back (index);
          }
        }
      }

      connect_control_ports ();

      DBG("setting callbacks")
      int ret;
      ret = jack_set_sample_rate_callback (m_jack_client, jacked_horst_sample_rate_callback, (void*)this);
      if (ret != 0) THROW("Failed to set sample rate callback");

      ret = jack_set_buffer_size_callback (m_jack_client, jacked_horst_buffer_size_callback, (void*)this);
      if (ret != 0) THROW("Failed to set buffer size callback");

      ret = jack_set_process_callback (m_jack_client, jacked_horst_process_callback, (void*)this);
      if (ret != 0) THROW("Failed to set process callback");

      ret = jack_set_thread_init_callback (m_jack_client, jacked_horst_thread_init_callback, (void*)this);
      if (ret != 0) THROW("Failed to set thread init callback");

      DBG("activating jack client")
      ret = jack_activate (m_jack_client);
      if (ret != 0) THROW("Failed to activate client");
      DBG_EXIT
    }

    horst_ptr get_horst ()
    {
      return m_horst;
    }

    void connect_control_ports ()
    {
      for (size_t port_index = 0; port_index < m_horst->m_port_properties.size (); ++port_index)
      {
        const port_properties &p = m_horst->m_port_properties[port_index];

        if (p.m_is_control)
        {
          m_horst->connect_port (port_index, m_port_data_locations[port_index]);
        }
      }
    }

    ~jacked_horst ()
    {
      DBG_ENTER
      jack_deactivate (m_jack_client);
      jack_client_close (m_jack_client);
      DBG_EXIT
    }

    inline int process_callback
    (
      jack_nframes_t nframes
    )
    {
      const size_t number_of_ports = m_horst->m_port_properties.size ();
      const size_t number_of_jack_input_ports = m_jack_input_port_indices.size ();
      const size_t number_of_jack_output_ports = m_jack_output_port_indices.size ();

      const bool enabled = m_atomic_enabled;
      const bool control_input_updates_enabled = m_atomic_control_input_updates_enabled;
      const bool control_output_updates_enabled = m_atomic_control_output_updates_enabled;
      const bool audio_input_monitoring_enabled = m_atomic_audio_input_monitoring_enabled;
      const bool audio_output_monitoring_enabled = m_atomic_audio_output_monitoring_enabled;

      for (size_t index = 0; index < number_of_ports; ++index)
      {
        const port_properties &p = m_horst->m_port_properties[index];
        if (p.m_is_audio || (m_expose_control_ports && p.m_is_control) || p.m_is_cv)
        {
          m_jack_port_buffers[index] = (float*)jack_port_get_buffer (m_jack_ports[index], nframes);
          m_port_data_locations[index] = m_jack_port_buffers[index];

          if (p.m_is_input && !enabled)
          {
            m_port_data_locations[index] = &m_zero_buffers[index][0];
          }

          m_horst->connect_port (index, m_port_data_locations[index]);
        }
      }

      if (control_input_updates_enabled)
      {
        for (size_t index = 0; index < number_of_ports; ++index)
        {
          const port_properties &p = m_horst->m_port_properties[index];
          if (p.m_is_control && p.m_is_input && !m_expose_control_ports)
          {
            m_port_values[index] = m_atomic_port_values[index];
          }
        }
      }

      if (control_output_updates_enabled)
      {
        for (size_t index = 0; index < number_of_ports; ++index)
        {
          const port_properties &p = m_horst->m_port_properties[index];
          if (p.m_is_control && p.m_is_output && !m_expose_control_ports)
          {
            m_atomic_port_values[index] = m_port_values[index];
          }
        }
      }
      
      jack_nframes_t processed_frames = 0;

      void *midi_port_buffer = jack_port_get_buffer (m_jack_midi_port, nframes);
      int event_count = jack_midi_get_event_count (midi_port_buffer);

      for (int event_index = 0; event_index < event_count; ++event_index) 
      {
        jack_midi_event_t event;
        jack_midi_event_get (&event, midi_port_buffer, event_index);

        if (event.size != 3) continue;
        if ((event.buffer[0] & cc_mask) != cc_mask) continue;

        const int channel = event.buffer[0] & 15;
        const int cc = event.buffer[1];
        const float value = event.buffer[2] / 127.0f;

        bool changed = false;

        for (size_t port_index = 0; port_index < m_atomic_midi_bindings.size (); ++port_index) 
        {
          const port_properties &props = m_horst->m_port_properties[port_index];
          if (!(props.m_is_input && props.m_is_control)) continue;

          const midi_binding &binding = m_atomic_midi_bindings[port_index];

          if (!binding.m_enabled) continue;
          if (binding.m_cc != cc) continue;
          if (binding.m_channel != channel) continue;

          // DBG("calling run (" << event.time - processed_frames <<")")
          if (!m_horst->m_fixed_block_length_required && processed_frames != event.time)
          {
            m_horst->run (event.time - processed_frames);
            processed_frames = event.time;
            changed = true;
          }

          const float transformed_value = binding.m_offset + binding.m_factor * value;

          const float mapped_value = props.m_minimum_value + (props.m_maximum_value - props.m_minimum_value) * transformed_value;

          // std::cout << event.time << " " << port_index << " " << mapped_value << "\n";

          m_port_values[port_index] = m_atomic_port_values[port_index] = mapped_value;
        }

        if (changed) 
        {
          for (size_t port_index = 0; port_index < m_jack_port_buffers.size (); ++port_index) 
          {
            const port_properties &p = m_horst->m_port_properties[port_index];
            if ((p.m_is_control && m_expose_control_ports) || p.m_is_audio || p.m_is_cv) 
            {
              m_horst->connect_port (port_index, m_port_data_locations[port_index] + processed_frames);
            }
          }
        }
      }

      // DBG("calling run (" << nframes - processed_frames << ")")
      m_horst->run (nframes - processed_frames);

      if (!enabled) 
      {
        for (size_t port_index = 0; port_index < std::min (number_of_jack_input_ports, number_of_jack_output_ports); ++port_index) 
        {
          for (jack_nframes_t frame_index = 0; frame_index < nframes; ++frame_index) 
          {
            m_jack_port_buffers[m_jack_output_port_indices[port_index]][frame_index] += m_jack_port_buffers[m_jack_input_port_indices[port_index]][frame_index];
          }
        }
      }

      if (audio_input_monitoring_enabled) 
      {
        for (size_t index = 0; index < number_of_jack_input_ports; ++index)
        {
          float max_value = 0;
          const float * const buffer = m_jack_port_buffers[m_jack_input_port_indices[index]];
          for (size_t frame = 0; frame < nframes; ++frame) 
          {
            if (fabs(buffer[frame]) > max_value) max_value = buffer[frame];
          }
          m_atomic_port_values[m_jack_input_port_indices[index]] = max_value;
        }
      }
      else
      {
        for (size_t index = 0; index < number_of_jack_input_ports; ++index) m_atomic_port_values[m_jack_input_port_indices[index]] = 0.f;
      }

      if (audio_output_monitoring_enabled)
      {
        for (size_t index = 0; index < number_of_jack_output_ports; ++index)
        {
          float max_value = 0;
          const float * const buffer = m_jack_port_buffers[m_jack_output_port_indices[index]];
          for (size_t frame = 0; frame < nframes; ++frame) 
          {
            if (fabs(buffer[frame]) > max_value) max_value = buffer[frame];
          }
          m_atomic_port_values[m_jack_output_port_indices[index]] = max_value;
        }
      }
      else
      {
        for (size_t index = 0; index < number_of_jack_output_ports; ++index) m_atomic_port_values[m_jack_output_port_indices[index]] = 0.f;
      }

      return 0;
    }

    void change_buffer_sizes () 
    {
      if (m_horst->m_power_of_two_block_length_required)
      {
        if (!(m_buffer_size & (m_buffer_size - 1))) 
        {
          THROW("power of two buffer size required");
        }
      }
      for (size_t port_index = 0; port_index < m_horst->m_port_properties.size (); ++port_index)
      {
        m_zero_buffers[port_index].resize (m_buffer_size, 0);
      }
    }

    int buffer_size_callback
    (
      jack_nframes_t buffer_size
    )
    {
      DBG_ENTER
      DBG("buffer_size: " << buffer_size)
      if (buffer_size != m_buffer_size) 
      {
        m_buffer_size = buffer_size;

        change_buffer_sizes ();

        DBG("re-instantiating")
        m_horst->instantiate ((double)m_sample_rate, m_buffer_size);
        connect_control_ports ();
      }
      DBG_EXIT
      return 0;
    }

    int sample_rate_callback
    (
      jack_nframes_t sample_rate
    )
    {
      DBG_ENTER
      if (sample_rate != m_sample_rate) 
      {
        m_sample_rate = sample_rate;
        // std::cout << "sample rate callback. sample rate: " << sample_rate << "\n";
        DBG("re-instantiating")
        m_horst->instantiate ((double)m_sample_rate, m_buffer_size);
        connect_control_ports ();
      }
      DBG_EXIT
      return 0;
    }

    int get_number_of_ports () 
    {
      return (int)(m_horst->m_port_properties.size ());
    }

    void set_control_port_value
    (
      size_t index,
      float value
    )
    {
      DBG("index: " << index << ", value: " << value)
      if (index >= m_port_values.size ()) 
      {
        THROW("index out of bounds");
      }
      m_atomic_port_values [index] = value;
    }

    float get_control_port_value (size_t index) 
    {
      DBG("index: " << index << ", value: " << m_atomic_port_values[index])
      if (index >= m_port_values.size ()) 
      {
        THROW("index out of bounds");
      }
      return m_atomic_port_values [index];
    }

    void set_midi_binding
    (
      size_t index,
      const midi_binding &binding
    )
    {
      if (index >= m_port_values.size ())
      {
        THROW("index out of bounds");
      }

      m_atomic_midi_bindings[index] = binding;
    }

    midi_binding get_midi_binding (size_t index) 
    {
      if (index >= m_port_values.size ()) 
      {
        THROW("index out of bounds");
      }

      return m_atomic_midi_bindings[index];
    }

    std::string get_jack_client_name () const 
    {
      return jack_get_client_name (m_jack_client);
    }

    void set_enabled
    (
      bool enabled
    )
    {
      m_atomic_enabled = enabled;
    }

    void set_control_input_updates_enabled
    (
      bool enabled
    )
    {
      m_atomic_control_input_updates_enabled = enabled;
    }

    void set_control_output_updates_enabled
    (
      bool enabled
    )
    {
      m_atomic_control_output_updates_enabled = enabled;
    }

    void set_audio_input_monitoring_enabled
    (
      bool enabled
    )
    {
      m_atomic_audio_input_monitoring_enabled = enabled;
    }

    void set_audio_output_monitoring_enabled
    (
      bool enabled
    )
    {
      m_atomic_audio_output_monitoring_enabled = enabled;
    }

    void save_state
    (
      const std::string &path
    )
    {
      m_horst->save_state (path);
    }

    void restore_state
    (
      const std::string &path
    )
    {
      m_horst->restore_state (path);
    }
  };

  typedef std::shared_ptr<jacked_horst> jacked_horst_ptr;
  
  extern "C" 
  {
    int jacked_horst_sample_rate_callback
    (
      jack_nframes_t nframes,
      void *arg
    )
    {
      return ((jacked_horst*)arg)->sample_rate_callback (nframes);
    }

    int jacked_horst_buffer_size_callback
    (
      jack_nframes_t nframes,
      void *arg
    )
    {
      return ((jacked_horst*)arg)->buffer_size_callback (nframes);
    }

    int jacked_horst_process_callback
    (
      jack_nframes_t nframes,
      void *arg
    )
    {
      return ((jacked_horst*)arg)->process_callback (nframes);
    }
    
    void jacked_horst_thread_init_callback
    (
      void *arg
    )
    {
      DBG_ENTER
      /* Taken from cras/src/dsp/dsp_util.c in Chromium OS code. * Copyright (c) 
        2013 The Chromium OS Authors. */
      #if defined(__i386__) || defined(__x86_64__)
        unsigned int mxcsr; mxcsr = __builtin_ia32_stmxcsr(); 
        __builtin_ia32_ldmxcsr(mxcsr | 0x8040);
      #elif defined(__aarch64__)
        uint64_t cw; __asm__ __volatile__ ( "mrs %0, fpcr \n" "orr %0, %0, #0x1000000 \n"
                "msr fpcr, %0 \n" "isb \n"
                : "=r"(cw) :: "memory");
      #elif defined(__arm__)
        uint32_t cw; __asm__ __volatile__ ( "vmrs %0, fpscr \n" "orr %0, %0, #0x1000000 \n"
                "vmsr fpscr, %0 \n"
                : "=r"(cw) :: "memory");
      #else 
         DBG("Don't know how to disable denormals. Performace may suffer.")
      #endif
      DBG_EXIT
    }
  }
}
