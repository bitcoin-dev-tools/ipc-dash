#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <interfaces/node.h>
#include <ipc/exception.h>
#include <univalue.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ipc {
namespace capnp {
void SetupNodeClient(ipc::Context& context);
} // namespace capnp
} // namespace ipc

namespace {

struct Config {
    std::string ipc_connect{"unix"};
    int metrics_port{9437};
    int poll_interval_seconds{5};
    int rpc_interval_seconds{15};
};

std::atomic<bool> g_running{true};

struct Metrics {
    mutable std::mutex mutex;

    double exporter_up{0};
    double exporter_reconnects_total{0};
    double exporter_errors_total{0};

    double blockchain_blocks{0};
    double blockchain_headers{0};
    double blockchain_verification_progress{0};
    double blockchain_difficulty{0};
    double blockchain_size_bytes{0};
    double block_height{0};

    double mempool_size{0};
    double mempool_bytes{0};
    double mempool_usage_bytes{0};
    double mempool_max_bytes{0};
    double mempool_minfee_per_kb{0};

    double net_bytes_recv_total{0};
    double net_bytes_sent_total{0};
    double net_connections_in{0};
    double net_connections_out{0};
    double connections_inbound{0};
    double connections_outbound{0};

    double block_transactions{0};
    double block_inputs{0};

    double mempool_added_total{0};
    double mempool_replaced_total{0};
    std::map<std::string, double> mempool_removed_total;

    std::string Render() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::ostringstream out;
        out.setf(std::ios::fixed);

        out << "# HELP bitcoind_ipc_exporter_up 1 if IPC session is healthy\\n";
        out << "# TYPE bitcoind_ipc_exporter_up gauge\\n";
        out << "bitcoind_ipc_exporter_up " << exporter_up << "\\n";

        out << "# HELP bitcoind_ipc_exporter_reconnects_total IPC reconnect count\\n";
        out << "# TYPE bitcoind_ipc_exporter_reconnects_total counter\\n";
        out << "bitcoind_ipc_exporter_reconnects_total " << exporter_reconnects_total << "\\n";

        out << "# HELP bitcoind_ipc_exporter_errors_total Exporter error count\\n";
        out << "# TYPE bitcoind_ipc_exporter_errors_total counter\\n";
        out << "bitcoind_ipc_exporter_errors_total " << exporter_errors_total << "\\n";

        out << "# HELP bitcoind_blockchain_blocks Current block count\\n";
        out << "# TYPE bitcoind_blockchain_blocks gauge\\n";
        out << "bitcoind_blockchain_blocks " << blockchain_blocks << "\\n";

        out << "# HELP bitcoind_blockchain_headers Current header count\\n";
        out << "# TYPE bitcoind_blockchain_headers gauge\\n";
        out << "bitcoind_blockchain_headers " << blockchain_headers << "\\n";

        out << "# HELP bitcoind_blockchain_verification_progress Chain verification progress\\n";
        out << "# TYPE bitcoind_blockchain_verification_progress gauge\\n";
        out << "bitcoind_blockchain_verification_progress " << blockchain_verification_progress << "\\n";

        out << "# HELP bitcoind_blockchain_difficulty Current difficulty\\n";
        out << "# TYPE bitcoind_blockchain_difficulty gauge\\n";
        out << "bitcoind_blockchain_difficulty " << blockchain_difficulty << "\\n";

        out << "# HELP bitcoind_blockchain_size_bytes Estimated blockchain size on disk\\n";
        out << "# TYPE bitcoind_blockchain_size_bytes gauge\\n";
        out << "bitcoind_blockchain_size_bytes " << blockchain_size_bytes << "\\n";

        out << "# HELP bitcoind_block_height Latest connected block height\\n";
        out << "# TYPE bitcoind_block_height gauge\\n";
        out << "bitcoind_block_height " << block_height << "\\n";

        out << "# HELP bitcoind_mempool_size Transactions in mempool\\n";
        out << "# TYPE bitcoind_mempool_size gauge\\n";
        out << "bitcoind_mempool_size " << mempool_size << "\\n";

        out << "# HELP bitcoind_mempool_bytes Mempool size in vbytes\\n";
        out << "# TYPE bitcoind_mempool_bytes gauge\\n";
        out << "bitcoind_mempool_bytes " << mempool_bytes << "\\n";

