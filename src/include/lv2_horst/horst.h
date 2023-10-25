#pragma once

#include <lv2_horst/lv2.h>
#include <lv2_horst/continuous_chunk_ringbuffer.h>

#include <lv2/worker/worker.h>
#include <lv2/state/state.h>
#include <lv2/options/options.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/atom/atom.h>
#include <lv2/patch/patch.h>

#include <algorithm>
#include <cstring>
#include <vector>
#include <atomic>
#include <array>
#include <sstream>
#include <condition_variable>
#include <mutex>

namespace lv2_horst
{
  struct port_properties 
  {
    bool m_is_audio;
    bool m_is_control;
    bool m_is_cv;
    bool m_is_input;
    bool m_is_output;
    bool m_is_side_chain;
    float m_minimum_value;
    float m_default_value;
    float m_maximum_value;
    bool m_is_logarithmic;
    std::string m_name;
    std::string m_symbol;
  };

  extern "C" 
  {
    LV2_URID 
    urid_map 
    (
      LV2_URID_Map_Handle handle, 
      const char *uri
    );
    
    LV2_Worker_Status 
    schedule_work 
    (
      LV2_Worker_Schedule_Handle handle, 
      uint32_t size, 
      const void *data
    );
    
    void *
    worker_thread 
    (
      void *
    );
    
    LV2_Worker_Status 
    worker_respond 
    (
      LV2_Worker_Respond_Handle handle, 
      uint32_t size, 
      const void *data
    );
    
    LV2_State_Status 
    state_store 
    (
      LV2_State_Handle handle, 
      uint32_t key, 
      const void *value, 
      size_t size, 
      uint32_t type, 
      uint32_t flags
    );
    
    const void *
    state_retrieve 
    (
      LV2_State_Handle handle, 
      uint32_t key, 
      size_t *size, 
      uint32_t *type, 
      uint32_t *flags
    );
  }

  #define HORST_DEFAULT_WORK_ITEMS_QUEUE_SIZE (1024 * 1024 * 10)
  #define HORST_DEFAULT_WORK_RESPONSE_ITEMS_QUEUE_SIZE (1024 * 1024 * 10)
  #define HORST_DEFAULT_REALTIME_MESSAGES_QUEUE_SIZE (1024 * 10)

  #define STRINGIFY(x) #x
  #define STRINGIFY2(x) STRINGIFY(x)
  #define LOG_REALTIME_MESSAGE(x) { log_realtime_message (__FILE_NAME__ ":", STRINGIFY2(__LINE__) " ", __FUNCTION__, "(): " x); }

  struct writable_parameter
  {
    std::string m_label;
    LV2_URID m_range;
  };

  struct horst
  {
    lv2_plugins_ptr m_lv2_plugins;
    lilv_plugin_ptr m_lilv_plugin;

    std::vector<writable_parameter> m_writable_parameters;

    std::vector<port_properties> m_port_properties;

    bool m_fixed_block_length_required;
    bool m_power_of_two_block_length_required;

    lilv_plugin_instance_ptr m_plugin_instance;

    const std::string m_uri;
    std::string m_name;

    uint32_t m_min_block_length;
    uint32_t m_max_block_length;
    uint32_t m_nominal_block_length;

    std::vector<LV2_Options_Option> m_options;

    LV2_State_Interface *m_state_interface;
    bool m_state_interface_required;

    LV2_Worker_Schedule m_worker_schedule;
    std::atomic<LV2_Worker_Interface*> m_worker_interface;
    bool m_worker_required;

    continuous_chunk_ringbuffer m_work_items_buffer;
    continuous_chunk_ringbuffer m_work_response_items_buffer;
    continuous_chunk_ringbuffer m_realtime_log_messages;
    size_t m_missed_realtime_log_messages;

    bool m_need_to_notify_worker_thread;
    std::mutex m_worker_mutex;
    std::condition_variable m_worker_condition_variable;
    std::atomic<bool> m_worker_quit;
    pthread_t m_worker_thread;

    std::vector<std::string> m_mapped_uris;

    LV2_URID_Map m_urid_map;
    LV2_Feature m_urid_map_feature;
    LV2_Feature m_is_live_feature;
    LV2_Feature m_bounded_block_length_feature;
    LV2_Feature m_nominal_block_length_feature;
    LV2_Feature m_fixed_block_length_feature;
    LV2_Feature m_coarse_block_length_feature;
    LV2_Feature m_power_of_two_block_length_feature;
    LV2_Feature m_options_feature;
    LV2_Feature m_worker_feature;
    std::vector<LV2_Feature*> m_supported_features;

