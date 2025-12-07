{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  outputs = { self, nixpkgs }: let
    inherit (nixpkgs) lib;
    forSystems = systems: outputs: lib.zipAttrsWith
      (type: lib.mergeAttrsList)
      (lib.map
        (system: lib.mapAttrs
          (type: value: { ${system} = value; })
          (lib.fix (systemOutputs:
            outputs system (self // systemOutputs))))
        systems);
  in {
    overlays.default = final: prev: {
      lib = prev.lib.extend (final: prev: {
        licenses = prev.licenses // {
          howtonotwin = {
            shortName = "howtonotwin";
            fullName = "(c) 2025 Rasheeq Azad. All Rights Reserved";
            free = false;
            deprecated = false;
            redistributable = false;
          };
        };
      });
      bashutils = final.callPackage ./. { stdenv = final.gcc15Stdenv; };
    };
  } // forSystems lib.systems.flakeExposed (system: self: let
    pkgs = import nixpkgs {
      inherit system;
      overlays = [ self.overlays.default ];
      config.allowlistedLicenses = [ pkgs.lib.licenses.howtonotwin ];
    };
  in {
    packages = {
      inherit (pkgs) bashutils;
      default = self.packages.bashutils;
    };
    devShells = {
      bashutils = let
        bash' = pkgs.bash.overrideAttrs (final: prev: {
          src = pkgs.runCommandLocal "bash-src" {
            inherit (prev) src;
          } ''
            unpackPhase
            mv -v "$sourceRoot" "$out"
          '';
          postUnpack = prev.postUnpack or "" + ''
            export CFLAGS+=" -Og -g -fdebug-prefix-map=$(realpath "$sourceRoot")=$src"
          '';
        });
        pkg = self.packages.bashutils.override { bash = bash'; };
      in (pkgs.mkShell.override {
        inherit (pkg) stdenv;
      } {
        name = "bashutils-dev";
        inputsFrom = [ pkg ];
      }).overrideAttrs (final: prev: {
        NIX_DEBUG_INFO_DIRS = lib.makeSearchPathOutput
          "debug"
          "lib/debug"
          [ bash' ];
      });
      default = self.devShells.bashutils;
    };
  });
}
