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
            shortName       = "howtonotwin";
            fullName        = "(c) Rasheeq Azad. All Rights Reserved";
            free            = false;
            deprecated      = false;
            redistributable = false;
          };
        };
      });
      bashutils = final.callPackage ./. {
        stdenv = final.gcc16Stdenv;
      };
    };
    overrides.bash.debuggable = pkgs: final: prev: {
      postUnpack = lib.concatStringsSep "\n"
        (  lib.catAttrs "postUnpack" [ prev ]
        ++ [ ''export CFLAGS="''${CFLAGS:+$CFLAGS }-Og -g"'' ]);
    };
    overrides.bash.fullPluginInterface = pkgs: final: prev: {
      postUnpack = lib.concatStringsSep "\n"
        (  lib.catAttrs "postUnpack" [ prev ]
        ++ [ ''export CFLAGS="''${CFLAGS:+$CFLAGS }-DINCLUDE_UNUSED"'' ]);
    };
  } // forSystems lib.platforms.linux (system: self: let
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
      default = self.devShellGenerators.bashutilsWithBashOverrides [
        self.overrides.bash.debuggable
      ];
      withFullBash = self.devShellGenerators.bashutilsWithBashOverrides [
        self.overrides.bash.debuggable
        self.overrides.bash.fullPluginInterface
      ];
    };
    devShellGenerators.bashutilsWithBashOverrides = overrides: let
      bash' = lib.foldl
        (pkg: override: pkg.overrideAttrs (override pkgs))
        pkgs.bash
        overrides;
      pkg = self.packages.bashutils.override { bash = bash'; };
    in pkg.overrideAttrs (final: prev: {
      name = "${prev.pname}-dev";
      NIX_DEBUG_INFO_DIRS = lib.makeSearchPathOutput
        "debug"
        "lib/debug"
        [ bash' ];
    });
  });
}