    horst
    (
      lv2_plugins_ptr plugins,
      const std::string &uri
    ) :
      m_lv2_plugins (plugins),
      m_lilv_plugin
      (
        new lilv_plugin
        (
          plugins->m_plugins,
          lilv_uri_node_ptr
          (
            new lilv_uri_node(plugins->m_world, uri)
          )
        )
      ),

      m_fixed_block_length_required (false),
      m_power_of_two_block_length_required (false),

      m_state_interface (0),
      m_state_interface_required (false),

      m_worker_schedule { (LV2_Worker_Schedule_Handle)this, lv2_horst::schedule_work },
      m_worker_interface (0),
      m_worker_required (false),

      m_work_items_buffer (HORST_DEFAULT_WORK_ITEMS_QUEUE_SIZE),
      m_work_response_items_buffer (HORST_DEFAULT_WORK_RESPONSE_ITEMS_QUEUE_SIZE),
      m_realtime_log_messages (HORST_DEFAULT_REALTIME_MESSAGES_QUEUE_SIZE),
      m_missed_realtime_log_messages (0),

      m_need_to_notify_worker_thread (false),

      m_worker_quit (false),

      m_urid_map { .handle = (LV2_URID_Map_Handle)this, .map = lv2_horst::urid_map },

      m_urid_map_feature { .URI = LV2_URID__map, .data = &m_urid_map },
      m_is_live_feature { .URI = LV2_CORE__isLive, .data = 0 },
      m_bounded_block_length_feature { .URI = LV2_BUF_SIZE__boundedBlockLength, .data = 0 },
      m_nominal_block_length_feature { .URI = LV2_BUF_SIZE__nominalBlockLength, .data = 0 },
      m_fixed_block_length_feature { .URI = LV2_BUF_SIZE__fixedBlockLength, .data = 0 },
      m_coarse_block_length_feature { .URI = LV2_BUF_SIZE__coarseBlockLength, .data = 0 },
      m_power_of_two_block_length_feature { .URI = LV2_BUF_SIZE__powerOf2BlockLength, .data = 0 },
      m_options_feature { .URI = LV2_OPTIONS__options, .data = &m_options[0] },
      m_worker_feature { .URI = LV2_WORKER__schedule, .data = &m_worker_schedule }
    {
      DBG_ENTER
      m_options.push_back (LV2_Options_Option { .context = LV2_OPTIONS_INSTANCE, .subject = 0, .key = urid_map (LV2_BUF_SIZE__minBlockLength), .size = sizeof (int32_t), .type = urid_map (LV2_ATOM__Int), .value = &m_min_block_length });
      m_options.push_back (LV2_Options_Option { .context = LV2_OPTIONS_INSTANCE, .subject = 0, .key = urid_map (LV2_BUF_SIZE__maxBlockLength), .size = sizeof (int32_t), .type = urid_map (LV2_ATOM__Int), .value = &m_max_block_length });
      m_options.push_back (LV2_Options_Option { .context = LV2_OPTIONS_INSTANCE, .subject = 0, .key = urid_map (LV2_BUF_SIZE__nominalBlockLength), .size = sizeof (int32_t), .type = urid_map (LV2_ATOM__Int), .value = &m_max_block_length });
      m_options.push_back (LV2_Options_Option { .context = LV2_OPTIONS_INSTANCE, .subject = 0, .key = 0, .size = 0, .type = 0, .value = 0 });
      m_options_feature.data = &m_options[0];

      m_supported_features.push_back (&m_urid_map_feature);
      m_supported_features.push_back (&m_is_live_feature);
      m_supported_features.push_back (&m_options_feature);
      m_supported_features.push_back (&m_bounded_block_length_feature);
      m_supported_features.push_back (&m_nominal_block_length_feature);
      m_supported_features.push_back (&m_fixed_block_length_feature);
      m_supported_features.push_back (&m_coarse_block_length_feature);
      m_supported_features.push_back (&m_power_of_two_block_length_feature);
      m_supported_features.push_back (&m_worker_feature);
      m_supported_features.push_back (0);

      check_features (true);
      check_features (false);

      #ifdef HORST_DEBUG
      lilv_uri_node required_options_uri (m_lv2_plugins->m_world, LV2_OPTIONS__requiredOption);
      LilvNodes *required_options = lilv_plugin_get_value (m_lilv_plugin->m, required_options_uri.m);
      LILV_FOREACH (nodes, i, required_options) {
        const LilvNode *node = lilv_nodes_get (required_options, i);
        DBG("Required options: " << lilv_node_as_string (node))
      }
      lilv_nodes_free (required_options);
      #endif

      lilv_uri_node state_extension_node (m_lv2_plugins->m_world, LV2_STATE__interface);
      if (lilv_plugin_has_extension_data (m_lilv_plugin->m, state_extension_node.m))
      {
        DBG("Has state extension")
        m_state_interface_required = true;
      }

      lilv_uri_node patch_writable_uri_node (m_lv2_plugins->m_world, LV2_PATCH__writable);
      LilvNodes *patch_writables = lilv_world_find_nodes (m_lv2_plugins->m_world->m, m_lilv_plugin->m_uri_node->m, patch_writable_uri_node.m, 0);
      LILV_FOREACH (nodes, i, patch_writables) {
        const LilvNode *node = lilv_nodes_get (patch_writables, i);
        DBG("patch writable: " << lilv_node_as_string (node))

        lilv_uri_node range_node (m_lv2_plugins->m_world, "http://www.w3.org/1999/02/22-rdf-syntax-ns#range");
        LilvNode* writable = lilv_world_get (m_lv2_plugins->m_world->m, node, range_node.m, 0);
        if (writable)
        {
          DBG("range: " << lilv_node_as_string(writable));
        }
      }
      lilv_nodes_free (patch_writables);

      lilv_uri_node input (m_lv2_plugins->m_world, LILV_URI_INPUT_PORT);
      lilv_uri_node output (m_lv2_plugins->m_world, LILV_URI_OUTPUT_PORT);
      lilv_uri_node audio (m_lv2_plugins->m_world, LILV_URI_AUDIO_PORT);
      lilv_uri_node control (m_lv2_plugins->m_world, LILV_URI_CONTROL_PORT);
      lilv_uri_node cv (m_lv2_plugins->m_world, LILV_URI_CV_PORT);
      lilv_uri_node side_chain (m_lv2_plugins->m_world, "https://lv2plug.in/ns/lv2core#isSideChain");

      m_port_properties.resize (lilv_plugin_get_num_ports (m_lilv_plugin->m));
      for (size_t index = 0; index < m_port_properties.size(); ++index) 
      {
        const LilvPort *lilv_port = lilv_plugin_get_port_by_index (m_lilv_plugin->m, index);
        port_properties &p = m_port_properties[index];
        p.m_symbol = lilv_node_as_string (lilv_port_get_symbol (m_lilv_plugin->m, lilv_port));
        p.m_name = lilv_node_as_string (lilv_port_get_name (m_lilv_plugin->m, lilv_port));

        p.m_is_audio = lilv_port_is_a (m_lilv_plugin->m, lilv_port, audio.m);
        p.m_is_control = lilv_port_is_a (m_lilv_plugin->m, lilv_port, control.m);
        p.m_is_cv = lilv_port_is_a (m_lilv_plugin->m, lilv_port, cv.m);
        p.m_is_input = lilv_port_is_a (m_lilv_plugin->m, lilv_port, input.m);
        p.m_is_output = lilv_port_is_a (m_lilv_plugin->m, lilv_port, output.m);
        p.m_is_side_chain = lilv_port_has_property (m_lilv_plugin->m, lilv_port, side_chain.m);

        if (p.m_is_input && p.m_is_control) 
        {
          LilvNode *def;
          LilvNode *min;
          LilvNode *max;

          lilv_port_get_range (m_lilv_plugin->m, lilv_port, &def, &min, &max);

          p.m_minimum_value = lilv_node_as_float (min);
          p.m_default_value = lilv_node_as_float (def);
          p.m_maximum_value = lilv_node_as_float (max);

          lilv_node_free (def);
          lilv_node_free (min);
          lilv_node_free (max);
        }
      }

      lilv_uri_node doap_name (m_lv2_plugins->m_world, "http://usefulinc.com/ns/doap#name");
      LilvNode *name_node = lilv_world_get (m_lv2_plugins->m_world->m, m_lilv_plugin->m_uri_node->m, doap_name.m, 0);
      if (name_node == 0) THROW("Failed to get name of plugin. URI: " + m_uri);
      m_name = lilv_node_as_string (name_node);
      lilv_node_free (name_node);

      if (m_worker_required)
      {
        int ret = pthread_create (&m_worker_thread, 0, lv2_horst::worker_thread, this);
        if (ret != 0) THROW("Failed to create worker thread");
      }
      DBG_EXIT
    }

