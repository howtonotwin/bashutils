{ lib, stdenv, pkg-config, bash, libcap }:
stdenv.mkDerivation {
  pname = "bashutils";
  version = "0.7.0";
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
  nativeBuildInputs      = [ pkg-config ];
  buildInputs            = [ bash libcap ];
  enableParallelBuilding = true;
  separateDebugInfo      = true;
  configurePhase         = ''
    runHook preConfigure
    makeFlagsArray+=("DESTDIR=''${out@Q}")
    runHook postConfigure
  '';

  meta = {
    platforms = lib.platforms.linux;
    license = lib.licenses.howtonotwin;
  };
}
