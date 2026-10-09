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
    concatMapAttrsToList = f: as: lib.concatAttrValues (lib.mapAttrs f as);
  in {
    overlays.default = final: prev: {
      bashutils = final.callPackage ./. {
        stdenv = final.gcc16Stdenv;
      };
    };
    lib.writeShellApplication = {
      writeTextFile, runtimeShell, runtimeShellPackage,
      stdenv, shellcheck-minimal
    }: lib.extendMkDerivation {
      constructDrv = writeTextFile;
      inheritFunctionArgs = false;
      excludeDrvArgNames = [
        "version"
        "runtimeInputs" "shellVars" "saveEnvVars" "savedEnvName" "bashOptions"
        "doShellCheck" "shellCheckFlags"
      ];
      # partly copied from nixpkgs's pkgs.writeShellApplication
      extendDrvArgs = lowerAttrs: {
        name,
        version    ? null,
        executable ? true,
        text,

        runtimeInputs ? [ ],
        shellVars     ? { },
        saveEnvVars   ? [ ],
        savedEnvName  ? "savedEnv",
        bashOptions   ? [ ], # [ "errexit" "nounset" ] is prepended

        checkPhase      ? null,
        doShellCheck    ? shellcheck-minimal.compiler.bootstrapAvailable,
        shellCheckFlags ? [ ],

        meta           ? { },
        passthru       ? { },
        derivationArgs ? { },
        pos            ? builtins.unsafeGetAttrPos "name" upperAttrs
      }@upperAttrs: let
        upperAttrs = {
          inherit
            name version executable text
            runtimeInputs shellVars saveEnvVars savedEnvName bashOptions
            checkPhase doShellCheck shellCheckFlags
            meta passthru derivationArgs pos;
        };
      in let
        getLoadables = p: lib.optionals (p ? bashLoadables) (
          assert p.bashLoadables.target == runtimeShell;
          p.bashLoadables.propagated);
        loadables =
          lib.concatMap getLoadables runtimeInputs
          ++ [ "${runtimeShellPackage}/lib/bash" ];
        shellVars = {
          BASH_LOADABLES_PATH = lib.concatStringsSep ":" loadables;
          # not exported! we're in Nix-land!
          PATH                = lib.makeBinPath          runtimeInputs;
        } // upperAttrs.shellVars;
        bashOptions = [ "errexit" "nounset" ] ++ upperAttrs.bashOptions;
        derivationArgs = {
          ${if version != null then "pname"   else "name"} = name;
          ${if version != null then "version" else null}   = version;
        } // upperAttrs.derivationArgs;
      in {
        name = "${name}${lib.optionalString (version != null) "-${version}"}";
        destination = "/bin/${name}";
        inherit executable;
        allowSubstitutes = true;
        preferLocalBuild = false;

        text = let
          saveVar = v:
            assert lib.isValidPosixName v; let
            savedV = "${savedEnvName}[${lib.escapeShellArg v}]";
          in "[ -n \"\${${v}+:}\" ] && ${savedV}=\${${v}}";
          setOption = option: let
            polarity = if lib.hasPrefix "^" option then "+" else "-";
            name     = lib.removePrefix "^" option;
          in "set ${polarity}o ${name}";
          setVar = name: value: lib.optional (value != null)
            "unset ${name}; ${lib.toShellVar name value}";
        in lib.concatStringsSep "\n" (
          lib.singleton "#!${runtimeShell}"
          ++ lib.optional (saveEnvVars != [ ]) (
            assert lib.isValidPosixName savedEnvName;
            "unset ${savedEnvName}; declare -A ${savedEnvName}=()")
          ++ lib.map              saveVar   saveEnvVars
          ++ lib.map              setOption bashOptions
          ++ concatMapAttrsToList setVar    shellVars
          ++ [ "" text ]);
        checkPhase =
          if   checkPhase == null
          then lib.concatLines (
            lib.singleton "runHook preCheck"
            ++ lib.singleton "${stdenv.shellDryRun} \"$target\""
            ++ lib.optional doShellCheck (let
                shellcheck = lib.escapeShellArg (lib.getExe shellcheck-minimal);
                flags      = lib.escapeShellArgs shellCheckFlags;
              in "${shellcheck} ${flags} \"$target\"")
            ++ lib.singleton "runHook postCheck")
          else checkPhase;

        meta = { mainProgram = name; } // meta;
        inherit passthru derivationArgs pos;
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
