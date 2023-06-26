#pragma once

#include <string_view>

namespace Template {

class Input {
public:
  Input() = default;
  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;
  virtual ~Input() {}

  std::string_view get(size_t eat=0, size_t ensure=std::string_view::npos);
  std::string_view get(std::string_view sv, size_t ensure=std::string_view::npos);
protected:
  virtual size_t _read(char *buf, size_t len) = 0;
  bool refill(size_t ensure);
private:
  char buf[1024];
  size_t pos = 0, fill = 0;
};

class StringInput : public Input {
public:
  StringInput(const std::string_view &sv) : sv(sv) {} // not copied!
protected:
  size_t _read(char *buf, size_t len) override;
private:
  std::string_view sv;
};

class FileInput : public Input {
public:
  FileInput(FILE *f); // takes!
  FileInput(const char *filename);

  ~FileInput();
protected:
  size_t _read(char *buf, size_t len) override;
private:
  FILE *f;
};

} // namespace Template

