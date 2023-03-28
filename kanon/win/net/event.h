#ifndef KANON_WIN_NET_EVENT_H__
#define KANON_WIN_NET_EVENT_H__

namespace kanon {

enum Event /* : unsigned char */ {
  NoneEvent = 0,
  ReadEvent = 0x1,
  WriteEvent = 0x2,
  WakeEvent = 0x4,
};

} // namespace kanon

#endif