{
  lib,
  stdenv,
  cmake,
  pkg-config,
  capnproto,
  bitcoinIpcSdk ? null,
  bitcoinSourceDir ? null,
  bitcoinBuildDir ? null,
  bitcoinIncludeDirs ? [ ],
  bitcoinLibraryDirs ? [ ],
  bitcoinLinkLibs ? [ ],
}:

assert (bitcoinIpcSdk != null) || (bitcoinSourceDir != null && bitcoinBuildDir != null);

stdenv.mkDerivation rec {
  pname = "bitcoind-ipc-exporter";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  buildInputs = [
    capnproto
  ];

  cmakeFlags = [
    "-DBUILD_WITH_BITCOIN_CORE_TREE=${if bitcoinIpcSdk != null then "OFF" else "ON"}"
    "-DBITCOIN_SDK_DIR=${if bitcoinIpcSdk != null then toString bitcoinIpcSdk else ""}"
    "-DBITCOIN_SOURCE_DIR=${if bitcoinSourceDir != null then toString bitcoinSourceDir else ""}"
    "-DBITCOIN_BUILD_DIR=${if bitcoinBuildDir != null then toString bitcoinBuildDir else ""}"
    "-DBITCOIN_INCLUDE_DIRS=${lib.concatStringsSep ";" bitcoinIncludeDirs}"
    "-DBITCOIN_LIBRARY_DIRS=${lib.concatStringsSep ";" bitcoinLibraryDirs}"
    "-DBITCOIN_LINK_LIBS=${lib.concatStringsSep ";" bitcoinLinkLibs}"
  ];

  meta = {
    description = "Prometheus exporter for Bitcoin Core IPC metrics";
    platforms = lib.platforms.linux;
    license = lib.licenses.mit;
  };
}
