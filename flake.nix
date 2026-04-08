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

        rev = self.rev or "dirty";
        version = pkgs.runCommand "version" { } ''
          SC_MAJOR_VERSION=$(cat ${self}/engine/config.hpp | grep "#define SC_MAJOR_VERSION" | awk '{ print $3 }' | tr -d '"')
          SC_MINOR_VERSION=$(cat ${self}/engine/config.hpp | grep "#define SC_MINOR_VERSION" | awk '{ print $3 }' | tr -d '"')
          echo "$SC_MAJOR_VERSION"-"$SC_MINOR_VERSION" >> $out
        '';
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
            version = "${builtins.readFile version}${rev}";
            src = self;
            nativeBuildInputs = nbi;
            buildInputs = packages;
            sconsFlags = "";
            enableParallelBuilding = true;
            cmakeFlags = [ "-DBUILD_GUI=OFF" ];
          };

          simcqt = llvm.stdenv.mkDerivation {
            pname = "simcqt";
            version = "${builtins.readFile version}${rev}";
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
