#include <lv2_horst/ringbuffer.h>

#define CAPACITY 16

int main ()
{
    lv2_horst::continuous_ringbuffer<size_t> rb (CAPACITY + 1);

    // Write and read some stuff until we're in the middle of the ringbuffer
    for (size_t index = 0; index < (CAPACITY / 2); ++index)
    {
      rb.write(index);
      size_t data = rb.read();
      std::cout << data << "\n";
      rb.report_status ();
    }

    size_t data[CAPACITY];

    for (size_t index = 0; index < CAPACITY; ++index)
    {
      data[index] = index;
    }

    rb.write (data, CAPACITY);
    rb.read (data);

    for (size_t index = 0; index < CAPACITY; ++index)
    {
      std::cout << data[index] << "\n";
    }

    return 0;
}
