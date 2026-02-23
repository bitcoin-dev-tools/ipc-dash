{
  lib,
  stdenv,
  bitcoinSourceDir,
  bitcoinBuildDir,
}:

stdenv.mkDerivation rec {
  pname = "bitcoin-ipc-sdk";
  version = "0.1.0";

  dontUnpack = true;

  installPhase = ''
    runHook preInstall

    mkdir -p "$out/include/src"
    mkdir -p "$out/include/build"
    mkdir -p "$out/lib"
    mkdir -p "$out/nix-support"

    # Source headers used by interfaces + their transitive includes.
    cp -R "${bitcoinSourceDir}/src/." "$out/include/src/"

    # Generated build headers (e.g. bitcoin-build-config.h and generated proxies).
    if [ -d "${bitcoinBuildDir}/src" ]; then
      cp -R "${bitcoinBuildDir}/src/." "$out/include/build/"
    fi
    if [ -f "${bitcoinBuildDir}/bitcoin-build-config.h" ]; then
      cp "${bitcoinBuildDir}/bitcoin-build-config.h" "$out/include/build/"
    fi

    # Bundle static libraries needed by IPC consumers.
    for pattern in \
      "${bitcoinBuildDir}/lib/"*.a \
      "${bitcoinBuildDir}/src/"*.a \
      "${bitcoinBuildDir}/src/univalue/"*.a \
      "${bitcoinBuildDir}/src/secp256k1/lib/"*.a \
      "${bitcoinBuildDir}/src/ipc/libmultiprocess/"*.a
    do
      if [ -f "$pattern" ]; then
        cp "$pattern" "$out/lib/"
      fi
    done

    cat > "$out/nix-support/cmake-flags" <<EOF_INNER
-DBITCOIN_SDK_DIR=$out
EOF_INNER

    runHook postInstall
  '';

  meta = {
    description = "Packaged Bitcoin Core IPC SDK (headers + static libraries)";
    platforms = lib.platforms.linux;
    license = lib.licenses.mit;
  };
}
