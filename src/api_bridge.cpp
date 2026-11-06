#include "application.hpp"
#include "nkrhuap_api.hpp"

extern "C" {
NK_EXPORT Application *nkrhuap_create_app() {
  Application *app = new Application();
  if (!app->initialize()) {
    app->shutdown();
    delete app;
    return nullptr;
  }
  return app;
}
}