    void instantiate
    (
      double sample_rate,
      size_t buffer_size
    )
    {
      DBG(sample_rate << " " << buffer_size)
      m_min_block_length = 0;
      m_max_block_length = (int32_t)buffer_size;
      m_nominal_block_length = (int32_t)buffer_size;

      if (m_fixed_block_length_required) 
      {
        m_min_block_length = (int32_t)buffer_size;
      }

      m_plugin_instance = lilv_plugin_instance_ptr (new lilv_plugin_instance (m_lilv_plugin, sample_rate, &m_supported_features[0]));

      if (m_worker_required) m_worker_interface = (LV2_Worker_Interface*)lilv_instance_get_extension_data (m_plugin_instance->m, LV2_WORKER__interface); 

      DBG("worker interface: " << m_worker_interface);

      if (m_state_interface_required) m_state_interface = (LV2_State_Interface*)lilv_instance_get_extension_data (m_plugin_instance->m, LV2_STATE__interface);

      DBG("state interface: " << m_state_interface);

      if (m_worker_interface)
      {
        DBG((void*)(m_worker_interface.load ()->work) << " " << (void*)(m_worker_interface.load ()->work_response) << " " << (void*)(m_worker_interface.load ()->end_run));
      }
      // usleep (500000);
    }

    void check_features (bool required)
    {
      LilvNodes *features = required 
        ? 
          lilv_plugin_get_required_features (m_lilv_plugin->m) 
        : 
          lilv_plugin_get_optional_features (m_lilv_plugin->m);

      if (features != 0) {
        std::stringstream s;
        LILV_FOREACH(nodes, i, features) {
          const LilvNode *node = lilv_nodes_get (features, i);
          std::string feature_uri =  lilv_node_as_uri (node);
          if (feature_uri == LV2_WORKER__schedule) m_worker_required = true;
          bool supported = false;
          for (size_t feature_index = 0; feature_index < m_supported_features.size () - 1; ++feature_index) 
          {
            if (m_supported_features[feature_index]->URI == feature_uri) {
              if (feature_uri == m_power_of_two_block_length_feature.URI) {
                m_fixed_block_length_required = true;
                m_power_of_two_block_length_required = true;
              }
              if (feature_uri == m_fixed_block_length_feature.URI || feature_uri == m_coarse_block_length_feature.URI) {
                m_fixed_block_length_required = true;
              }
              supported = true;
              break;
            }
          }
          if (!supported)
          {
            if  (required) {
              lilv_nodes_free (features);
              THROW("Unsupported feature: " + feature_uri);
            }
            else
            {
              DBG("Unsupported optional feature: " << feature_uri)
            }
          }
        }
        lilv_nodes_free (features);
      }
    }

