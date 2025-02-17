# Development shell initialization for NixOS.
#
# Details: https://roc-streaming.org/toolkit/docs/building/dependencies.html

{ pkgs ? import <nixpkgs> {} }:

pkgs.clangStdenv.mkDerivation {
  name = "clang-shell";

  nativeBuildInputs = [
    pkgs.clang-tools
    # A properly wrapped clangd is already made available by the clang-tools derivation.
    # clang-tools has to come before clang to set precedence in PATH for clangd.
    pkgs.clang

    # build deps
    pkgs.autoconf
    pkgs.automake
    pkgs.cmake
    pkgs.gengetopt
    pkgs.gnumake
    pkgs.intltool
    pkgs.libtool
    pkgs.meson
    pkgs.pkg-config
    pkgs.ragel
    pkgs.scons
  ];

  buildInputs = [
    # other deps
    pkgs.libpulseaudio
    pkgs.libsndfile
    pkgs.libunwind
    pkgs.libuuid
    pkgs.libuv
    pkgs.openssl
    pkgs.sox
    pkgs.speexdsp

    # optional deps: formatting, tests, ...
    pkgs.cpputest
    pkgs.gbenchmark
  ];
}
