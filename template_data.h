#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

namespace Template {

struct Data;
struct List;

namespace detail {

struct map_init_t;
struct value_init_t;
struct pair_init_t;

struct list_init_t {
private:
  friend struct value_init_t;
  template <typename> friend class map_ctx_t;

  list_init_t() = default;
  list_init_t(list_init_t &&) = default;   // (+ deletes list_init_t(const list_init_t &) )
public:

  list_init_t(const std::initializer_list<map_init_t> &list)
    : list(&list) { }

  list_init_t(const List &ctlist)
    : ctlist(&ctlist) { }

  // visitor(const map_init_t &)
  template <typename Visitor>
  void visit(Visitor&& visitor) const;

  // empty list_init_t is only visible via map_init_t::visit_mapctx...
  bool empty() const {
    return (!list && !ctlist);
  }

private:
  friend struct ::Template::Data;

  const std::initializer_list<map_init_t> *list = nullptr;
  const List *ctlist = nullptr;
};

struct map_init_t {
  map_init_t(const std::initializer_list<pair_init_t> &list);

  map_init_t(const Data &ctdata)
    : ctdata(&ctdata) { }

  map_init_t(const map_init_t &) = delete;

  // visitor(std::string_view key, std::string_view value)
  // visitor(std::string_view key, const list_init_t &value)
  template <typename Visitor>
  void visit(Visitor&& visitor) const;

  // needed for Engine::render_context_t
  // visitor(const map_ctx_t<std::unordered_map<std::string_view, const detail::value_init_t &>> &map_ctx)
  // visitor(const map_ctx_t<std::unordered_map<std::string, Data::value_t>> &map_ctx)
  template <typename Visitor>
  void visit_mapctx(Visitor&& visitor) const;

private:
  friend struct ::Template::Data;

  const std::initializer_list<pair_init_t> *list = nullptr;
  const Data *ctdata = nullptr;

  std::unordered_map<std::string_view, const value_init_t &> map; // only when (list != nullptr)
};

struct value_init_t {
  value_init_t(std::string_view string)
    : string(string) {
    if (!string.data()) {
      throw std::invalid_argument("value_init_t string may not be nullptr");
    }
  }

  value_init_t(list_init_t &&list) // only allow temporaries / implicit conversion
    : list(std::move(list)) { }

  value_init_t(const value_init_t &) = delete;

  inline bool is_list() const {
    return !string.data();
  }

private:
  friend struct ::Template::Data;
  template <typename Visitor> friend void map_init_t::visit(Visitor &&) const;
  template <typename> friend class map_ctx_t;

  std::string_view string;
  list_init_t list;
};

struct pair_init_t {
  pair_init_t(std::string_view key, std::string_view string)
    : key(key), value(string) { }

  pair_init_t(std::string_view key, const std::initializer_list<map_init_t> &list)
    : key(key), value(list) { }

  pair_init_t(std::string_view key, const List &list)
    : key(key), value(list) { }

private:
  friend struct ::Template::Data;
  friend struct map_init_t;

  std::string_view key;
  value_init_t value;
};

// MapT = std::unordered_map<std::string_view, const value_init_t &>
//     or std::unordered_map<std::string, Data::value_t>
template <typename MapT>
class map_ctx_t {
  const MapT &map;

  explicit map_ctx_t(const MapT &map) : map(map) { }
  friend struct map_init_t;

  static constexpr const bool is_init = std::is_same_v<decltype(map.begin()->second.list), list_init_t>;
public:

  bool has(const std::string &name) const {
    return (map.find(name) != map.end());
  }

  // not found: !.data()
  std::string_view get_string(const std::string &name) const {
    auto it = map.find(name);
    if (it == map.end()) {
      return {}; // -> (!.data())
    } else if (it->second.is_list()) {
      throw std::runtime_error(std::string("Expected String, got List for variable '").append(name).append("'"));
    }
    // assert(it->second.string.data()); // via value_init_t ctor / value_t std::string
    return it->second.string;
  }

