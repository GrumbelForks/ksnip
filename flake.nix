{
  description = "ksnip - cross-platform screenshot tool with annotation features";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        inherit (pkgs) kdePackages qt6;
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "ksnip";
          version = "1.11.0-dev";

          src = ./.;

          nativeBuildInputs = [
            pkgs.cmake
            kdePackages.extra-cmake-modules
            qt6.wrapQtAppsHook
            qt6.qttools
          ];

          buildInputs = [
            kdePackages.kcolorpicker
            kdePackages.kimageannotator
            qt6.qtsvg
            # X11 / XCB capture (not pulled in automatically for local src builds)
            pkgs.xorg.libX11
            pkgs.xorg.libxcb
            pkgs.xorg.xcbutil
            pkgs.libxcb
          ];

          cmakeFlags = [
            "-DBUILD_WITH_QT6=ON"
            "-DBUILD_TESTS=OFF"
            "-DUSE_SUBMODULE_KCOLORPICKER=OFF"
            "-DUSE_SUBMODULE_KIMAGEANNOTATOR=OFF"
            "-DQT_FIND_PRIVATE_MODULES=ON"
          ];

          # Desktop file ships an absolute path that is wrong under Nix.
          postPatch = ''
            substituteInPlace desktop/org.ksnip.ksnip.desktop \
              --replace-fail "Exec=/usr/bin/ksnip" "Exec=ksnip" \
              || true
          '';

          meta = with pkgs.lib; {
            description = "Cross-platform screenshot tool with many annotation features";
            homepage = "https://github.com/ksnip/ksnip";
            license = licenses.gpl2Plus;
            platforms = platforms.linux;
            mainProgram = "ksnip";
          };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          packages = [
            pkgs.cmake
            kdePackages.extra-cmake-modules
            qt6.qttools
            pkgs.gdb
            pkgs.clang-tools
          ];
          shellHook = ''
            echo "ksnip dev shell (Qt6). Example:"
            echo "  cmake -B build -DBUILD_WITH_QT6=ON -DUSE_SUBMODULE_KCOLORPICKER=OFF -DUSE_SUBMODULE_KIMAGEANNOTATOR=OFF"
            echo "  cmake --build build -j"
          '';
        };
      });
}
