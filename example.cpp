#include "template_engine.h"

int main()
{
  try {
    auto tmpl = Template::Engine::fromString(R"(
    )");

    tmpl.render({
//      {},
    });
    printf("\n");
  } catch (std::exception &e) {
    printf("Exception: %s\n",e.what());
    return 1;
  }

  return 0;
}
