args @ {pkgs ? import <nixpkgs> {}, ...}: pkgs.mkShell (import ./default.nix args)
