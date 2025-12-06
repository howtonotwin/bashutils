{ lib, stdenv, pkg-config, bash }:
stdenv.mkDerivation {
  pname = "bashutils";
  version = "0-unstable-2025-12-06";

  src = ./.;
  nativeBuildInputs = [ pkg-config ];
  buildInputs = [ bash ];
  preUnpack = ''
    installFlagsArray+=("DESTDIR=''${out@Q}")
  '';

  meta = {
    platforms = lib.platforms.linux;
    license = lib.licenses.howtonotwin;
  };
}
