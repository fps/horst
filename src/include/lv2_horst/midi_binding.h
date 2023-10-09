#include <lv2_horst/dbg.h>

namespace lv2_horst
{
  struct midi_binding
  {
    bool m_enabled;
    int m_channel;
    int m_cc;
    float m_factor;
    float m_offset;

    midi_binding
    (
      bool enabled = false,
      int channel = 0,
      int cc = 0,
      float factor = 1.0f,
      float offset = 0.0f
    ) :
      m_enabled (enabled),
      m_channel (channel),
      m_cc (cc),
      m_factor (factor),
      m_offset (offset)
    {
      DBG("enabled: " << enabled << " channel: " << channel << " cc: " << cc << " factor: " << factor << " offset: " << offset)
    }
  };
}
