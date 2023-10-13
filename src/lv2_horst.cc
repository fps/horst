#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <lv2_horst/jacked_horst.h>
#include <lv2_horst/connection.h>

namespace bp = pybind11;

PYBIND11_MODULE(lv2_horst, m)
{
  bp::class_<lv2_horst::lv2_plugins, lv2_horst::lv2_plugins_ptr> (m, "lv2_plugins")
    .def (bp::init<>())
    .def ("get_uris", &lv2_horst::lv2_plugins::get_uris)
  ;

  bp::class_<lv2_horst::horst, lv2_horst::horst_ptr> (m, "horst")
    .def (bp::init<lv2_horst::lv2_plugins_ptr, const std::string&> ())
    .def ("get_name", &lv2_horst::horst::get_name)
    .def ("instantiate", &lv2_horst::horst::instantiate)
    .def ("run", &lv2_horst::horst::run)
    .def ("urid_map", &lv2_horst::horst::urid_map)
    .def ("urid_unmap", &lv2_horst::horst::urid_unmap)
    .def ("save_state", &lv2_horst::horst::save_state)
    .def ("restore_state", &lv2_horst::horst::restore_state)
  ;

  bp::class_<lv2_horst::port_properties>(m, "port_properties", bp::dynamic_attr ())
    .def_readonly ("is_audio", &lv2_horst::port_properties::m_is_audio)
    .def_readonly ("is_control", &lv2_horst::port_properties::m_is_control)
    .def_readonly ("is_cv", &lv2_horst::port_properties::m_is_cv)
    .def_readonly ("is_input", &lv2_horst::port_properties::m_is_input)
    .def_readonly ("is_output", &lv2_horst::port_properties::m_is_output)
    .def_readonly ("is_side_chain", &lv2_horst::port_properties::m_is_side_chain)
    .def_readonly ("minimum_value", &lv2_horst::port_properties::m_minimum_value)
    .def_readonly ("default_value", &lv2_horst::port_properties::m_default_value)
    .def_readonly ("maximum_value", &lv2_horst::port_properties::m_maximum_value)
    .def_readonly ("is_logarithmic", &lv2_horst::port_properties::m_is_logarithmic)
    .def_readonly ("name", &lv2_horst::port_properties::m_name)
  ;

  bp::class_<lv2_horst::connection_manager>(m, "connection_manager")
    .def (bp::init<std::string>(), bp::arg("jack_client_name") = "lv2_horst_connection_manager")
    .def ("connect", &lv2_horst::connection_manager::connect, bp::arg("the_connections"), bp::arg("throw_on_error") = false)
    .def ("disconnect", &lv2_horst::connection_manager::disconnect)
  ;

  bp::class_<lv2_horst::midi_binding>(m, "midi_binding")
    .def (
      bp::init<bool, int, int, float, float>(), 
      bp::arg ("enabled") = false, bp::arg ("channel") = 0, bp::arg ("cc") = 0, bp::arg ("factor") = 1.0f, bp::arg ("offset") = 0.0f
    )
  ;

  bp::class_<lv2_horst::jacked_horst, lv2_horst::jacked_horst_ptr> (m, "jacked_horst", bp::dynamic_attr ())
    .def (bp::init<lv2_horst::lv2_plugins_ptr, const std::string&, const std::string&, bool>(), bp::arg("plugins"), bp::arg("uri"), bp::arg("jack_client_name") = "", bp::arg("expose_control_ports") = false)
    .def ("set_control_port_value", &lv2_horst::jacked_horst::set_control_port_value)
    .def ("get_control_port_value", &lv2_horst::jacked_horst::get_control_port_value)
    .def ("set_midi_binding", &lv2_horst::jacked_horst::set_midi_binding)
    .def ("get_midi_binding", &lv2_horst::jacked_horst::get_midi_binding)
    .def ("get_number_of_ports", &lv2_horst::jacked_horst::get_number_of_ports)
    .def ("get_port_properties", &lv2_horst::jacked_horst::get_port_properties)
    .def ("set_enabled", &lv2_horst::jacked_horst::set_enabled)
    .def ("set_control_input_updates_enabled", &lv2_horst::jacked_horst::set_control_input_updates_enabled)
    .def ("set_control_output_updates_enabled", &lv2_horst::jacked_horst::set_control_output_updates_enabled)
    .def ("set_audio_input_monitoring_enabled", &lv2_horst::jacked_horst::set_audio_input_monitoring_enabled)
    .def ("set_audio_output_monitoring_enabled", &lv2_horst::jacked_horst::set_audio_output_monitoring_enabled)
    .def ("get_jack_client_name", &lv2_horst::jacked_horst::get_jack_client_name)
    .def ("save_state", &lv2_horst::jacked_horst::save_state)
    .def ("restore_state", &lv2_horst::jacked_horst::restore_state)
  ;
}
