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
      bashutils = final.callPackage ./. {
        stdenv = final.gcc16Stdenv;
      };
    };
  } // forSystems lib.platforms.linux (system: self: let
    pkgs = import nixpkgs {
      inherit system;
      overlays = [ self.overlays.default ];
    };
  in {
    packages = {
      inherit (pkgs) bashutils;
      default = self.packages.bashutils;
    };
  });
}