        out << "# HELP bitcoind_mempool_usage_bytes Mempool memory usage\\n";
        out << "# TYPE bitcoind_mempool_usage_bytes gauge\\n";
        out << "bitcoind_mempool_usage_bytes " << mempool_usage_bytes << "\\n";

        out << "# HELP bitcoind_mempool_max_bytes Maximum mempool size\\n";
        out << "# TYPE bitcoind_mempool_max_bytes gauge\\n";
        out << "bitcoind_mempool_max_bytes " << mempool_max_bytes << "\\n";

        out << "# HELP bitcoind_mempool_minfee_per_kb Minimum fee rate for mempool acceptance (BTC/kB)\\n";
        out << "# TYPE bitcoind_mempool_minfee_per_kb gauge\\n";
        out << "bitcoind_mempool_minfee_per_kb " << mempool_minfee_per_kb << "\\n";

        out << "# HELP bitcoind_net_bytes_recv_total Total bytes received\\n";
        out << "# TYPE bitcoind_net_bytes_recv_total gauge\\n";
        out << "bitcoind_net_bytes_recv_total " << net_bytes_recv_total << "\\n";

        out << "# HELP bitcoind_net_bytes_sent_total Total bytes sent\\n";
        out << "# TYPE bitcoind_net_bytes_sent_total gauge\\n";
        out << "bitcoind_net_bytes_sent_total " << net_bytes_sent_total << "\\n";

        out << "# HELP bitcoind_net_connections_in Inbound connections\\n";
        out << "# TYPE bitcoind_net_connections_in gauge\\n";
        out << "bitcoind_net_connections_in " << net_connections_in << "\\n";

        out << "# HELP bitcoind_net_connections_out Outbound connections\\n";
        out << "# TYPE bitcoind_net_connections_out gauge\\n";
        out << "bitcoind_net_connections_out " << net_connections_out << "\\n";

        out << "# HELP bitcoind_connections_inbound Current inbound connections\\n";
        out << "# TYPE bitcoind_connections_inbound gauge\\n";
        out << "bitcoind_connections_inbound " << connections_inbound << "\\n";

        out << "# HELP bitcoind_connections_outbound Current outbound connections\\n";
        out << "# TYPE bitcoind_connections_outbound gauge\\n";
        out << "bitcoind_connections_outbound " << connections_outbound << "\\n";

        out << "# HELP bitcoind_block_transactions Transactions in last connected block\\n";
        out << "# TYPE bitcoind_block_transactions gauge\\n";
        out << "bitcoind_block_transactions " << block_transactions << "\\n";

        out << "# HELP bitcoind_block_inputs Inputs in last connected block\\n";
        out << "# TYPE bitcoind_block_inputs gauge\\n";
        out << "bitcoind_block_inputs " << block_inputs << "\\n";

        out << "# HELP bitcoind_mempool_added_total Transactions added to mempool\\n";
        out << "# TYPE bitcoind_mempool_added_total counter\\n";
        out << "bitcoind_mempool_added_total " << mempool_added_total << "\\n";

        out << "# HELP bitcoind_mempool_replaced_total Transactions replaced in mempool\\n";
        out << "# TYPE bitcoind_mempool_replaced_total counter\\n";
        out << "bitcoind_mempool_replaced_total " << mempool_replaced_total << "\\n";

        out << "# HELP bitcoind_mempool_removed_total Transactions removed from mempool\\n";
        out << "# TYPE bitcoind_mempool_removed_total counter\\n";
        for (const auto& [labels, value] : mempool_removed_total) {
            out << "bitcoind_mempool_removed_total{" << labels << "} " << value << "\\n";
        }

        return out.str();
    }
};

class HttpServer {
public:
    HttpServer(int port, const Metrics& metrics)
        : m_port(port), m_metrics(metrics)
    {
    }

    ~HttpServer() { Stop(); }

    void Start()
    {
        m_thread = std::thread([this] { Run(); });
    }

    void Stop()
    {
        m_running.store(false);
        if (m_fd >= 0) {
            shutdown(m_fd, SHUT_RDWR);
            close(m_fd);
            m_fd = -1;
        }
        if (m_thread.joinable()) m_thread.join();
    }

private:
    void Run()
    {
        m_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_fd < 0) throw std::runtime_error("failed to create socket");