    void connect_port
    (
      size_t port_index,
      float *data
    )
    {
      if (port_index >= m_port_properties.size ())
      {
        THROW("Index out of bounds")
      }

      lilv_instance_connect_port (m_plugin_instance->m, port_index, data);
    }

    void run
    (
      size_t nframes
    )
    {
      if (!m_plugin_instance)
      {
        THROW("No instance!");
      }
      
      LV2_Worker_Interface *interface = m_worker_interface;
      if (interface) 
      {
        while (!m_work_response_items_buffer.isempty ())
        {
          LOG_REALTIME_MESSAGE("!m_work_response_items_buffer.isempty()")
          size_t item_size = m_work_response_items_buffer.read_available ();

          if (interface->work_response)
          {
            LOG_REALTIME_MESSAGE("Calling interface->work_response()")
            interface->work_response (m_plugin_instance->m_handle, item_size, m_work_response_items_buffer.read_pointer ());
          }

          m_work_response_items_buffer.read_advance (item_size);
        }
      }

      lilv_instance_run (m_plugin_instance->m, nframes);

      if (m_need_to_notify_worker_thread)
      {
        LOG_REALTIME_MESSAGE("Need to notify worker thread")
        bool locked = m_worker_mutex.try_lock ();
        if (locked)
        {
          LOG_REALTIME_MESSAGE("Notifying worker_thread")
          m_worker_mutex.unlock ();
          m_worker_condition_variable.notify_one ();
          m_need_to_notify_worker_thread = false;
        }
      }

      if (interface && interface->end_run) 
      {
        interface->end_run (m_plugin_instance->m_handle);
      }
    }

