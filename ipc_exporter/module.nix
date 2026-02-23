{
  config,
  lib,
  pkgs,
  ...
}:
let
  cfg = config.services.bitcoind-ipc-exporter;
in
{
  options.services.bitcoind-ipc-exporter = {
    enable = lib.mkEnableOption "Bitcoin Core IPC Prometheus exporter";

    package = lib.mkOption {
      type = lib.types.nullOr lib.types.package;
      default = null;
      description = "Package containing the bitcoind-ipc-exporter binary.";
    };

    ipcConnect = lib.mkOption {
      type = lib.types.str;
      default = "unix";
      description = "Value passed to --ipc-connect (e.g. unix, auto, unix:/path/to/node.sock).";
    };

    metricsPort = lib.mkOption {
      type = lib.types.port;
      default = 9437;
      description = "Port served by the exporter /metrics endpoint.";
    };

    pollIntervalSeconds = lib.mkOption {
      type = lib.types.ints.positive;
      default = 5;
      description = "Direct IPC polling interval in seconds.";
    };

    rpcIntervalSeconds = lib.mkOption {
      type = lib.types.ints.positive;
      default = 15;
      description = "executeRpc fallback polling interval in seconds.";
    };

    user = lib.mkOption {
      type = lib.types.str;
      default = "bitcoind-mainnet";
      description = "User running the exporter service.";
    };

    group = lib.mkOption {
      type = lib.types.str;
      default = "bitcoind-mainnet";
      description = "Group running the exporter service.";
    };

    addPrometheusScrapeConfig = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = "Whether to append a Prometheus scrape config for this exporter.";
    };
  };

  config = lib.mkIf cfg.enable {
    assertions = [
      {
        assertion = cfg.package != null;
        message = "services.bitcoind-ipc-exporter.package must be set when enabling the service.";
      }
    ];

    systemd.services.bitcoind-ipc-exporter = {
      description = "Bitcoin Core IPC Metrics Exporter";
      after = [ "bitcoind-mainnet.service" ];
      bindsTo = [ "bitcoind-mainnet.service" ];
      wantedBy = [ "multi-user.target" ];

      serviceConfig = {
        User = cfg.user;
        Group = cfg.group;
        Restart = "on-failure";
        RestartSec = 10;
        ExecStart =
          "${cfg.package}/bin/bitcoind-ipc-exporter "
          + "--ipc-connect ${lib.escapeShellArg cfg.ipcConnect} "
          + "--metrics-port ${toString cfg.metricsPort} "
          + "--poll-interval ${toString cfg.pollIntervalSeconds} "
          + "--rpc-interval ${toString cfg.rpcIntervalSeconds}";
      };
    };

    services.prometheus.scrapeConfigs = lib.mkIf cfg.addPrometheusScrapeConfig [
      {
        job_name = "bitcoind-ipc";
        static_configs = [ { targets = [ "localhost:${toString cfg.metricsPort}" ]; } ];
        scrape_interval = "5s";
      }
    ];
  };
}
