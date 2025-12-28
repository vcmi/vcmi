{
  description = "Nix flake for packaging and developing on VCMI, an open source reimplementation of HoMM III";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
  #My beloved submodules...
  inputs.self.submodules = true;
  outputs = inputs @ {
    flake-parts,
    self,
    ...
  }:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];

      perSystem = {
        config,
        self',
        inputs',
        pkgs,
        system,
        ...
      }: {
        devShells.default = import ./shell.nix {
          inherit pkgs;
          ninja = pkgs.ninja;
        };
        packages.default = pkgs.stdenv.mkDerivation (import ./default.nix {
          inherit pkgs;
          ninja = pkgs.ninja;
        });
        formatter = pkgs.alejandra;
      };
    };
}
