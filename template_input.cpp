#include "template_input.h"
#include <system_error>
#include <stdexcept>
#include <string.h>

namespace Template {

std::string_view Input::get(size_t eat, size_t ensure)
{
  if (eat == std::string_view::npos) {
    eat = fill - pos;
  } else if (eat > fill - pos) {
    throw std::invalid_argument("get(eat) must be <= fill");
  }
  pos += eat;

  refill(ensure);
  return { buf + pos, fill - pos };
}

std::string_view Input::get(std::string_view sv, size_t ensure)
{
  if (sv.data() < buf + pos || sv.data() + sv.size() != buf + fill) {
    throw std::invalid_argument("get(sv) is not a suffix of the current get()-view");
  }
  return get(sv.data() - (buf + pos), ensure);
}

bool Input::refill(size_t ensure)
{
  // assert(pos <= fill);
  const size_t size = fill - pos;
  if (size == 0) {
    pos = 0;
    fill = 0;
  } else if (ensure == std::string_view::npos || size < ensure) {
    memmove(buf, buf + pos, fill - pos);
    fill -= pos;
    pos = 0;
  }

  const size_t len = _read(buf + pos, sizeof(buf) - fill);
  fill += len;
  return (fill > pos);
}


size_t StringInput::_read(char *buf, size_t len)
{
  const size_t ret = std::min(sv.size(), len);
  sv.copy(buf, ret);
  sv.remove_prefix(ret);
  return ret;
}


FileInput::FileInput(FILE *f) // takes!
  : f(f)
{
  if (!f) {
    throw std::invalid_argument("FILE * must not be nullptr");
  }
}

FileInput::FileInput(const char *filename)
{
  f = fopen(filename, "r");
  if (!f) {
    throw std::system_error(errno, std::generic_category(), std::string("Failed to open file: ").append(filename));
  }
}

FileInput::~FileInput()
{
  fclose(f);
}

size_t FileInput::_read(char *buf, size_t len)
{
  // assert(buf);
  return fread(buf, 1, len, f);
}

} // namespace Template

