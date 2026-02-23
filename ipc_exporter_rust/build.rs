use std::path::Path;

fn main() {
    let files = [
        "schema/ipc/capnp/common.capnp",
        "schema/ipc/capnp/handler.capnp",
        "schema/ipc/capnp/echo.capnp",
        "schema/ipc/capnp/mining.capnp",
        "schema/ipc/capnp/wallet.capnp",
        "schema/ipc/capnp/node.capnp",
        "schema/ipc/capnp/chain.capnp",
        "schema/ipc/capnp/init.capnp",
        "schema/ipc/libmultiprocess/include/mp/proxy.capnp",
    ];

    for file in files {
        println!("cargo:rerun-if-changed={file}");
    }

    let mut cmd = capnpc::CompilerCommand::new();
    cmd.src_prefix("schema")
        .import_path("schema")
        .import_path("schema/ipc/libmultiprocess/include")
        .output_path(Path::new(&std::env::var("OUT_DIR").expect("OUT_DIR should be set")));

    for file in files {
        cmd.file(file);
    }

    cmd.run().expect("capnp codegen failed");
}
