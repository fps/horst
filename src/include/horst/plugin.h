#pragma once

#include <horst/lv2.h>

namespace horst
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
  };

  extern "C" 
  {
    LV2_URID urid_map (LV2_URID_Map_Handle handle, const char *uri);
    LV2_Worker_Status schedule_work (LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);
    void *worker_thread (void *);
    LV2_Worker_Status worker_respond (LV2_Worker_Respond_Handle handle, uint32_t size, const void *data);
    LV2_State_Status state_store (LV2_State_Handle handle, uint32_t key, const void *value, size_t size, uint32_t type, uint32_t flags);
    const void *state_retrieve (LV2_State_Handle handle, uint32_t key, size_t *size, uint32_t *type, uint32_t *flags);
  }

  #define HORST_DEFAULT_WORK_QUEUE_SIZE (1024 * 1024 * 10)
  #define HORST_DEFAULT_WORK_RESPONSE_QUEUE_SIZE (1024 * 1024)

  struct writable_parameter
  {
    std::string m_label;
    LV2_URID m_range;
  };

  struct lv2_plugin 
  {
    lilv_world_ptr m_lilv_world;
    lilv_plugins_ptr m_lilv_plugins;
    lilv_uri_node_ptr m_lilv_plugin_uri;
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

    std::array<std::pair<unsigned, std::array<uint8_t, HORST_WORK_ITEM_MAX_SIZE>>, HORST_WORK_ITEMS> m_work_items;
    std::atomic<size_t> m_work_items_head;
    std::atomic<size_t> m_work_items_tail;

    std::array<std::pair<unsigned, std::array<uint8_t, HORST_WORK_ITEM_MAX_SIZE>>, HORST_WORK_ITEMS> m_work_responses;
    std::atomic<size_t> m_work_responses_head;
    std::atomic<size_t> m_work_responses_tail;

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
              throw std::runtime_error ("horst: lv2_plugin: Unsupported feature: " + feature_uri);
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

    lv2_plugin (lilv_world_ptr world, lilv_plugins_ptr plugins, const std::string &uri) :
      m_lilv_world (world),
      m_lilv_plugins (plugins),
      m_lilv_plugin_uri (new lilv_uri_node (world, uri)),
      m_lilv_plugin (new lilv_plugin (plugins, m_lilv_plugin_uri)),

      m_fixed_block_length_required (false),
      m_power_of_two_block_length_required (false),

      m_uri (uri),

      m_state_interface (0),
      m_state_interface_required (false),

      m_worker_schedule { (LV2_Worker_Schedule_Handle)this, horst::schedule_work },
      m_worker_interface (0),
      m_worker_required (false),

      m_work_items_head (0),
      m_work_items_tail (0),

      m_work_responses_head (0),
      m_work_responses_tail (0),

      m_worker_quit (false),

      m_urid_map { .handle = (LV2_URID_Map_Handle)this, .map = horst::urid_map },

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
      for (size_t index = 0; index < HORST_WORK_ITEMS; ++index) 
      {
        m_work_items[index].first = 0;
        m_work_items[index].second = std::array<uint8_t, HORST_WORK_ITEM_MAX_SIZE>{};
        m_work_responses[index].first = 0;
        m_work_responses[index].second = std::array<uint8_t, HORST_WORK_ITEM_MAX_SIZE>{};
      }

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
      lilv_uri_node required_options_uri (world, LV2_OPTIONS__requiredOption);
      LilvNodes *required_options = lilv_plugin_get_value (m_lilv_plugin->m, required_options_uri.m);
      LILV_FOREACH (nodes, i, required_options) {
        const LilvNode *node = lilv_nodes_get (required_options, i);
        DBG("Required options: " << lilv_node_as_string (node))
      }
      lilv_nodes_free (required_options);
      #endif

      lilv_uri_node state_extension_node (world, LV2_STATE__interface);
      if (lilv_plugin_has_extension_data (m_lilv_plugin->m, state_extension_node.m))
      {
        DBG("Has state extension")
        m_state_interface_required = true;
      }

      lilv_uri_node patch_writable_uri_node (m_lilv_world, LV2_PATCH__writable);
      LilvNodes *patch_writables = lilv_world_find_nodes (m_lilv_world->m, m_lilv_plugin->m_uri_node->m, patch_writable_uri_node.m, 0);
      LILV_FOREACH (nodes, i, patch_writables) {
        const LilvNode *node = lilv_nodes_get (patch_writables, i);
        DBG("patch writable: " << lilv_node_as_string (node))

        lilv_uri_node range_node (m_lilv_world, "http://www.w3.org/1999/02/22-rdf-syntax-ns#range");
        LilvNode* writable = lilv_world_get (m_lilv_world->m, node, range_node.m, 0);
        if (writable)
        {
          DBG("range: " << lilv_node_as_string(writable));
        }
      }
      lilv_nodes_free (patch_writables);

      lilv_uri_node input (world, LILV_URI_INPUT_PORT);
      lilv_uri_node output (world, LILV_URI_OUTPUT_PORT);
      lilv_uri_node audio (world, LILV_URI_AUDIO_PORT);
      lilv_uri_node control (world, LILV_URI_CONTROL_PORT);
      lilv_uri_node cv (world, LILV_URI_CV_PORT);
      lilv_uri_node side_chain (world, "https://lv2plug.in/ns/lv2core#isSideChain");

      m_port_properties.resize (lilv_plugin_get_num_ports (m_lilv_plugin->m));
      for (size_t index = 0; index < m_port_properties.size(); ++index) 
      {
        const LilvPort *lilv_port = lilv_plugin_get_port_by_index (m_lilv_plugin->m, index);
        port_properties &p = m_port_properties[index];
        p.m_name = lilv_node_as_string (lilv_port_get_symbol (m_lilv_plugin->m, lilv_port));

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

      lilv_uri_node doap_name (world, "http://usefulinc.com/ns/doap#name");
      LilvNode *name_node = lilv_world_get (world->m, m_lilv_plugin_uri->m, doap_name.m, 0);
      if (name_node == 0) throw std::runtime_error ("horst: lv2_plugin: Failed to get name of plugin. URI: " + m_uri);
      m_name = lilv_node_as_string (name_node);
      lilv_node_free (name_node);

      if (m_worker_required)
      {
        int ret = pthread_create (&m_worker_thread, 0, horst::worker_thread, this);
        if (ret != 0) throw std::runtime_error ("horst: lv2_plugin: Failed to create worker thread");
      }
      DBG_EXIT
    }

    const std::string &get_name () const 
    { 
      return m_name; 
    }

    void instantiate (double sample_rate, size_t buffer_size) 
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

    void connect_port (size_t port_index, float *data) 
    {
      lilv_instance_connect_port (m_plugin_instance->m, port_index, data);
    }

    void run (size_t nframes) 
    {
      LV2_Worker_Interface *interface = m_worker_interface;
      if (interface) 
      {
        if (number_of_items (m_work_responses_head, m_work_responses_tail, m_work_responses.size ())) 
        {
          DBG_JACK("has responses")
          auto &item = m_work_responses[m_work_responses_tail];
          if (interface->work_response) 
          {
            DBG_JACK("item: " << item.first << " " << (int)item.second[0])
            interface->work_response (m_plugin_instance->m_lv2_handle, item.first, &item.second[0]);
          }
          advance (m_work_responses_tail, m_work_responses.size ());
        }
      }

      lilv_instance_run (m_plugin_instance->m, nframes);

      if (interface && interface->end_run) 
      {
        interface->end_run (m_plugin_instance->m_lv2_handle);
      }
    }

    const std::string urid_unmap (LV2_URID urid) 
    {
      if (urid >= m_mapped_uris.size ()) 
      {
        throw std::runtime_error ("URID out of bounds");
      }

      return m_mapped_uris[urid];
    }

    LV2_URID urid_map (const char *uri) 
    {
      auto it = std::find (m_mapped_uris.begin (), m_mapped_uris.end (), uri);
      LV2_URID urid = it - m_mapped_uris.begin ();
      if (it == m_mapped_uris.end ()) 
      {
        m_mapped_uris.push_back (uri);
      }

      urid += 1;

      // std::cout << "URI: " << uri << " -> " << urid << "\n";
      DBG("URI: " << uri << " -> URID: " << urid)
      return urid;
    }

    int number_of_items (const int &head, const int &tail, const size_t &size) 
    {
      const int items = (head >= tail) ? (head - tail) : (head + size - tail);
      // DBG("#items: " << items) 
      return items;
    }

    void advance (std::atomic<size_t> &item, const size_t &size) 
    {
      DBG("advance: " << item);
      int prev = item;
      item = (prev + 1) % size;
      DBG("advanced: " << item)
    }

    LV2_Worker_Status schedule_work (uint32_t size, const void *data) 
    {
      DBG_ENTER
      if (m_worker_quit == true) {
        DBG("quit!")
        return LV2_WORKER_ERR_UNKNOWN;
      }

      if (m_worker_interface) 
      {
        DBG("schedule_work")
        if (number_of_items (m_work_items_head, m_work_items_tail, m_work_items.size ()) < (int)m_work_items.size() - 1) 
        {
          if (size > HORST_WORK_ITEM_MAX_SIZE) return LV2_WORKER_ERR_NO_SPACE;

          auto &item = m_work_items[m_work_items_head];
          item.first = size;
          memcpy (&item.second[0], data, size);
          DBG_JACK("item: " << item.first << " " << (int)item.second[0])
          advance (m_work_items_head, m_work_items.size ());
          DBG_EXIT
          return LV2_WORKER_SUCCESS; // m_worker_interface->work (m_plugin_instance->m, horst::worker_respond, this, size, data);
        }
        DBG_EXIT
        return LV2_WORKER_ERR_NO_SPACE;
      }
      DBG_EXIT
      return LV2_WORKER_ERR_UNKNOWN;
    }

    LV2_Worker_Status worker_respond (uint32_t size, const void *data) 
    {
      DBG_ENTER
      if (m_worker_interface) 
      {
        DBG("respond.");
        if (size > HORST_WORK_ITEM_MAX_SIZE) return LV2_WORKER_ERR_NO_SPACE;
        auto &item = m_work_responses[m_work_responses_head];
        memcpy (&item.second[0], data, size);
        DBG("item: " << item.first << " " << (int)item.second[0])
        advance (m_work_responses_head, m_work_responses.size ());
      }
      DBG_EXIT
      return LV2_WORKER_SUCCESS;
    }

    void *worker_thread () 
    {
      // TODO: Use a condition variable to signal the worker thread instead of busy looping
      DBG_ENTER
      while (!m_worker_quit) 
      {
        LV2_Worker_Interface *interface = m_worker_interface;
        while (m_worker_interface && number_of_items (m_work_items_head, m_work_items_tail, m_work_items.size ())) 
        {
          DBG("getting to work: " << (void*)(interface->work) << " " << (void*)(interface->work_response) << " " << (void*)(interface->end_run))
          auto &item = m_work_items[m_work_items_tail];
          DBG("item: " << item.first << " " << (int)item.second[0])
          if (interface->work) 
          {
            DBG(m_plugin_instance->m)
            #ifdef HORST_DEBUG
            LV2_Worker_Status res = 
            #endif
              interface->work (m_plugin_instance->m_lv2_handle, &horst::worker_respond, (LV2_Worker_Respond_Handle)this, item.first, &item.second[0]);
            DBG("worker_thread: res: " << res)
          }
          advance (m_work_items_tail, m_work_items.size ());
        }
        usleep (10000);
      }
      DBG_EXIT
      return 0;
    }

    void save_state (const std::string &path) 
    {
      if (m_state_interface) 
      {
        m_state_interface->save (m_plugin_instance->m_lv2_handle, state_store, 0, 0, 0);
      }
      else
      {
        DBG("No state interface - not saving")
      }
    }

    void restore_state (const std::string &path) 
    {
      if (m_state_interface) 
      {
        m_state_interface->restore (m_plugin_instance->m_lv2_handle, state_retrieve, 0, 0, 0);
      }
      else
      {
        DBG("No state interface - not restoring")
      }
    }

    ~lv2_plugin () 
    {
      DBG_ENTER
      if (m_worker_required)
      {
        m_worker_quit = true;
        pthread_join (m_worker_thread, 0);
      }

      m_plugin_instance = lilv_plugin_instance_ptr();
      DBG_EXIT
    }
  };

  extern "C" 
  {
    LV2_URID urid_map (LV2_URID_Map_Handle handle, const char *uri) 
    {
      return ((lv2_plugin*)handle)->urid_map(uri);
    }

    LV2_Worker_Status schedule_work (LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data) 
    {
      return  ((lv2_plugin*)handle)->schedule_work (size, data);
    }

    LV2_Worker_Status worker_respond (LV2_Worker_Respond_Handle handle, uint32_t size, const void *data) 
    {
      return ((lv2_plugin*)handle)->worker_respond (size, data);
    }

    void *worker_thread (void *arg) 
    {
      return ((lv2_plugin*)arg)->worker_thread ();
    }

    LV2_State_Status state_store (LV2_State_Handle handle, uint32_t key, const void *value, size_t size, uint32_t type, uint32_t flags)
    {
      DBG_ENTER_EXIT
      DBG("key: " << key << " size: " << size << " type: " << type << " flags: " << flags)
      return LV2_STATE_ERR_UNKNOWN;
    }

    const void *state_retrieve (LV2_State_Handle handle, uint32_t key, size_t *size, uint32_t *type, uint32_t *flags)
    {
      DBG_ENTER_EXIT
      DBG("key: " << key << " size: " << size << " type: " << type << " flags: " << flags)
      return 0;
    }
  }


  typedef std::shared_ptr<lv2_plugin> lv2_plugin_ptr;
}
