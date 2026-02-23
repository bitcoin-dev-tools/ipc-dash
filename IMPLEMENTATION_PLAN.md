# IPC Exporter Implementation Plan (Rust Schema-First)

## Goal
Build a Bitcoin Core IPC metrics exporter in Rust without linking Bitcoin Core C++ libraries.

## Chosen Architecture
- Language/runtime: Rust
- Transport/protocol: Cap'n Proto over Bitcoin Core IPC socket
- Source of interface contracts: vendored `.capnp` files from Bitcoin Core multiprocess branch
- Build strategy: Nix + Cargo + `build.rs` codegen

## Progress
## Phase 0: Foundation and Cleanup
Status: Completed

- removed old C++ exporter (`ipc_exporter/`)
- removed old SDK packaging/linking flow
- removed old config module import in `configuration.nix`
- added Rust skeleton project at `ipc_exporter_rust/`
- added schema vendoring from Bitcoin Core `src/`
- added `build.rs` code generation stage
- added Nix package build for the Rust crate
- added `just` targets for sync/build/run skeleton

## Phase 1: IPC Client Skeleton (Next)
Status: Pending

1. Build minimal IPC connection layer in Rust:
- connect to `unix`/explicit socket
- implement Init handshake primitives from schema

2. Add basic runtime logging and error handling:
- connect/retry/backoff
- clean shutdown

Deliverable:
- process connects and logs successful IPC handshake

## Phase 2: Query-Only Metrics Slice
Status: Pending

1. Implement periodic reads for direct state:
- blocks/headers/progress
- mempool size/usage/max
- net bytes and connection counts

2. Expose Prometheus `/metrics` endpoint

Deliverable:
- first working metric endpoint backed by IPC queries

## Phase 3: Callback/Event Metrics
Status: Pending

1. Register notifications:
- mempool add/remove
- block connected / tip updates

2. Add event counters with dedupe where required

Deliverable:
- callback-driven counter metrics available

## Phase 4: Derived + RPC-Fallback Metrics
Status: Pending

1. Derived metrics:
- peer snapshot deltas (if feasible with schema/client model)

2. Fallback values:
- IPC `executeRpc` path for difficulty/chain size/mempool bytes/minfee

Deliverable:
- near-parity with current dashboard-critical metrics

## Phase 5: NixOS Service Integration and Rollout
Status: Pending

1. Add standalone NixOS module for the Rust exporter
2. Run in parallel with existing USDT/RPC exporters
3. Validate parity, monotonicity, restart behavior

Deliverable:
- deployable service and cutover readiness report

## Notes / Constraints
- Current milestone intentionally stops at compilable skeleton to reduce risk early.
- No metric behavior should be assumed yet until Phase 2+ is completed.

