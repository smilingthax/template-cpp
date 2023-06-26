#pragma once

#include "template_input.h"
#include <string>
#include <vector>

namespace Template {

namespace detail {
class Builder {
public:
  virtual ~Builder() {}

  virtual void text(std::string_view data) = 0;
  virtual void variable(std::string_view name, std::string_view modifiers = {}) = 0;

  virtual void enter_optional() = 0;
  virtual void leave_optional() = 0;

  virtual void enter_group(std::string_view name, std::string_view joiner) = 0;
  virtual void leave_group() = 0;

  virtual void finish() = 0;
};

void parse(Input &&in, Builder &tb);
} // namespace detail

enum struct part_type_e : char {
  text, variable, optional, group
};

struct part_t {
  part_t(part_type_e type, std::string_view text_name = {}, std::string_view modifiers_joiner = {})
    : text_name(text_name), modifiers_joiner(modifiers_joiner), type(type)
  { }

  std::string text_name;               // for text: text, for variable/group: name (not used for optional)
  std::string modifiers_joiner;        // for variable: modifiers, for group: joiner (not used for optional)
  std::vector<part_t> sub;             // only for optional/group
  part_type_e type;
  char unmerged_newline = 0;           // for optional/group
};

std::vector<part_t> parse_string(const std::string_view &sv);
std::vector<part_t> parse_file(const char *filename);

} // namespace Template

