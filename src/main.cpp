#include "application.hpp"
#include <SDL3/SDL_main.h>

int main(int argc, char *argv[]) {
  Application app;
  if (app.initialize()) {
    app.run();
  }
  app.shutdown();

  return 0;
}
