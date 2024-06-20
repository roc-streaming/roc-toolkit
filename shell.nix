{ pkgs ? import <nixpkgs> {} }:

  pkgs.mkShell {
    buildInputs = [
      # build deps
      pkgs.autoconf
      pkgs.automake
      pkgs.clang
      pkgs.cmake
      pkgs.gcc
      pkgs.gengetopt
      pkgs.gnumake
      pkgs.intltool
      pkgs.libtool
      pkgs.meson
      pkgs.pkg-config
      pkgs.ragel
      pkgs.scons

      # other deps
      pkgs.libpulseaudio
      pkgs.libsndfile
      pkgs.libunwind
      pkgs.libuv
      pkgs.openssl
      pkgs.sox
      pkgs.speexdsp

      # optional deps: formatting, tests, ...
      pkgs.clang-tools
      pkgs.cpputest
      pkgs.gbenchmark
    ];
  }
