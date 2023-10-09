#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <horst/horst.h>

namespace bp = pybind11;

PYBIND11_MODULE(lv2_horst, m)
{
  bp::class_<horst::lilv_world, horst::lilv_world_ptr> (m, "lilv_world")
    .def (bp::init<> ())
  ;

  bp::class_<horst::lilv_plugins, horst::lilv_plugins_ptr> (m, "lilv_plugins")
    .def (bp::init<horst::lilv_world_ptr> ())
    .def ("get_uris", &horst::lilv_plugins::get_uris)
  ;

  bp::class_<horst::lilv_uri_node, horst::lilv_uri_node_ptr> (m, "lilv_uri_node")
    .def (bp::init<horst::lilv_world_ptr, std::string> ())
  ;

  bp::class_<horst::lilv_plugin, horst::lilv_plugin_ptr> (m, "lilv_plugin")
    .def (bp::init<horst::lilv_plugins_ptr, horst::lilv_uri_node_ptr> ())
  ;

  bp::class_<horst::plugin, horst::plugin_ptr> (m, "plugin")
    .def (bp::init<horst::lilv_world_ptr, horst::lilv_plugins_ptr, std::string> ())
    .def ("get_name", &horst::plugin::get_name)
    .def ("instantiate", &horst::plugin::instantiate)
    .def ("run", &horst::plugin::run)
    .def ("urid_map", &horst::plugin::urid_map)
    .def ("urid_unmap", &horst::plugin::urid_unmap)
    .def ("save_state", &horst::plugin::save_state)
    .def ("restore_state", &horst::plugin::restore_state)
  ;

  bp::class_<horst::port_properties>(m, "port_properties", bp::dynamic_attr ())
    .def_readonly ("is_audio", &horst::port_properties::m_is_audio)
    .def_readonly ("is_control", &horst::port_properties::m_is_control)
    .def_readonly ("is_cv", &horst::port_properties::m_is_cv)
    .def_readonly ("is_input", &horst::port_properties::m_is_input)
    .def_readonly ("is_output", &horst::port_properties::m_is_output)
    .def_readonly ("is_side_chain", &horst::port_properties::m_is_side_chain)
    .def_readonly ("minimum_value", &horst::port_properties::m_minimum_value)
    .def_readonly ("default_value", &horst::port_properties::m_default_value)
    .def_readonly ("maximum_value", &horst::port_properties::m_maximum_value)
    .def_readonly ("is_logarithmic", &horst::port_properties::m_is_logarithmic)
    .def_readonly ("name", &horst::port_properties::m_name)
  ;

  bp::class_<horst::connection>(m, "connection")
    .def (bp::init<const std::string&, const std::string&> ())
  ;

  void (horst::connections::*add1)(const std::string &, const std::string &) = &horst::connections::add;
  void (horst::connections::*add2)(const horst::connection &) = &horst::connections::add;
  bp::class_<horst::connections>(m, "connections")
    .def (bp::init<> ())
    .def ("add", add1)
    .def ("add", add2)
  ;

  bp::class_<horst::connection_manager>(m, "connection_manager")
    .def (bp::init<std::string>(), bp::arg("jack_client_name") = "connection_manager")
    .def ("connect", &horst::connection_manager::connect)
    .def ("disconnect", &horst::connection_manager::disconnect)
  ;

  bp::class_<horst::midi_binding>(m, "midi_binding")
    .def (
      bp::init<bool, int, int, float, float>(), 
      bp::arg ("enabled") = false, bp::arg ("channel") = 0, bp::arg ("cc") = 0, bp::arg ("factor") = 1.0f, bp::arg ("offset") = 0.0f
    )
  ;

  bp::class_<horst::unit, horst::unit_ptr> (m, "unit", bp::dynamic_attr ())
    .def (bp::init<horst::plugin_ptr, std::string, bool>())
    .def ("set_control_port_value", &horst::unit::set_control_port_value)
    .def ("get_control_port_value", &horst::unit::get_control_port_value)
    .def ("set_midi_binding", &horst::unit::set_midi_binding)
    .def ("get_midi_binding", &horst::unit::get_midi_binding)
    .def ("get_number_of_ports", &horst::unit::get_number_of_ports)
    .def ("get_port_properties", &horst::unit::get_port_properties)
    .def ("set_enabled", &horst::unit::set_enabled)
    .def ("set_control_input_updates_enabled", &horst::unit::set_control_input_updates_enabled)
    .def ("set_control_output_updates_enabled", &horst::unit::set_control_output_updates_enabled)
    .def ("set_audio_input_monitoring_enabled", &horst::unit::set_audio_input_monitoring_enabled)
    .def ("set_audio_output_monitoring_enabled", &horst::unit::set_audio_output_monitoring_enabled)
    .def ("get_jack_client_name", &horst::unit::get_jack_client_name)
    .def ("save_state", &horst::unit::save_state)
    .def ("restore_state", &horst::unit::restore_state)
  ;
}