        int one = 1;
        setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(m_port));
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            throw std::runtime_error("failed to bind metrics socket");
        }
        if (listen(m_fd, 16) != 0) throw std::runtime_error("failed to listen on metrics socket");

        while (m_running.load()) {
            int client = accept(m_fd, nullptr, nullptr);
            if (client < 0) {
                if (!m_running.load()) return;
                continue;
            }

            char req_buf[1024];
            const ssize_t n = read(client, req_buf, sizeof(req_buf) - 1);
            if (n <= 0) {
                close(client);
                continue;
            }
            req_buf[n] = '\0';
            const std::string req(req_buf);
            const bool metrics_path = req.rfind("GET /metrics", 0) == 0;

            if (!metrics_path) {
                const char* body = "not found\n";
                const std::string response =
                    "HTTP/1.1 404 Not Found\\r\\n"
                    "Content-Type: text/plain\\r\\n"
                    "Content-Length: " + std::to_string(std::strlen(body)) + "\\r\\n\\r\\n" + body;
                write(client, response.data(), response.size());
                close(client);
                continue;
            }

            const std::string body = m_metrics.Render();
            const std::string response =
                "HTTP/1.1 200 OK\\r\\n"
                "Content-Type: text/plain; version=0.0.4\\r\\n"
                "Content-Length: " + std::to_string(body.size()) + "\\r\\n\\r\\n" + body;
            write(client, response.data(), response.size());
            close(client);
        }
    }

    int m_port;
    const Metrics& m_metrics;
    std::atomic<bool> m_running{true};
    int m_fd{-1};
    std::thread m_thread;
};

class Exporter;

class Notifications : public interfaces::Chain::Notifications {
public:
    explicit Notifications(Exporter& exporter) : m_exporter(exporter) {}

    void transactionAddedToMempool(const CTransactionRef& tx) override;
    void transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason) override;
    void blockConnected(const kernel::ChainstateRole& role, const interfaces::BlockInfo& block) override;

private:
    Exporter& m_exporter;
};

class Exporter {
public:
    explicit Exporter(Config config) : m_config(std::move(config)) {}

