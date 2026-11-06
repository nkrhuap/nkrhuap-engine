#[repr(C)]
pub struct Application {
    _private: [u8; 0],
}

#[link(name = "nkrhuap_engine", kind = "dylib")]
extern "C" {
    fn nkrhuap_create_app() -> *mut Application;
    fn nkrhuap_poll_events(app: *mut Application) -> bool;
    fn nkrhuap_render_frame(app: *mut Application);
    fn nkrhuap_destroy_app(app: *mut Application);
}

fn main() {
    unsafe {
        let app = nkrhuap_create_app();
        if app.is_null() {
            eprintln!("Erro ao inicializar engine.");
            return;
        }

        loop {
            let running = nkrhua_poll_events(app);
            if !running {
                break;
            }

            //  Logica de jogo e talz, vou add conforme for indo.

            nkrhuap_render_frame(app);
        }

        nkrhuap_destroy_app(app);
    }

    println!("Obrigado!");
}
