{
  description = "NixOS configuration for Bitcoin Core IPC monitoring";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    disko.url = "github:nix-community/disko";
    disko.inputs.nixpkgs.follows = "nixpkgs";
    sops-nix.url = "github:Mic92/sops-nix";
    sops-nix.inputs.nixpkgs.follows = "nixpkgs";
    ipc-exporter.url = "git+file:../ipc-exporter-rust";
    ipc-exporter.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    {
      nixpkgs,
      disko,
      sops-nix,
      ipc-exporter,
      ...
    }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
      settings = {
        hostName = "ipc-dash";
        domain = "tracing.fish.foo";
        networkInterface = "enp1s0";
        ipAddress = "188.245.67.250";
        prefixLength = 32;
        gateway = "172.31.1.1";
        sshKeys = [
          "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIH988C5DbEPHfoCphoW23MWq9M6fmA4UTXREiZU0J7n0 will.hetzner@temp.com"
        ];
      };
    in
    {
      nixosConfigurations.default = nixpkgs.lib.nixosSystem {
        system = "x86_64-linux";
        specialArgs = {
          inherit settings ipc-exporter;
        };
        modules = [
          disko.nixosModules.disko
          sops-nix.nixosModules.sops
          ipc-exporter.nixosModules.default
          { nixpkgs.overlays = [ ipc-exporter.overlays.default ]; }
          ./disk-config.nix
          ./hardware-configuration.nix
          ./configuration.nix
        ];
      };

      devShells = forAllSystems (system: {
        default = nixpkgs.legacyPackages.${system}.mkShell {
          packages = with nixpkgs.legacyPackages.${system}; [
            just
            sops
            age
            ssh-to-age
            nixos-anywhere
            nixfmt-tree
          ];
        };
      });

      formatter = forAllSystems (system: nixpkgs.legacyPackages.${system}.nixfmt-tree);
    };
}
