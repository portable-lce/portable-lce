{
  description = "portablelce nix-package and dev-shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    shiggy = {
      url = "github:portable-lce/shiggy/main";
      flake = false;
    };

    miniaudio = {
      url = "https://github.com/mackron/miniaudio/archive/refs/tags/0.11.22.tar.gz";
      flake = false;
    };

    # patches only get applied if they follow <subproject_to_patch>-patch naming
    miniaudio-patch = {
      url = "https://wrapdb.mesonbuild.com/v2/miniaudio_0.11.22-2/get_patch";
      flake = false;
    };

    stb = {
      url = "github:nothings/stb/master";
      flake = false;
    };

    simdutf = {
      url = "github:simdutf/simdutf";
      flake = false;
    };
  };

  outputs =
    { self, nixpkgs, flake-utils, ... }@inputs:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        lib = pkgs.lib;

        subprojectNames = [
          "shiggy"
          "stb"
          "simdutf"
          "miniaudio"
        ];

        # helper: copy all subproject sources
        copySubprojects = ''
          mkdir -p $sourceRoot/subprojects
          ${lib.concatMapStringsSep "\n" (name: "cp -r ${inputs.${name}} $sourceRoot/subprojects/${name}") subprojectNames}
          chmod -R u+w $sourceRoot/subprojects
        '';

        # helper: copy packagefiles
        copyPackagefiles = ''
          for proj in ${builtins.toString subprojectNames}; do
            if [ -d "subprojects/packagefiles/$proj" ]; then
              cp -r subprojects/packagefiles/$proj/* subprojects/$proj/
            fi
          done
        '';

        # helper: apply patches from '-patch' inputs
        applyPatches = lib.concatMapStringsSep "\n" (name: ''
          patch_input="${inputs.${name + "-patch"} or ""}"
          if [ -n "$patch_input" ]; then
            unzip "$patch_input" -d ${name}-patch-tmp
            if [ $(ls -1 ${name}-patch-tmp | wc -l) -eq 1 ] && [ -d ${name}-patch-tmp/* ]; then
              cp -r ${name}-patch-tmp/*/* subprojects/${name}/
            else
              cp -r ${name}-patch-tmp/* subprojects/${name}/
            fi
            rm -rf ${name}-patch-tmp
          fi
        '') subprojectNames;

      in
      {
        packages.default = pkgs.clangStdenv.mkDerivation {
          pname = "portablelce";
          version = "0.1.0";
          src = ./.;

          dontFixup = true;
          dontUseCmakeConfigure = true;

          postUnpack = ''
            ${copySubprojects}
          '';

          postPatch = ''
            # Remove wrap files so Meson doesn't try to download them
            for proj in ${builtins.toString subprojectNames}; do
              rm -f subprojects/$proj.wrap
            done

            ${copyPackagefiles}
            ${applyPatches}
          '';

          nativeBuildInputs = with pkgs; [
            lld
            makeWrapper
            meson
            ninja
            pkg-config
            python3
            unzip
          ];

          buildInputs = with pkgs; [
            openssl.dev
            libGL
            libGLU
            glm
            SDL2
            zlib
          ];

          installPhase = ''
            mkdir -p $out/share/portablelce
            cp -r targets/app/. $out/share/portablelce/

            mkdir -p $out/bin
            makeWrapper $out/share/portablelce/Minecraft.Client $out/bin/portablelce \
              --run "cd $out/share/portablelce"
          '';

          meta = {
            description = "Portable-LCE";
            platforms = lib.platforms.unix;
          };
        };

        devShells.default =
          pkgs.mkShell.override
            {
              stdenv = pkgs.clangStdenv;
            }
            {
              inputsFrom = [ self.packages.${system}.default ];

              packages = with pkgs; [
                clang-tools
                lldb
                valgrind
                include-what-you-use
                ccache
              ];

              shellHook = ''
                export CC="ccache clang"
                export CXX="ccache clang++"
              '';
            };
      }
    );
}
