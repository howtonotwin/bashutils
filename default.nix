{ lib, stdenv, pkg-config, bash }:
stdenv.mkDerivation {
  pname = "bashutils";
  version = "0.3.1-unstable-2026-05-08";
  __structuredAttrs = true;

  src               = ./.;
  nativeBuildInputs = [ pkg-config ];
  buildInputs       = [ bash ];
  preUnpack = ''
    installFlagsArray+=("DESTDIR=''${out@Q}")
  '';
  separateDebugInfo = true;

  meta = {
    platforms = lib.platforms.linux;
    license = lib.licenses.howtonotwin;
  };
}
