# bitcoind IPC exporter (C++)

Initial C++ implementation of a Prometheus exporter that collects metrics from Bitcoin Core IPC (`Node` + `Chain`) and exposes `/metrics`.

## Build (CMake)

This project expects an IPC-enabled Bitcoin Core build tree because it depends on generated Cap'n Proto proxy headers and Bitcoin Core IPC libraries.

```sh
cmake -S ipc_exporter -B build/ipc_exporter \
  -DBITCOIN_SOURCE_DIR=/path/to/bitcoin \
  -DBITCOIN_BUILD_DIR=/path/to/bitcoin/build
cmake --build build/ipc_exporter -j
```

If your link/include layout differs, pass:

- `-DBITCOIN_INCLUDE_DIRS='dir1;dir2'`
- `-DBITCOIN_LIBRARY_DIRS='dir1;dir2'`
- `-DBITCOIN_LINK_LIBS='lib1;lib2;lib3'`

## Run

```sh
./build/ipc_exporter/bitcoind-ipc-exporter \
  --ipc-connect unix \
  --metrics-port 9437 \
  --poll-interval 5 \
  --rpc-interval 15
```

Key arguments:

- `--ipc-connect`: IPC address (`unix`, `auto`, or explicit address)
- `--metrics-port`: HTTP port for `/metrics`
- `--poll-interval`: direct/snapshot polling interval seconds
- `--rpc-interval`: `executeRpc` fallback polling interval seconds

## Current scope

Implemented in this interfaces-only first iteration:

- direct IPC gauges (`getNumBlocks`, `getHeaderTip`, `getVerificationProgress`, mempool/net totals)
- `Chain` callbacks for mempool add/remove and block connected
- IPC `executeRpc` fallback for:
  - `bitcoind_blockchain_difficulty`
  - `bitcoind_blockchain_size_bytes`
  - `bitcoind_mempool_bytes`
  - `bitcoind_mempool_minfee_per_kb`

Not included yet:

- peer-delta derived metrics (`bitcoind_p2p_bytes_total`, connection open/close counters)
- mempool added vbytes total
- USDT-only metrics
