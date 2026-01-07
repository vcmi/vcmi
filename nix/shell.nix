args @ {
  pkgs ? import <nixpkgs> {},
  src ? (with pkgs.lib.fileset;
    toSource {
      root = ./..;
      fileset = gitTrackedWith {recurseSubmodules = true;} ./..;
    }),
  ...
}:
pkgs.mkShell (import ./package.nix (args
  // {
    inherit pkgs;
    inherit src;
  }))
