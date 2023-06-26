#include "template_data.h"

namespace Template {
namespace detail {

map_init_t::map_init_t(const std::initializer_list<pair_init_t> &list)
  : list(&list), map(list.size())
{
  for (auto &it : list) {
    if (!map.emplace(it.key, it.value).second) {
      throw std::invalid_argument(std::string("duplicate key ").append(it.key));
    }
  }
}

} // namespace detail

Data::Data(const detail::map_init_t &&init)
  : data([&init] {
    if (init.list) {
      // NOTE: cannot reuse init.map here, because types do not match, esp. value_init_t -> value_t
      std::unordered_map<std::string, value_t> data(init.list->size());
      for (auto &it : *init.list) {
        // duplicate check already happened in map_init_t(list) ctor
        data.emplace(it.key, std::move(it.value)); // move because init is rvalue
      }
      return data;
    } else if (init.ctdata) { // deep copy
      return init.ctdata->data;
    } else {
      throw std::invalid_argument("bad map_init_t");
    }
  }())
{
}

Data::value_t::value_t(const detail::value_init_t &&init)
  : string(init.string), // "no-op" for init.is_list() (i.e. !init.string.data())
    list([&init]() -> List {
      if (!init.is_list()) {
        return {}; // "no-op"
      } else if (init.list.list) {
        return std::move(*init.list.list);
      } else if (init.list.ctlist) { // deep copy
        return *init.list.ctlist;
      } else {
        throw std::invalid_argument("bad value_init_t");
      }
    }())
{
  if (init.is_list() && !list.data.capacity()) {
    list.data.reserve(1); // trick/hack
    // assert(is_list());
  }
}

} // namespace Template

