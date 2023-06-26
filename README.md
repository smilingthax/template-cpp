# Simple, but powerful, C++(17) template engine

Usage:

```
#include "template_engine.h"

...
  // or: ... = Template::Engine::fromFile("filename.tmpl");
  auto tmpl = Template::Engine::fromString(R"(
  )");

  tmpl.render({
    {"a", "1"},
    {"b", "2"},
    {"c", "3"},
    {"er", "{}"},
    {"efg", {
      { {"h", "H"} },
      {
        {"x","3"},
        {"y","4"}
      }
    }}
  });
```

Instead of passing `initializer_list`, template values can also be stored / created dynamically:
```
  Template::List ls = {
    {}
  };
  ls.add({
    {"a", "5"}
  });

  Template::Data ks = {
    {"p", "1"},
    {"q", "2"},
    {"r", ls}
  };

  ks.set({ {"a", "k"}, {"a", "l"} });
  ks.set("er", "k");
  ks.set("erg1", {});  // empty list

  ks.add_list("erg2", { {"a", "9"} });  // add to list at key (created, when none yet)

  tmpl.render(ks);
```

* Template variables could be written as `$var` (alphanumeric) or `${var}`, the latter form also allows passing a modifer/"escaper" name: `${var:json}`.
  If no modifier is given, the modifier defaults to `""` (empty string).
* A literal `$` can be output with `$$`.
* Groups (`$[listvar ...inner template... $]`) are automatically repeated as often as necessary, joined by the first character after `listvar`.
  Alternatively, `$[listvar{joinstr}...$]` can be used (possibly escaped with `\`).
* Optionals (`$(...inner template...$)`) are completely skipped, when not all inner variables are present (and no warnings are printed).
* When a group or optional is not rendered – i.e. listvar is empty, resp. not all vars are present –,
  a directly preceding and directly following newline is collapsed into a single newline: `a\n$($missing$)\nb` becomes `a\nb`.
* Second-and-following lines of multi-line variable data are indented (w/ space, but tabs are copied) to match first line's indent/position.

TODO:
* Fix/better example.
* escaper function `(const std::string &string, const std::string &modifier) -> std::string` shall be passed to `Template::Engine` and applied to every variable output.
* ? remove trailing newline from variable output, when template also contains directly following newline?

Copyright (c) 2023 Tobias Hoffmann

License: https://opensource.org/licenses/MIT

