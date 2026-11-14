use std::env::{self};
use std::path::Path;
use std::process::Command;

fn main() {
    let _out_dir = env::var("OUT_DIR").unwrap();
    let current_dir = std::env::current_dir().unwrap();
    let root_dir = current_dir.parent().unwrap();
    let build_dir = root_dir.join("build");

    let vcpkg_root = env::var("VCPKG_ROOT").expect("A variável de ambiente VCPKG_ROOT não foi definida. Não somos adivinhadores, defina-a antes de compilar.");
    let vcpkg_toolchain = format!("{}/scripts/buildsystems/vcpkg.cmake", vcpkg_root);

    let configure_status = Command::new("cmake")
        .args([
            "-S",
            root_dir.to_str().unwrap(),
            "-B",
            build_dir.to_str().unwrap(),
            "-G",
            "Ninja",
            "-DCMAKE_C_COMPILER=clang-cl",
            "-DCMAKE_CXX_COMPILER=clang-cl",
            &format!("-DCMAKE_TOOLCHAIN_FILE={}", vcpkg_toolchain),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ])
        .status()
        .expect("Falha ao rodar configuração do CMake");
    assert!(configure_status.success());

    let build_status = Command::new("cmake")
        .args([
            "--build",
            build_dir.to_str().unwrap(),
            "--config",
            "Release",
        ])
        .status()
        .expect("Falha ao compilar a engine");
    assert!(build_status.success());

    let profile = env::var("PROFILE").unwrap();
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let target_dir = Path::new(&manifest_dir).join("target").join(profile);

    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    let (prefix, extension) = if target_os == "windows" {
        ("", "dll")
    } else {
        ("lib", "so")
    };

    let nkrhuap_lib = format!("{}nkrhuap.{}", prefix, extension);
    let sdl_lib = format!("{}SDL3.{}", prefix, extension);

    let nkrhuap_source = build_dir.join(&nkrhuap_lib);
    if nkrhuap_source.exists() {
        std::fs::copy(&nkrhuap_source, target_dir.join(&nkrhuap_lib)).unwrap();
    }

    let sdl_source = build_dir.join(&sdl_lib);
    if sdl_source.exists() {
        std::fs::copy(&sdl_source, target_dir.join(&sdl_lib)).unwrap();
    }

    println!(
        "cargo:rustc-link-search=native={}",
        build_dir.to_str().unwrap()
    );
    println!("cargo:rustc-link-lib=dylib=nkrhuap");
    println!("cargo:rustc-link-lib=dylib=SDL3");

    //  No Linux, diz ao linker para embutir o diretório do binário como caminho de busca.
    if target_os == "linux" {
        print!("cargo:rustc-link-arg=Wl,-rpath,$ORIGIN");
    }

    println!("cargo:rerun-if-changed=../src/");
    println!("cargo:rerun-if-changed=../CMakeLists.txt");
}
