#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/worker/worker.h"

#include <cstring>
#include <cstdlib>
#include <vector>

#define HORST_PLUGINS_WORKER_TEST_URI "https://dfdx.eu/plugins/horst-plugins/worker-test"

#define WORK_ITEM_SIZE (1024 * 1024)

struct worker_test
{
  std::vector<char> m_data;
  LV2_Worker_Schedule *m_schedule;

  worker_test() :
    m_data (WORK_ITEM_SIZE)
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
  worker_test *w = new worker_test;

  const char *missing = lv2_features_query
  (
    features,
    LV2_WORKER__schedule, &w->m_schedule,   true,
    0
  );

  return (LV2_Handle)w;
}

static void
run
(
  LV2_Handle instance,
  uint32_t sample_cout
)
{
  worker_test *w = (worker_test*)instance;

  w->m_schedule->schedule_work(w->m_schedule->handle, WORK_ITEM_SIZE, &(w->m_data[0]));
}

static LV2_Worker_Status
work
(
  LV2_Handle instance,
  LV2_Worker_Respond_Function respond,
  LV2_Worker_Respond_Handle handle,
  uint32_t size,
  const void* data
)
{
  if (size != WORK_ITEM_SIZE)
  {
    abort();
  }

  respond (handle, size, data);

  return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status
work_response
(
  LV2_Handle instance,
  uint32_t size,
  const void* data
)
{
  if (size != WORK_ITEM_SIZE)
  {
    abort();
  }

  return LV2_WORKER_SUCCESS;
}

static const void*
extension_data
(
  const char* uri
)
{
  static const LV2_Worker_Interface worker = {work, work_response, 0};

  if (!strcmp(uri, LV2_WORKER__interface)) {
    return &worker;
  }

  return 0;
}

static void cleanup
(
  LV2_Handle instance
)
{
  delete (worker_test*)instance;
}

static const LV2_Descriptor descriptor =
{
  HORST_PLUGINS_WORKER_TEST_URI,
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