    void Run()
    {
        HttpServer http(m_config.metrics_port, m_metrics);
        http.Start();

        auto last_poll = std::chrono::steady_clock::now() - std::chrono::seconds(m_config.poll_interval_seconds);
        auto last_rpc_poll = std::chrono::steady_clock::now() - std::chrono::seconds(m_config.rpc_interval_seconds);

        while (g_running.load()) {
            try {
                if (!m_node || !m_chain) {
                    Connect();
                    std::lock_guard<std::mutex> lock(m_metrics.mutex);
                    m_metrics.exporter_up = 1;
                }

                auto now = std::chrono::steady_clock::now();
                if (now - last_poll >= std::chrono::seconds(m_config.poll_interval_seconds)) {
                    PollDirectMetrics();
                    last_poll = now;
                }
                if (now - last_rpc_poll >= std::chrono::seconds(m_config.rpc_interval_seconds)) {
                    PollRpcFallback();
                    last_rpc_poll = now;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            } catch (const ipc::Exception& e) {
                HandleError(std::string("IPC exception: ") + e.what());
            } catch (const std::exception& e) {
                HandleError(std::string("Exporter exception: ") + e.what());
            }
        }
    }

    void OnTxAdded(const CTransactionRef& tx)
    {
        const std::string txid = tx->GetHash().ToString();
        std::lock_guard<std::mutex> state_lock(m_state_mutex);
        const auto [_, inserted] = m_known_mempool_txids.insert(txid);
        if (!inserted) return;

        std::lock_guard<std::mutex> lock(m_metrics.mutex);
        m_metrics.mempool_added_total += 1;
    }

    void OnTxRemoved(const CTransactionRef& tx, MemPoolRemovalReason reason)
    {
        const std::string txid = tx->GetHash().ToString();
        std::lock_guard<std::mutex> state_lock(m_state_mutex);
        if (!m_known_mempool_txids.erase(txid)) return;

        const std::string reason_label = "reason=\"code_" + std::to_string(static_cast<int>(reason)) + "\"";

        std::lock_guard<std::mutex> lock(m_metrics.mutex);
        m_metrics.mempool_removed_total[reason_label] += 1;
        if (reason_label == "reason=\"code_5\"") {
            m_metrics.mempool_replaced_total += 1;
        }
    }

    void OnBlockConnected(const interfaces::BlockInfo& block)
    {
        int tx_count = 0;
        int input_count = 0;
        if (block.data != nullptr) {
            tx_count = static_cast<int>(block.data->vtx.size());
            for (const auto& tx : block.data->vtx) {
                input_count += static_cast<int>(tx->vin.size());
            }
        }

        std::lock_guard<std::mutex> lock(m_metrics.mutex);
        m_metrics.block_height = block.height;
        if (tx_count > 0) m_metrics.block_transactions = tx_count;
        if (input_count > 0) m_metrics.block_inputs = input_count;
    }

private:
    class ClientInit : public interfaces::Init {
    public:
        explicit ClientInit(const char* argv0)
            : m_ipc(interfaces::MakeIpc("bitcoind-ipc-exporter", ".ipc-exporter", argv0, *this))
        {
            ipc::capnp::SetupNodeClient(m_ipc->context());
        }

        interfaces::Ipc* ipc() override { return m_ipc.get(); }

    private:
        std::unique_ptr<interfaces::Ipc> m_ipc;
    };

    void Connect()
    {
        m_notifications_handler.reset();
        m_connections_handler.reset();
        m_block_tip_handler.reset();
        m_chain.reset();
        m_node.reset();
        m_remote_init.reset();

        if (!m_local_init) {
            m_local_init = std::make_unique<ClientInit>("bitcoind-ipc-exporter");
        }

        std::string connect_addr = m_config.ipc_connect;
        m_remote_init = m_local_init->ipc()->connectAddress(connect_addr);
        if (!m_remote_init) {
            throw std::runtime_error("failed to connect to IPC address: " + m_config.ipc_connect);
        }

        m_node = m_remote_init->makeNode();
        m_chain = m_remote_init->makeChain();
        if (!m_node || !m_chain) {
            throw std::runtime_error("remote init did not provide Node and Chain interfaces");
        }

        m_notifications = std::make_shared<Notifications>(*this);
        m_notifications_handler = m_chain->handleNotifications(m_notifications);
        m_connections_handler = m_node->handleNotifyNumConnectionsChanged([this](int /*unused*/) {
            try {
                PollConnectionGauges();
            } catch (...) {
            }
        });
        m_block_tip_handler = m_node->handleNotifyBlockTip(
            [this](SynchronizationState /*state*/, interfaces::BlockTip tip, double progress) {
                std::lock_guard<std::mutex> lock(m_metrics.mutex);
                m_metrics.block_height = tip.block_height;
                m_metrics.blockchain_verification_progress = progress;
            });

        m_chain->requestMempoolTransactions(*m_notifications);

        {
            std::lock_guard<std::mutex> lock(m_metrics.mutex);
            m_metrics.exporter_reconnects_total += 1;
        }
    }

    void PollDirectMetrics()
    {
        PollConnectionGauges();

        std::lock_guard<std::mutex> lock(m_metrics.mutex);
        m_metrics.blockchain_blocks = m_node->getNumBlocks();

        int header_height = 0;
        int64_t header_time = 0;
        if (m_node->getHeaderTip(header_height, header_time)) {
            m_metrics.blockchain_headers = header_height;
        }

        m_metrics.blockchain_verification_progress = m_node->getVerificationProgress();
        m_metrics.mempool_size = m_node->getMempoolSize();
        m_metrics.mempool_usage_bytes = m_node->getMempoolDynamicUsage();
        m_metrics.mempool_max_bytes = m_node->getMempoolMaxUsage();
        m_metrics.net_bytes_recv_total = m_node->getTotalBytesRecv();
        m_metrics.net_bytes_sent_total = m_node->getTotalBytesSent();
    }

    void PollConnectionGauges()
    {
        const size_t in_count = m_node->getNodeCount(ConnectionDirection::In);
        const size_t out_count = m_node->getNodeCount(ConnectionDirection::Out);

        std::lock_guard<std::mutex> lock(m_metrics.mutex);
        m_metrics.net_connections_in = in_count;
        m_metrics.net_connections_out = out_count;
        m_metrics.connections_inbound = in_count;
        m_metrics.connections_outbound = out_count;
    }

    void PollRpcFallback()
    {
        UniValue params(UniValue::VARR);

        const UniValue chain_info = m_node->executeRpc("getblockchaininfo", params, "");
        const UniValue mempool_info = m_node->executeRpc("getmempoolinfo", params, "");

        std::lock_guard<std::mutex> lock(m_metrics.mutex);
        if (chain_info.isObject()) {
            const UniValue& difficulty = chain_info["difficulty"];
            if (difficulty.isNum()) m_metrics.blockchain_difficulty = difficulty.get_real();

            const UniValue& size_on_disk = chain_info["size_on_disk"];
            if (size_on_disk.isNum()) m_metrics.blockchain_size_bytes = size_on_disk.get_real();
        }

        if (mempool_info.isObject()) {
            const UniValue& bytes = mempool_info["bytes"];
            if (bytes.isNum()) m_metrics.mempool_bytes = bytes.get_real();

            const UniValue& minfee = mempool_info["mempoolminfee"];
            if (minfee.isNum()) m_metrics.mempool_minfee_per_kb = minfee.get_real();
        }
    }

    void HandleError(const std::string& message)
    {
        {
            std::lock_guard<std::mutex> lock(m_metrics.mutex);
            m_metrics.exporter_up = 0;
            m_metrics.exporter_errors_total += 1;
        }
        std::cerr << message << std::endl;

        m_notifications_handler.reset();
        m_connections_handler.reset();
        m_block_tip_handler.reset();
        m_notifications.reset();
        m_chain.reset();
        m_node.reset();
        m_remote_init.reset();

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    Config m_config;
    Metrics m_metrics;

    std::unique_ptr<ClientInit> m_local_init;
    std::unique_ptr<interfaces::Init> m_remote_init;
    std::unique_ptr<interfaces::Node> m_node;
    std::unique_ptr<interfaces::Chain> m_chain;

    std::shared_ptr<Notifications> m_notifications;
    std::unique_ptr<interfaces::Handler> m_notifications_handler;
    std::unique_ptr<interfaces::Handler> m_connections_handler;
    std::unique_ptr<interfaces::Handler> m_block_tip_handler;

    std::mutex m_state_mutex;
    std::set<std::string> m_known_mempool_txids;
};

void Notifications::transactionAddedToMempool(const CTransactionRef& tx)
{
    m_exporter.OnTxAdded(tx);
}

void Notifications::transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason)
{
    m_exporter.OnTxRemoved(tx, reason);
}

void Notifications::blockConnected(const kernel::ChainstateRole& /*role*/, const interfaces::BlockInfo& block)
{
    m_exporter.OnBlockConnected(block);
}

Config ParseArgs(int argc, char* argv[])
{
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto get_value = [&](const std::string& prefix) -> std::string {
            if (arg.rfind(prefix + "=", 0) == 0) return arg.substr(prefix.size() + 1);
            if (arg == prefix && i + 1 < argc) return argv[++i];
            throw std::runtime_error("missing value for " + prefix);
        };

        if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage: bitcoind-ipc-exporter [--ipc-connect ADDR] [--metrics-port PORT] "
                << "[--poll-interval SEC] [--rpc-interval SEC]\\n";
            std::exit(0);
        } else if (arg.rfind("--ipc-connect", 0) == 0) {
            cfg.ipc_connect = get_value("--ipc-connect");
        } else if (arg.rfind("--metrics-port", 0) == 0) {
            cfg.metrics_port = std::stoi(get_value("--metrics-port"));
        } else if (arg.rfind("--poll-interval", 0) == 0) {
            cfg.poll_interval_seconds = std::stoi(get_value("--poll-interval"));
        } else if (arg.rfind("--rpc-interval", 0) == 0) {
            cfg.rpc_interval_seconds = std::stoi(get_value("--rpc-interval"));
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }
    return cfg;
}

void HandleSignal(int)
{
    g_running.store(false);
}

} // namespace

int main(int argc, char* argv[])
{
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    try {
        Config config = ParseArgs(argc, argv);
        Exporter exporter(std::move(config));
        exporter.Run();
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
