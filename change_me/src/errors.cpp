#include "../include/errors.hpp"
using namespace errs;

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
    ::write(2, data, size);
    return *this;
  }
  if (pos + size > BUF_SIZE) {
    FlushBuffer();
  }
  std::memcpy(buffer + pos, data, size);
  pos += size;
  return *this;
}