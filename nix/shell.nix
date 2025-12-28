args @ {pkgs ? import <nixpkgs> {}, ...}: pkgs.mkShell (import ./package.nix args)
