{
  description = "fauxboy";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      perSystem = { pkgs, lib, ... }: let
        cmake = pkgs.cmake;

        ninja = pkgs.ninja.override {
          buildDocs = false;
        };
        
        catch2 = pkgs.catch2.overrideAttrs rec {
          version = "3.8.1";

          src = pkgs.fetchFromGitHub {
            owner = "catchorg";
            repo = "Catch2";
            rev = "v${version}";
            sha256 = "0v1k14n02aiw4rv5sxhc5612cjhkdj59cjpm50qfxhapsdv54n3f";
          };
        };

        simdjson = pkgs.simdjson;

        filesystem_pkgs = []
          ++ lib.optionals pkgs.stdenv.isLinux [ pkgs.inotify-tools ]
          ++ lib.optionals pkgs.stdenv.isDarwin (with pkgs.darwin.apple_sdk.frameworks; [ CoreFoundation CoreServices ]);
      in {
        devShells.default = pkgs.mkShell.override { stdenv = pkgs.gcc15Stdenv; } {
          # Disable unnecessary compiler flags
          hardeningDisable = [ "all" ];

          nativeBuildInputs = [
            cmake
            ninja
          ];
          
          buildInputs = [
            catch2
            simdjson
          ] ++ filesystem_pkgs;

          # Environment variables
          LOCALE_ARCHIVE = if pkgs.stdenv.isLinux then "${pkgs.glibcLocales}/lib/locale/locale-archive" else "";
          LANG = "C.UTF-8";
        };
      };
  };
}