    const std::string urid_unmap
    (
      LV2_URID urid
    )
    {
      if (urid >= m_mapped_uris.size ()) 
      {
        THROW("URID out of bounds");
      }

      return m_mapped_uris[urid];
    }

    LV2_URID urid_map
    (
      const char *uri
    )
    {
      auto it = std::find (m_mapped_uris.begin (), m_mapped_uris.end (), uri);
      LV2_URID urid = it - m_mapped_uris.begin ();
      if (it == m_mapped_uris.end ()) 
      {
        m_mapped_uris.push_back (uri);
      }

      urid += 1;

      DBG("URI: " << uri << " -> URID: " << urid)
      return urid;
    }

    LV2_Worker_Status schedule_work
    (
      uint32_t size,
      const void *data
    )
    {
      LOG_REALTIME_MESSAGE ("schedule_work!");

      if (m_worker_quit == true) {
        LOG_REALTIME_MESSAGE ("worker_quit == true");
        return LV2_WORKER_ERR_UNKNOWN;
      }

      if (!m_worker_interface)
      {
        return LV2_WORKER_ERR_UNKNOWN;
      }
      
      LOG_REALTIME_MESSAGE ("m_worker_interface != 0");

      if (m_work_items_buffer.write_available () < (int)size)
      {
        LOG_REALTIME_MESSAGE ("ERROR: NO SPACE LEFT FOR WRITING WORK ITEM!");
        return LV2_WORKER_ERR_NO_SPACE;
      }

      LOG_REALTIME_MESSAGE ("Copying data into buffer");
      memcpy(m_work_items_buffer.write_pointer (), data, size);
      m_work_items_buffer.write_advance (size);

      m_need_to_notify_worker_thread = true;

      LOG_REALTIME_MESSAGE ("Done.");
      return LV2_WORKER_SUCCESS;
    }

    LV2_Worker_Status worker_respond
    (
      uint32_t size,
      const void *data
    )
    {
      DBG_ENTER
      DBG("size: " << size)
      if (m_worker_interface) 
      {
        DBG("m_worker_interface != 0");

        if (m_work_response_items_buffer.write_available () >= (int)size)
        {
          DBG("Copying data into buffer. Size: " << size)
          memcpy (m_work_response_items_buffer.write_pointer (), data, size);
          m_work_response_items_buffer.write_advance (size);
        }
        else
        {
          INFO("ERROR: NO SPACE LEFT FOR WRITING RESPONSE ITEM")
          return LV2_WORKER_ERR_NO_SPACE;
        }
      }
      DBG_EXIT
      return LV2_WORKER_SUCCESS;
    }

    void *worker_thread () 
    {
      DBG_ENTER
      while (!m_worker_quit)
      {
        {
          DBG("Acquiring lock")
          std::unique_lock lock (m_worker_mutex);
          DBG("Waiting on condition variable")
          m_worker_condition_variable.wait (lock);
          DBG("Done waiting.")
        }

        //if (m_missed_realtime_log_messages != 0)
        {
          DBG("[RT] missed messages: " << m_missed_realtime_log_messages)
        }

        while (false == m_realtime_log_messages.isempty ())
        {
          int chunk_size = m_realtime_log_messages.read_available ();
          DBG("[RT] " << (char*)m_realtime_log_messages.read_pointer ())
          m_realtime_log_messages.read_advance (chunk_size);
        }

        LV2_Worker_Interface *interface = m_worker_interface;

        if (!m_worker_interface) continue;

        while (false == m_work_items_buffer.isempty ())
        {
          DBG("m_work_items_buffer.empty () == false")

          if (interface->work) 
          {
            DBG("interface->work != 0")

            // DBG("plugin_instance->: " << m_plugin_instance->m << " has interface->work")
            size_t item_size = m_work_items_buffer.read_available ();
            DBG("item_size: " << item_size)

            LV2_Worker_Status res =
              interface->work (m_plugin_instance->m_handle, &lv2_horst::worker_respond, (LV2_Worker_Respond_Handle)this, item_size, m_work_items_buffer.read_pointer ());

            if (res != LV2_WORKER_SUCCESS)
            {
              INFO("res != LV2_WORKER_SUCCESS. res: " << res)
            }

            DBG("res: " << res)
            m_work_items_buffer.read_advance (item_size);
          }
        }
      }
      DBG_EXIT
      return 0;
    }

