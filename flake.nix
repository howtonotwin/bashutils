{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
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
  });
}
