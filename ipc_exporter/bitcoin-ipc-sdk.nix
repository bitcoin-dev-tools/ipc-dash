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

    # Build a client-only IPC archive to avoid pulling wallet IPC server code.
    ipc_objdir="${bitcoinBuildDir}/src/ipc/CMakeFiles/bitcoin_ipc.dir"
    if [ -d "$ipc_objdir" ]; then
      cp \
        "$ipc_objdir/interfaces.cpp.o" \
        "$ipc_objdir/process.cpp.o" \
        "$ipc_objdir/capnp/common.cpp.o" \
        "$ipc_objdir/capnp/protocol.cpp.o" \
        "$ipc_objdir/capnp/node.cpp.o" \
        "$ipc_objdir/capnp/chain.cpp.o" \
        "$ipc_objdir/capnp/common.capnp.c++.o" \
        "$ipc_objdir/capnp/common.capnp.proxy-client.c++.o" \
        "$ipc_objdir/capnp/common.capnp.proxy-server.c++.o" \
        "$ipc_objdir/capnp/common.capnp.proxy-types.c++.o" \
        "$ipc_objdir/capnp/handler.capnp.c++.o" \
        "$ipc_objdir/capnp/handler.capnp.proxy-client.c++.o" \
        "$ipc_objdir/capnp/handler.capnp.proxy-server.c++.o" \
        "$ipc_objdir/capnp/handler.capnp.proxy-types.c++.o" \
        "$ipc_objdir/capnp/init.capnp.c++.o" \
        "$ipc_objdir/capnp/init.capnp.proxy-client.c++.o" \
        "$ipc_objdir/capnp/init.capnp.proxy-server.c++.o" \
        "$ipc_objdir/capnp/init.capnp.proxy-types.c++.o" \
        "$ipc_objdir/capnp/node.capnp.c++.o" \
        "$ipc_objdir/capnp/node.capnp.proxy-client.c++.o" \
        "$ipc_objdir/capnp/node.capnp.proxy-server.c++.o" \
        "$ipc_objdir/capnp/node.capnp.proxy-types.c++.o" \
        "$ipc_objdir/capnp/chain.capnp.c++.o" \
        "$ipc_objdir/capnp/chain.capnp.proxy-client.c++.o" \
        "$ipc_objdir/capnp/chain.capnp.proxy-server.c++.o" \
        "$ipc_objdir/capnp/chain.capnp.proxy-types.c++.o" \
        "$ipc_objdir/capnp/echo.capnp.c++.o" \
        "$ipc_objdir/capnp/mining.capnp.c++.o" \
        "$ipc_objdir/capnp/wallet.capnp.c++.o" \
        "$TMPDIR/"
      ar rcs "$out/lib/libbitcoin_ipc_client.a" "$TMPDIR/"*.o
    fi

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