    void log_realtime_message (const char *filename, const char* line, const char *function, const char *x)
    {
        size_t required_chunk_size = strlen(filename) + strlen(line) + strlen(function) + strlen(x) + 1;
        
        if (m_realtime_log_messages.write_available () < (int)required_chunk_size)
        {
          ++m_missed_realtime_log_messages;
          return;
        }
        
        uint8_t *write_pointer = m_realtime_log_messages.write_pointer ();
        
        memcpy(write_pointer, filename, strlen(filename));
        write_pointer += strlen(filename);
        
        memcpy(write_pointer, line, strlen(line));
        write_pointer += strlen(line);
        
        memcpy(write_pointer, function, strlen(function));
        write_pointer += strlen(function);
        
        memcpy(write_pointer, x, strlen(x) + 1);
        write_pointer += strlen(x) + 1;
        
        m_realtime_log_messages.write_advance (required_chunk_size);
    }
    
    void log_realtime_message (const char * message)
    {
      size_t chunk_size = strlen (message) + 1;

      if (m_realtime_log_messages.write_available () < (int)chunk_size)
      {
        ++m_missed_realtime_log_messages;
        return;
      }

      memcpy(m_realtime_log_messages.write_pointer (), message, chunk_size);
      m_realtime_log_messages.write_advance (chunk_size);

      m_need_to_notify_worker_thread = true;
    }

    void save_state
    (
      const std::string &path
    )
    {
      if (m_state_interface) 
      {
        m_state_interface->save (m_plugin_instance->m_handle, state_store, 0, 0, 0);
      }
      else
      {
        DBG("No state interface - not saving")
      }
    }

    void restore_state
    (
      const std::string &path
    )
    {
      if (m_state_interface) 
      {
        m_state_interface->restore (m_plugin_instance->m_handle, state_retrieve, 0, 0, 0);
      }
      else
      {
        DBG("No state interface - not restoring")
      }
    }

    ~horst ()
    {
      DBG_ENTER
      if (m_worker_required)
      {
        {
          std::unique_lock lock (m_worker_mutex);
          m_worker_quit = true;
          m_worker_condition_variable.notify_one ();
        }
        DBG("Waiting for worker thread...")
        pthread_join (m_worker_thread, 0);
        DBG("Thread has joined")

        DBG("Draining [RT] messages...")
        while (!m_realtime_log_messages.isempty())
        {
          int chunk_size = m_realtime_log_messages.read_available ();
          DBG("[RT] " << m_realtime_log_messages.read_pointer ())
          m_realtime_log_messages.read_advance (chunk_size);
        }
      }

      m_plugin_instance = lilv_plugin_instance_ptr();
      DBG_EXIT
    }
  };

  extern "C" 
  {
    LV2_URID urid_map
    (
      LV2_URID_Map_Handle handle,
      const char *uri
    )
    {
      return ((horst*)handle)->urid_map(uri);
    }

    LV2_Worker_Status schedule_work
    (
      LV2_Worker_Schedule_Handle handle,
      uint32_t size,
      const void *data
    )
    {
      return  ((horst*)handle)->schedule_work (size, data);
    }

    LV2_Worker_Status worker_respond
    (
      LV2_Worker_Respond_Handle handle,
      uint32_t size,
      const void *data
    )
    {
      return ((horst*)handle)->worker_respond (size, data);
    }

    void *worker_thread
    (
      void *arg
    )
    {
      return ((horst*)arg)->worker_thread ();
    }

    LV2_State_Status state_store
    (
      LV2_State_Handle handle,
      uint32_t key,
      const void *value,
      size_t size,
      uint32_t type,
      uint32_t flags
    )
    {
      // TODO: Implement state_store ()
      DBG_ENTER
      DBG("key: " << key << " size: " << size << " type: " << type << " flags: " << flags)
      DBG_EXIT
      return LV2_STATE_ERR_UNKNOWN;
    }

    const void *state_retrieve
    (
      LV2_State_Handle handle,
      uint32_t key,
      size_t *size,
      uint32_t *type,
      uint32_t *flags
    )
    {
      // TODO:: implement state_retrieve ()
      DBG_ENTER
      DBG("key: " << key << " size: " << size << " type: " << type << " flags: " << flags)
      DBG_EXIT
      return 0;
    }
  }


  typedef std::shared_ptr<horst> horst_ptr;
}
