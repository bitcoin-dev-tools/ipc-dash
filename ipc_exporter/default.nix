{
  lib,
  stdenv,
  cmake,
  pkg-config,
  bitcoinSourceDir ? null,
  bitcoinBuildDir ? null,
  bitcoinIncludeDirs ? [ ],
  bitcoinLibraryDirs ? [ ],
  bitcoinLinkLibs ? [ ],
}:

assert bitcoinSourceDir != null;
assert bitcoinBuildDir != null;

stdenv.mkDerivation rec {
  pname = "bitcoind-ipc-exporter";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  cmakeFlags = [
    "-DBUILD_WITH_BITCOIN_CORE_TREE=ON"
    "-DBITCOIN_SOURCE_DIR=${toString bitcoinSourceDir}"
    "-DBITCOIN_BUILD_DIR=${toString bitcoinBuildDir}"
    "-DBITCOIN_INCLUDE_DIRS=${lib.concatStringsSep \";\" bitcoinIncludeDirs}"
    "-DBITCOIN_LIBRARY_DIRS=${lib.concatStringsSep \";\" bitcoinLibraryDirs}"
    "-DBITCOIN_LINK_LIBS=${lib.concatStringsSep \";\" bitcoinLinkLibs}"
  ];

  meta = {
    description = "Prometheus exporter for Bitcoin Core IPC metrics";
    platforms = lib.platforms.linux;
    license = lib.licenses.mit;
  };
}
