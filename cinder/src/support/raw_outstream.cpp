#include "cinder/support/raw_outstream.hpp"
using namespace ostream;

RawOutStream::RawOutStream(FD fd) : fd(fd) {}

RawOutStream::~RawOutStream() {
  FlushBuffer();
}

void RawOutStream::FlushBuffer() {
  if (pos > 0) {
    ::write(2, buffer, pos);
    pos = 0;
  }
}

RawOutStream& RawOutStream::Write(const char* data, size_t size) {
  if (size > BUF_SIZE) {
    FlushBuffer();
    ::write(fd, data, size);
    return *this;
  }
  if (pos + size > BUF_SIZE) {
    FlushBuffer();
  }
  std::memcpy(buffer + pos, data, size);
  pos += size;
  return *this;
}
