{ lib, stdenv, pkg-config, bash }:
stdenv.mkDerivation {
  pname = "bashutils";
  version = "0.1.0-unstable-2025-12-06";

  src = ./.;
  nativeBuildInputs = [ pkg-config ];
  buildInputs = [ bash ];
  preUnpack = ''
    installFlagsArray+=("DESTDIR=''${out@Q}")
  '';
  postUnpack = ''
    export CFLAGS="-fdebug-prefix-map=$(realpath "$sourceRoot")=$src''${CFLAGS+ }$CFLAGS"
  '';
  separateDebugInfo = true;

  meta = {
    platforms = lib.platforms.linux;
    license = lib.licenses.howtonotwin;
  };
}
