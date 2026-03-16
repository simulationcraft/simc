{
  description = "simulationcraft";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    git-hooks = {
      url = "github:cachix/git-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      flake-utils,
      treefmt-nix,
      nixpkgs,
      git-hooks,
      self,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        treefmt = treefmt-nix.lib.evalModule pkgs (_: {
          projectRootFile = "flake.nix";
          programs = {
            actionlint.enable = true;
            dockerfmt.enable = true;
            nixfmt.enable = true;
          };
        });

        githooksEval = git-hooks.lib.${system}.run {
          src = self;
          hooks.treefmt = {
            enable = true;
            package = treefmt.config.build.wrapper;
          };
        };

        llvm = pkgs.llvmPackages_latest;
        packages = with pkgs; [
          clang-tools
          llvm.clang
          llvm.lldb
          qt6.qtbase
          qt6.qtwebengine
        ];

        nbi = with pkgs; [
          pkg-config
          curlFull
          cmake
          qt6.wrapQtAppsHook
        ];

        qtCmakePath = pkgs.symlinkJoin {
          name = "qt6-cmake";
          paths = with pkgs.qt6; [
            qtbase
            qtwebengine
          ];
        };

        rev = self.rev or "dirty";
      in
      {
        formatter = treefmt.config.build.wrapper;
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = nbi;
          packages = packages;
          shellHook = ''
            ${githooksEval.shellHook}
          '';
        };

        packages.simc = llvm.stdenv.mkDerivation rec {
          pname = "simc";
          version = "1201.01.${rev}";
          src = self;
          nativeBuildInputs = nbi;
          buildInputs = packages;
          sconsFlags = "";
          enableParallelBuilding = true;
          cmakeFlags = [ "-DCMAKE_PREFIX_PATH=${qtCmakePath}/lib/cmake" ];
        };
        packages.default = self.packages.${system}.simc;
      }
    );
}
