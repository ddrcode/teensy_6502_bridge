{ pkgs ? import <nixpkgs> { } }:

let
  inherit (pkgs.lib) optionals;
  inherit (pkgs.stdenv) isDarwin;
  inherit (pkgs.stdenv) isLinux;
in

pkgs.mkShell {
  packages = with pkgs; [
    acme
    arduino-cli
    arduino-language-server
    
    # Rust - use rustup to manage toolchains (handles cross-compilation targets)
    # After entering the shell, run: rustup default stable
    # For Teensy 4.1 firmware: rustup target add thumbv7em-none-eabihf
    rustup
    
    # C++ tooling
    catch2_3
    cpplint
    
    # Formatting
    treefmt
    astyle
    
    # Build tools
    pkg-config
    teensy-loader-cli
  ] ++ optionals isDarwin [
    darwin.libiconv
    # darwin.IOKit
  ] ++ optionals isLinux [
    teensy-udev-rules
    teensyduino
    udev
  ];
  
  # Ensure rustup-managed toolchain takes precedence
  shellHook = ''
    export PATH="$HOME/.cargo/bin:$PATH"
  '';
}
