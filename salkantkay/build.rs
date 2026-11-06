use std::env::{self};
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

    println!(
        "cargo:rustc-link-search=native={}",
        build_dir.to_str().unwrap()
    );
    println!("cargo:rustc-link-lib=dylib=nkrhuap");

    println!("cargo:rerun-if-changed=../src/");
    println!("cargo:rerun-if-changed=../CMakeLists.txt");
}
