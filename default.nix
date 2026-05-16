{ lib, stdenv, pkg-config, bash }:
stdenv.mkDerivation {
  pname = "bashutils";
  version = "0.4.0-unstable-2026-05-15";
  __structuredAttrs = true;

  src               = lib.fileset.toSource {
    root    = ./.;
    fileset = let
      nixFiles = lib.fileset.unions [
        ./default.nix
        ./flake.nix
        ./flake.lock
      ];
    in lib.fileset.difference ./. nixFiles;
  };
  nativeBuildInputs = [ pkg-config ];
  buildInputs       = [ bash ];
  configurePhase    = ''
    runHook preConfigure
    makeFlagsArray+=("DESTDIR=''${out@Q}")
    runHook postConfigure
  '';
  separateDebugInfo = true;

  meta = {
    platforms = lib.platforms.linux;
    license = lib.licenses.howtonotwin;
  };
}
