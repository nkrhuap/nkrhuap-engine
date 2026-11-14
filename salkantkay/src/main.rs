mod ffi;
mod window;

fn main() {
    unsafe {
        let app = nkrhuap_create_app();
        if app.is_null() {
            eprintln!("Erro ao inicializar engine.");
            return;
        }

        loop {
            let running = nkrhuap_poll_events(app);
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
