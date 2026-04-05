#include <cstddef>
#include <cstdio>

#include "console.hpp"
#include "font.hpp"
#include "graphics.hpp"

void *operator new(size_t size, void *buf) { return buf; }

void operator delete(void *obj) noexcept {}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;
extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config) {
  switch (frame_buffer_config.pixel_format) {
  case kPixelRGBResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter(frame_buffer_config);
    break;
  case kPixelBGRResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter(frame_buffer_config);
    break;
  }

  for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
    for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
      pixel_writer->Write(x, y, {255, 255, 255});
    }
  }

  for (int x = 0; x < 400; ++x) {
    for (int y = 0; y < 200; ++y) {
      pixel_writer->Write(x, y, {0, 255, 0});
    }
  }

  // WriteAscii(*pixel_writer, 50, 50, 'A', {0,0,0});
  // WriteAscii(*pixel_writer, 58, 50, 'Z', {0,0,0});
  // WriteString(*pixel_writer, 58, 50 - 18 * 2, "Hello! World!", {0, 0, 0});

  /*
  char buf[128];
  sprintf(buf, "1 + 2 = %d", 1 + 2);
  WriteString(*pixel_writer, 58, 50 - 18, buf, {0, 0, 0});

  for (int i = 0; i < 128; ++i) {
    char c = static_cast<char>(i);
    int x_off = i % 32;
    int y_off = i / 32;
    WriteAscii(*pixel_writer, 50 + (x_off * 8), 50 + (y_off * 16), c,
               {0, 0, 0});
  }
  */

  Console console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

  char buf[128];
  for (int i = 0; i < 27; ++i) {
    sprintf(buf, "line %d\n", i);
    console.PutString(buf);
  }

  while (1)
    __asm__("hlt");
}
