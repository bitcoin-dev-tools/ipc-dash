{
  lib,
  rustPlatform,
  capnproto,
}:

rustPlatform.buildRustPackage {
  pname = "ipc-exporter-rust";
  version = "0.1.0";

  src = ./.;
  cargoLock.lockFile = ./Cargo.lock;

  nativeBuildInputs = [
    capnproto
  ];

  meta = {
    description = "Rust IPC exporter skeleton with Bitcoin Core capnp schemas";
    license = lib.licenses.mit;
    platforms = lib.platforms.linux;
  };
}
