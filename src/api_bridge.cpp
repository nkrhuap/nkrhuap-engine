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

NK_EXPORT bool nkrhuap_poll_events(Application *app) {
  if (!app)
    return false;

  SDL_Event event{0};
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      return false;
    }
  }
  return true;
}

NK_EXPORT void nkrhuap_render_frame(Application *app) {
  if (app) {
  }
}

NK_EXPORT void nkrhuap_destroy_app(Application *app) {
  if (app) {
    app->shutdown();
    delete app;
  }
}
}
