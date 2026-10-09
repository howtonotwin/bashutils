{ lib, stdenv, pkg-config, bash, libcap }:
stdenv.mkDerivation {
  pname = "bashutils";
  version = "0.7.3-unstable-2026-09-28";
  __structuredAttrs = true;

  src = lib.fileset.toSource {
    root    = ./.;
    fileset = lib.fileset.difference
      (lib.fileset.gitTracked ./.)
      (lib.fileset.unions [
        ./.gitignore ./README.md
        ./default.nix ./flake.nix ./flake.lock
      ]);
  };
  nativeBuildInputs      = [ pkg-config ];
  buildInputs            = [ bash libcap ];
  enableParallelBuilding = true;
  separateDebugInfo      = true;
  configurePhase         = ''
    runHook preConfigure
    makeFlagsArray+=("PREFIX = ''${out@Q}")
    runHook postConfigure
  '';

  meta = {
    platforms = lib.platforms.linux;
    license   = lib.licensesSpdx."GPL-3.0-or-later";
  };
}
