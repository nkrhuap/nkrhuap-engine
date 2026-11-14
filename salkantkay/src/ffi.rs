#[repr(C)]
pub struct Application {
    _private: [u8; 0],
}

#[repr(C)]
pub struct EngineInitDesc {
    pub window_handle: *mut std::ffi::c_void,
    pub width: u32,
    pub height: u32,
}

#[link(name = "nkrhuap", kind = "dylib")]
unsafe extern "C" {
    pub fn nkrhuap_create_app(desc: *const EngineInitDesc) -> *mut Application;
    pub fn nkrhuap_render_frame(app: *mut Application);
    pub fn nkrhuap_destroy_app(app: *mut Application);
}
