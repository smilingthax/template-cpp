#include "template_parser.h"
#include <stdexcept>

namespace Template {

namespace {
size_t parse_shortname(const std::string_view &sv)
{
  for (auto it = sv.begin(); it != sv.end(); ++it) {
    if (!isalnum(*it)) {
      return std::distance(sv.begin(), it);
    }
  }
  return 0;
}

size_t parse_braced(std::string &ret, const std::string_view &sv) // sv to start after '{'
{
  for (auto it = sv.begin(); it != sv.end(); ++it) {
    const char ch = *it;
    if (ch == '\\') {
      if (++it == sv.end()) {
        return 0;
      }
      // TODO? if (*it == 'n') { ret.push_back('\n'); } else ...   ?
      ret.push_back(*it);
    } else if (ch == '}') {
      return std::distance(sv.begin(), ++it); // (>= 1)
    } else {
      ret.push_back(ch);
    }
  }
  return 0;
}

void parse_command(std::string_view &sv, detail::Builder &tb)
{
  if (sv.empty()) {
    throw std::runtime_error("Incomplete $ sequence");
  }

  const char ch = sv.front();
  if (ch == '$') {
    tb.text({ sv.data(), 1 });
    sv.remove_prefix(1);

  } else if (ch == '(') { // optional
    tb.enter_optional();
    sv.remove_prefix(1);

  } else if (ch == ')') {
    tb.leave_optional();
    sv.remove_prefix(1);

  } else if (ch == '[') { // array
    sv.remove_prefix(1);
    const size_t pos = parse_shortname(sv);
    if (!pos) {
      throw std::runtime_error("Bad $[ sequence");
    }
    auto name = sv.substr(0, pos);
    sv.remove_prefix(pos);

    // parse joiner string
    // either { ... } or single character
    if (sv.front() == '{') {
      sv.remove_prefix(1);
      std::string joiner;
      const size_t pos = parse_braced(joiner, sv);
      if (!pos) {
        throw std::runtime_error("Bad $[...{...} sequence");
      }
      tb.enter_group(name, joiner);
      sv.remove_prefix(pos);
    } else {
      tb.enter_group(name, { sv.begin(), 1 });
      sv.remove_prefix(1);
    }

  } else if (ch == ']') {
    tb.leave_group();
    sv.remove_prefix(1);

  } else if (ch == '{') { // long varname (possibly with modifiers)
    // read until }
    // NOTE/TODO: must fit into ensured sv size...
    const size_t pos = sv.find('}');
    if (pos == sv.npos) {
      throw std::runtime_error("Could not find end of ${ ...");
    }
    const size_t mpos = sv.find(':');
    if (mpos < pos) { // (esp. != npos)
      tb.variable(sv.substr(1, mpos - 1), sv.substr(mpos + 1, pos - mpos - 1));
    } else {
      tb.variable(sv.substr(1, pos));
    }
    sv.remove_prefix(pos + 1);

  } else {
    const size_t pos = parse_shortname(sv);
    if (!pos) {
      throw std::runtime_error("Bad $ sequence");
    }
    tb.variable(sv.substr(0, pos));
    sv.remove_prefix(pos);
  }
}
} // namespace

void detail::parse(Input &&in, detail::Builder &tb)
{
  for (auto sv = in.get(); !sv.empty(); ) {
    const size_t pos = sv.find_first_of("$\n");
    if (pos == sv.npos) {
      tb.text(sv);
      sv = in.get(sv.size());
    } else if (sv[pos] == '\n') {
      tb.text(sv.substr(0, pos + 1));
      sv = in.get(pos + 1);
    } else { // (sv[pos] == '$')
      if (pos) {
        tb.text({ sv.data(), pos });
      }
      sv = in.get(pos + 1);
      parse_command(sv, tb);
      sv = in.get(sv);
    }
  }

  tb.finish();
}

// ---

namespace {
bool has_ending_newline(const part_t &part)
{
  switch (part.type) {
  case part_type_e::text:
    return (!part.text_name.empty() &&   // .ends_with('\n')
            part.text_name.back() == '\n');
  case part_type_e::variable:
    return false;
  case part_type_e::optional:
  case part_type_e::group:
    return (!!part.unmerged_newline);
  }
  throw std::logic_error("unreachable?!"); // gcc does not think that all cases were handled...
}

class Builder final : public detail::Builder {
public:
  void clear() {
    stack.clear();
    top = &data;
  }

  void text(std::string_view text) override {
    // assert(!text.empty());
    if (text.front() == '\n' && !top->empty()) {
      switch (top->back().type) {
      case part_type_e::text:
        break;
      case part_type_e::variable:
        break;
      case part_type_e::optional:
      case part_type_e::group:
        if (!top->back().unmerged_newline &&  // i.e. not already merged (and empty text was not added)
            top->size() >= 2) {
          if (has_ending_newline((*top)[top->size() - 2])) {
            top->back().unmerged_newline = text.front();
            text.remove_prefix(1);
            if (text.empty()) {
              return;
            }
          }
        }
        break;
      }
    }
    top->emplace_back(part_type_e::text, text);
  }
  void variable(std::string_view name, std::string_view modifiers = {}) override {
    top->emplace_back(part_type_e::variable, name, modifiers);
  }

  void enter_optional() override {
    auto &elem = top->emplace_back(part_type_e::optional);  // retval: we certainly have c++17 (string_view!)
    stack.emplace_back(elem);
    top = &elem.sub;
  }
  void leave_optional() override {
    if (stack.empty() ||
        stack.back().part.type != part_type_e::optional) {
      throw std::runtime_error("no matching $( for $)");
    }
    stack.pop_back();
    top = (!stack.empty()) ? &stack.back().part.sub : &data;
  }

  void enter_group(std::string_view name, std::string_view joiner) override {
    auto &elem = top->emplace_back(part_type_e::group, name, joiner);
    stack.emplace_back(elem);
    top = &elem.sub;
  }
  void leave_group() override {
    if (stack.empty() ||
        stack.back().part.type != part_type_e::group) {
      throw std::runtime_error("no matching $[ for $]");
    }
    stack.pop_back();
    top = (!stack.empty()) ? &stack.back().part.sub : &data;
  }

  void finish() override {
    if (!stack.empty()) {
      if (stack.back().part.type == part_type_e::optional) {
        throw std::runtime_error("opened $( not closed");
      } else {
        throw std::runtime_error("opened $[ not closed");
      }
    } // else assert(top == &data);
  }

  std::vector<part_t> get() {
    return std::move(data);
  }

private:
  struct stack_t {
    stack_t(part_t &part) : part(part) { }
    part_t &part;
  };
  std::vector<stack_t> stack;
  std::vector<part_t> data;
  std::vector<part_t> *top = &data;
};
} // namespace

std::vector<part_t> parse_string(const std::string_view &sv)
{
  Builder builder;
  detail::parse(StringInput(sv), builder);
  return builder.get();
}

std::vector<part_t> parse_file(const char *filename)
{
  Builder builder;
  detail::parse(FileInput(filename), builder);
  return builder.get();
}

} // namespace Template

