{
  description = "simulationcraft";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    {
      flake-utils,
      nixpkgs,
      self,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        rev = self.rev or "dirty";
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_latest;

        packages = with pkgs; [
          clang-tools
          llvm.clang
          llvm.lldb
        ];

        qtPackages =
          with pkgs;
          [
            qt6.qtbase
            qt6.qtwebengine
          ]
          ++ packages;

        nbi = with pkgs; [
          curlFull
          cmake
          pkg-config
        ];

        qtNbi = with pkgs; [ qt6.wrapQtAppsHook ] ++ nbi;
      in
      {
        devShells = {
          default = self.devShells.${system}.simc;
          simc = pkgs.mkShell {
            nativeBuildInputs = nbi;
            packages = packages;
          };

          simcqt = pkgs.mkShell {
            nativeBuildInputs = qtNbi;
            packages = qtPackages;
          };
        };

        packages = {
          default = self.packages.${system}.simc;
          simc = llvm.stdenv.mkDerivation {
            pname = "simc";
            version = "1201.01.${rev}";
            src = self;
            nativeBuildInputs = nbi;
            buildInputs = packages;
            sconsFlags = "";
            enableParallelBuilding = true;
            cmakeFlags = [ "-DBUILD_GUI=OFF" ];
          };

          simcqt = llvm.stdenv.mkDerivation {
            pname = "simcqt";
            version = "1201.01.${rev}";
            src = self;
            nativeBuildInputs = qtNbi;
            buildInputs = qtPackages;
            sconsFlags = "";
            enableParallelBuilding = true;
            cmakeFlags = [ "-DCMAKE_PREFIX_PATH=${pkgs.qt6.qtbase}/lib/cmake" ];
          };
        };
      }
    );
}
