# USDT to IPC Metrics Collection Overview

## Objective
Migrate telemetry collection from USDT+BCC and RPC polling to Bitcoin Core IPC.

Current Bitcoin Core IPC worktree used for schema source:
- `/home/will/src/core/worktrees/pr-10102`

## Current State (February 23, 2026)
The previous C++ exporter and SDK-linking approach was removed.

Repository now contains a Rust IPC client project:
- `ipc_exporter_rust/` with flat vendored capnp schemas
- Cap'n Proto codegen via `build.rs` producing flat Rust modules
- Runtime dependencies for IPC: `capnp-rpc`, `tokio`, `tokio-util`
- `nix develop` devShell with Rust toolchain and capnproto

## Why We Switched
The C++/SDK approach introduced heavy static-link complexity and unnecessary coupling to wallet/internal libraries.

The new approach mirrors the proven model used in `core_bdk_wallet`:
- copy `.capnp` files from Bitcoin Core
- generate client code directly in Rust
- communicate over IPC socket protocol without linking `libbitcoin_*`

## What Is Implemented Now
1. Rust crate with IPC runtime dependencies:
- `ipc_exporter_rust/Cargo.toml` (capnp, capnp-rpc, tokio, tokio-util)
- `ipc_exporter_rust/build.rs` (flat schema codegen)
- `ipc_exporter_rust/src/main.rs` (stub -- IPC client next)

2. Flat vendored schema set (only files needed for IPC client):
- `ipc_exporter_rust/schema/*.capnp` (chain, common, echo, handler, init, mining, node, wallet)
- `ipc_exporter_rust/schema/mp/proxy.capnp`

3. Nix integration:
- `ipc_exporter_rust/default.nix` (package build)
- `flake.nix` devShell with cargo, rustc, clippy, rustfmt, capnproto

4. Project wiring:
- `justfile` targets: `sync-ipc-capnp`, `build-ipc-rust`, `run-ipc-rust`
- sync target needs updating for flat layout (next step)

## Verified
- `nix develop` provides working Rust toolchain
- `cargo build` succeeds with flat schema codegen
- Generated modules use simple paths: `chain_capnp`, `init_capnp`, `proxy_capnp`
- Cross-references confirmed: generated code uses `crate::proxy_capnp` (not `crate::mp::proxy_capnp`), so proxy include goes at crate root despite file living at `$OUT_DIR/mp/proxy_capnp.rs`

## Not Implemented Yet
- IPC client: Unix socket connect, Init handshake, Chain/Node queries
- ChainNotifications subscription (block tip, mempool events)
- Prometheus exporter endpoint and metrics
- NixOS service/module integration
- justfile sync-ipc-capnp and run-ipc-rust target updates

