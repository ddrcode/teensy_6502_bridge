{ pkgs ? import <nixpkgs> { } }:

let
  inherit (pkgs.lib) optionals;
  inherit (pkgs.stdenv) isDarwin;
in

pkgs.mkShell {
  packages = with pkgs; [
    acme
    # arduino-cli
    arduino-language-server
    cargo
    catch2_3
    rustc
    rustfmt
    cpplint
    treefmt
    astyle
    pkg-config
    udev
    teensyduino
    teensy-udev-rules
    teensy-loader-cli
    # python311Packages.mdit-py-plugins
    # python311Packages.mdformat
    # python311Packages.mdformat-gfm
  ] ++ optionals isDarwin [
    darwin.libiconv
    darwin.IOKit
  ];
}
