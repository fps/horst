#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/state/state.h"

#include <cstring>
#include <cstdlib>
#include <vector>

#define HORST_PLUGINS_STATE_TEST_URI "https://dfdx.eu/plugins/horst-plugins/state-test"

#define STATE_ITEM_SIZE

struct state_test
{
  std::vector<char> m_data;

  state_test() :
    m_data (STATE_ITEM_SIZE)
  {

  }
};

static LV2_Handle
instantiate
(
  const LV2_Descriptor* descriptor,
  double rate,
  const char* path,
  const LV2_Feature* const* features
)
{
  state_test *w = new state_test;

  /*
  const char *missing = lv2_features_query
  (
    features,
    LV2_STATE__interface, &w->m_schedule,   true,
    0
  );
  */
  return (LV2_Handle)w;
}

static void
run
(
  LV2_Handle instance,
  uint32_t sample_cout
)
{
  // state_test *w = (state_test*)instance;
}


static const void*
extension_data
(
  const char* uri
)
{
  /*
  static const LV2_STATE__interface worker = {work, work_response, 0};

  if (!strcmp(uri, LV2_STATE__interface)) {
    return &worker;
  }
  */
  return 0;
}

static void cleanup
(
  LV2_Handle instance
)
{
  delete (state_test*)instance;
}

static const LV2_Descriptor descriptor =
{
  HORST_PLUGINS_STATE_TEST_URI,
  instantiate,
  0,
  0,
  run,
  0,
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor
(
  uint32_t index
)
{
  return index == 0 ? &descriptor : 0;
}
