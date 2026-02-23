# USDT to IPC Metrics Collection Overview

## Objective
Migrate telemetry collection from USDT+BCC and RPC polling to Bitcoin Core IPC.

Current Bitcoin Core IPC worktree used for schema source:
- `/home/will/src/core/worktrees/pr-10102`

## Current State (February 23, 2026)
The previous C++ exporter and SDK-linking approach was removed.

Repository now contains a new Rust schema-first skeleton:
- `ipc_exporter_rust/`
- no Bitcoin Core C++ linking
- no metrics collection logic yet
- only Cap'n Proto schema vendoring + Rust codegen in `build.rs`

## Why We Switched
The C++/SDK approach introduced heavy static-link complexity and unnecessary coupling to wallet/internal libraries.

The new approach mirrors the proven model used in `core_bdk_wallet`:
- copy `.capnp` files from Bitcoin Core
- generate client code directly in Rust
- communicate over IPC socket protocol without linking `libbitcoin_*`

## What Is Implemented Now
1. New Rust crate:
- `ipc_exporter_rust/Cargo.toml`
- `ipc_exporter_rust/build.rs`
- `ipc_exporter_rust/src/main.rs`

2. Vendored schema set copied from Bitcoin Core `src/` tree:
- `ipc_exporter_rust/schema/ipc/capnp/*.capnp`
- `ipc_exporter_rust/schema/ipc/libmultiprocess/include/mp/proxy.capnp`
- plus other `.capnp` files under `src/ipc/...` copied for reference completeness

3. Nix build for the skeleton:
- `ipc_exporter_rust/default.nix`

4. Project wiring updates:
- removed `ipc_exporter/` C++ implementation
- removed `configuration.nix` import of old IPC module
- updated `justfile` targets:
  - `sync-ipc-capnp`
  - `build-ipc-rust`
  - `run-ipc-rust`

## Verified
- `nix build` for `ipc_exporter_rust` succeeds
- skeleton binary runs and prints startup message

## Not Implemented Yet
- IPC runtime client handshake/connection
- Node/Chain IPC requests and callbacks
- Prometheus exporter endpoint and metrics
- NixOS service/module integration for runtime deployment

