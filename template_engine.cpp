#include "template_engine.h"
#include "template_parser.h"
#include <algorithm>

namespace Template {
Engine::Engine(std::vector<part_t> parts)
 : parts(std::move(parts))
{
}

Engine::~Engine() = default;

Engine Engine::fromString(const std::string_view &sv)
{
  return parse_string(sv);
}

Engine Engine::fromFile(const char *filename)
{
  return parse_file(filename);
}

struct Engine::render_context_t {
  void render(const std::vector<part_t> &parts, const detail::map_init_t &map) {
    map.visit_mapctx([this, &parts](const auto &map_ctx) {
      for (const auto &part : parts) {
        render_one(part, map_ctx);
      }
    });
  }

private:
  template <typename MapCtxT>
  void render_one(const part_t &part, const MapCtxT &map_ctx);

  void add_indent(std::string_view sv, char unmerged_newline = 0) {
    if (unmerged_newline) {
      indent.clear();
      return;
    }

    // convert text to "space", but keep tabs...
    const size_t pos = sv.rfind('\n');
    if (pos != sv.npos) {
      sv.remove_prefix(pos + 1);
      indent.clear();
    }

    const size_t dpos = indent.size();
    indent.resize(dpos + sv.size(), ' ');
    for (size_t i = 0; i < sv.size(); i++) {
      if (sv[i] == '\t') {
        indent[dpos + i] = sv[i];
      }
    }
  }

  // does add_indent internally!
  void out_indent(const std::string_view &sv) {
    size_t pos = sv.find('\n'), next;
    if (pos == sv.npos) {
      out(sv);
      add_indent(sv);
    } else {
      // "explode()", but keep delimiter at the ends
#if 1  // indent all lines
      out(sv.substr(0, pos + 1));
      out(indent);
      while ((next = sv.find('\n', pos + 1)) != sv.npos) {
        out(sv.substr(pos + 1, next - pos));
        out(indent);
        pos = next;
      }
      out(sv.substr(pos + 1));
      add_indent(sv.substr(pos + 1));
#else  // only indent non-empty lines
      out(sv.substr(0, pos + 1));
      while ((next = sv.find('\n', pos + 1)) != sv.npos) {
        if (pos + 1 < next) { // no indent for empty lines
          out(indent);
          out(sv.substr(pos + 1, next - pos));
        }
        pos = next;
      }
      if (pos + 1 < sv.size()) { // no indent for empty last line  // TODO? only when variable part is directly followed by newline ?
        out(indent);
        out(sv.substr(pos + 1));
        add_indent(sv.substr(pos + 1));
      } else {
        indent.clear();
      }
#endif
    }
  }

  void out_newline(char unmerged_newline) {
    if (unmerged_newline) {
      out({ &unmerged_newline, 1 });
    }
  }

  void out(const std::string_view &sv) {
    printf("%.*s", (int)sv.size(), sv.data());  // FIXME...
  }

  void warn(const std::string_view &sv) {
    fprintf(stderr, "Warning: %.*s\n", (int)sv.size(), sv.data());
  }

  // Output ...
  std::string indent = {};
};

// MapCtxT<...> {
//   bool has(const std::string &name) const;
//   std::string_view get_string(const std::string &name) const;
//   {const list_init_t &, list_init_t} get_list(const std::string &name) const;
// }
template <typename MapCtxT>
void Engine::render_context_t::render_one(const part_t &part, const MapCtxT &map_ctx)
{
  switch (part.type) {
  case part_type_e::text:
    out(part.text_name);
    add_indent(part.text_name);
    break;

  case part_type_e::variable: {
      auto sv = map_ctx.get_string(part.text_name);
      if (sv.data()) {
// FIXME: part.modifiers_joiner -> formatter
        out_indent(sv);  // calls add_indent internally
      } else {
        warn(std::string("Variable '").append(part.text_name).append("' not found"));
      }
    }
    break;

  case part_type_e::optional: {
      // check whether ALL variables inside are given in map
      const bool full = std::all_of(part.sub.begin(), part.sub.end(), [&map_ctx](const auto &p) {
        return (p.type == part_type_e::text || p.type == part_type_e::optional || map_ctx.has(p.text_name));
      });

      if (full) {
        for (const auto &p : part.sub) {
          render_one(p, map_ctx);
        }
        out_newline(part.unmerged_newline);
        add_indent({}, part.unmerged_newline);
      }
    }
    break;

  case part_type_e::group: {
      const auto &list = map_ctx.get_list(part.text_name);
      if (!list.empty()) {
        size_t i = 0;
        list.visit([this, &part, &i](const Template::detail::map_init_t &submap) {
          if (i++ > 0) { // add joiner
            out(part.modifiers_joiner);
            add_indent(part.modifiers_joiner);
          }
          render(part.sub, submap);
        });
        if (i) {
          out_newline(part.unmerged_newline);
          add_indent({}, part.unmerged_newline);
        }
      } else {
        warn(std::string("Group Variable '").append(part.text_name).append("' not found"));
      }
    }
    break;
  }
}

void Engine::render(const detail::map_init_t &map) const
{
  render_context_t ctx;
  ctx.render(parts, map);
}

void Engine::do_printvar(const std::vector<part_t> &parts, const std::string &indent)
{
  for (const auto &part : parts) {
    switch (part.type) {
    case part_type_e::text:
      break;
    case part_type_e::variable:
      printf("%s%s\n", indent.c_str(), part.text_name.c_str());
      break;
    case part_type_e::optional:
      printf("%s-\n", indent.c_str());
      do_printvar(part.sub, indent + "  ");
      break;
    case part_type_e::group:
      printf("%s-%s\n", indent.c_str(), part.text_name.c_str());
      do_printvar(part.sub, indent + "  ");
      break;
    }
  }
}

} // namespace Template

