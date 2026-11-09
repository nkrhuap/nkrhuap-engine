#pragma once

#ifdef _WIN32
#define NK_EXPORT __declspec(dllexport)
#else
#define NK_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
struct Application;

NK_EXPORT Application *nkrhuap_create_app();
NK_EXPORT bool nkrhuap_poll_events(Application *app);
NK_EXPORT void nkrhuap_render_frame(Application *app);
NK_EXPORT void nkrhuap_destroy_app(Application *app);
}