  // not found: .empty()
  auto get_list(const std::string &name) const
    -> std::conditional_t<is_init, const list_init_t &, list_init_t> {
    auto it = map.find(name);
    if (it == map.end()) {
      if constexpr (is_init) {
        static constexpr const list_init_t none;
        return none; // -> (.empty())
      } else {
        return {}; // -> (.empty())
      }
    } else if (!it->second.is_list()) {
      throw std::runtime_error(std::string("Expected List, got String for group '").append(name).append("'"));
    }
    // it->second.list = const list_init_t &  (contains either initializer_list<map_init_t>, or List)
    //                or const List &         (only contains Data/List)
    return it->second.list;
  }
};

} // namespace detail

struct List {
  List() = default;

  List(const std::initializer_list<detail::map_init_t> &&list) {
    data.reserve(list.size());
    for (auto &it : list) {
      data.emplace_back(std::move(it));
    }
  }

  void add(Data &&map) { // catches implicit conversions
    data.emplace_back(std::move(map));
  }

private:
  friend struct Data; // (actually Data::value_t)
  template <typename Visitor> friend void detail::list_init_t::visit(Visitor&&) const;

  std::vector<Data> data;
};

struct Data {
  Data(const std::initializer_list<detail::pair_init_t> &&list) {
    for (auto &it : list) {
      if (!data.emplace(it.key, std::move(it.value)).second) {
        throw std::invalid_argument(std::string("duplicate key ").append(it.key));
      }
    }
  }

  Data(const detail::map_init_t &&init);

  // NOTE: unlike ctor, this always overwrites (and does not detect duplicates within list)
  void set(const std::initializer_list<detail::pair_init_t> &&list) {
    for (auto &it : list) {
      data.insert_or_assign(std::string(it.key), std::move(it.value));
    }
  }
  // (cf. pair_init_t)
  void set(std::string_view key, std::string_view string) {
    data.insert_or_assign(std::string(key), detail::value_init_t(string));
  }
  void set(std::string_view key, const std::initializer_list<detail::map_init_t> &list) {
    data.insert_or_assign(std::string(key), detail::value_init_t(list));
  }
  void set(std::string_view key, const List &list) {
    data.insert_or_assign(std::string(key), detail::value_init_t(list));
  }

  void add_list(std::string_view key, Data &&map) {
    get_list(key).add(std::move(map));
  }

  // NOTE: data access via map_init_t::visit() !

private:
  friend struct List;   // (needed for friend struct List::value_t)
  friend struct detail::map_init_t;  // visit, visit_mapctx

  List &get_list(std::string_view key) { // created, when missing
    auto [it, inserted] = data.emplace(key, detail::list_init_t({}));
    if (!inserted && !it->second.is_list()) {
      throw std::invalid_argument(std::string("key '").append(key).append("' is not a List"));
    }
    return it->second.list;
  }

  struct value_t {
    value_t(const detail::value_init_t &&init);

    inline bool is_list() const {
      return (list.data.capacity() > 0);
    }

    // NOTE: naming must match with value_init_t for map_ctx_t<...>
    std::string string;
    List list;
  };

  std::unordered_map<std::string, value_t> data;
};

template <typename Visitor>
void detail::list_init_t::visit(Visitor&& visitor) const
{
  if (list) {
    for (const auto &it : *list) {
      visitor(it);
    }
  } else if (ctlist) {
    for (const auto &it : ctlist->data) {
      visitor(map_init_t(it));
    }
  } // else: empty -> no-op
}

template <typename Visitor>
void detail::map_init_t::visit(Visitor&& visitor) const
{
  if (list) {
    for (auto &it : *list) {
      if (!it.value.is_list()) {
        visitor(it.key, it.value.string);
      } else {
        visitor(it.key, it.value.list);
      }
    }
  } else if (ctdata) {
    for (auto &it : ctdata->data) {
      if (!it.second.is_list()) {
        visitor(it.first, it.second.string);
      } else {
        visitor(it.first, list_init_t(it.second.list));
      }
    }
  } // else: assert(0);  // (no ctor that would allow this)
}

template <typename Visitor>
void detail::map_init_t::visit_mapctx(Visitor&& visitor) const
{
  if (list) {
    visitor(map_ctx_t(map));
  } else if (ctdata) {
    visitor(map_ctx_t(ctdata->data));
  } // else: assert(0);
}

} // namespace Template

