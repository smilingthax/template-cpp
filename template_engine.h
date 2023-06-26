#pragma once

#include "template_data.h"

namespace Template {

struct part_t;

class Engine {
public:
  Engine(std::vector<part_t> parts);
  ~Engine();

  static Engine fromString(const std::string_view &sv);
  static Engine fromFile(const char *filename);

  void render(const detail::map_init_t &map) const;

  std::string toString(const detail::map_init_t &map) const;
#if 0
  void toFile(const char *filename) const;
#endif

  void printvar() const {
    do_printvar(parts);
  }

private:
  static void do_printvar(const std::vector<part_t> &parts, const std::string &indent = {});

  struct render_context_t;

  std::vector<part_t> parts;
};

} // namespace Template

