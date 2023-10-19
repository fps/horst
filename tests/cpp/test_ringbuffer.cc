#include <lv2_horst/ringbuffer.h>

#define CAPACITY 16

int main ()
{
    lv2_horst::ringbuffer<size_t> rb (CAPACITY);
    
    for (size_t index = 0; index < CAPACITY; ++index)
    {
        DBG("index: " << index)
        rb.report_status ();
        rb.write(index);
        rb.report_status ();
    }

    for (size_t index = 0; index < CAPACITY/2; ++index)
    {
        DBG("index: " << index)
        rb.report_status ();
        const size_t d = rb.read ();
        DBG("d: " << d)
        rb.report_status ();
    }

    return 0;
}
