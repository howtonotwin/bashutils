{ bash, buildEnv, loadables, makeBinaryWrapper }:
buildEnv rec {
  name = "${pname}-${version}";
  pname = "${bash.pname}WithLoadables";
  inherit (bash) version;

  paths = [ bash ] ++ loadables;
  pathsToLink = [
    "/bin"
    "/lib/bash"
    "/share"
  ];

  nativeBuildInputs = [ makeBinaryWrapper ];
  # MUST use makeBinaryWrapper, as otherwise the wrapper script is also bash!
  postBuild = ''
    wrapProgram "$out/bin/bash" \
      --set-default BASH_LOADABLES_PATH "$out/lib/bash:."
  '';
  meta.mainProgram = bash.meta.mainProgram;
}
