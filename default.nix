let
  pkgs = import <nixpkgs> {};
in pkgs.mkShell rec {
  buildInputs = [
    pkgs.meson
    pkgs.ninja
  ];
}
