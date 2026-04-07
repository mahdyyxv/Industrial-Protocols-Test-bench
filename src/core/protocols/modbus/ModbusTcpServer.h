#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QString>
#include <array>
#include <atomic>

struct _modbus;

namespace rtu::core::modbus {

// ──────────────────────────────────────────────────────────────
// ModbusTcpServer
//
// Modbus TCP slave / server.  Listens on a TCP port and serves
// holding-register read/write requests from any master.
//
// The server runs on an internal QThread.  All signals are emitted
// on that thread and automatically queued when connected to objects
// on the main thread.
//
// Register bank: 256 holding registers (addresses 0-255).
// Thread-safe: setRegisterValue / registerValue may be called from
// any thread while the server is running.
// ──────────────────────────────────────────────────────────────
class ModbusTcpServer : public QObject {
    Q_OBJECT

public:
    static constexpr int kRegisterCount = 256;

    explicit ModbusTcpServer(QObject* parent = nullptr);
    ~ModbusTcpServer() override;

    // Start listening on the given port.  Returns false immediately if
    // the port cannot be bound (emits errorOccurred).
    bool start(int port = 502);

    // Stop the server.  Blocks until the worker thread exits.
    void stop();

    bool isRunning() const;
    int  listenPort() const;
    QString lastError() const;

    // Register bank — thread-safe
    uint16_t registerValue(int addr) const;
    void     setRegisterValue(int addr, uint16_t value);

    // Returns a snapshot of the full register bank
    std::array<uint16_t, kRegisterCount> allRegisters() const;

signals:
    void serverStarted(int port);
    void serverStopped();
    void clientConnected(const QString& clientIp);
    void clientDisconnected();
    void registerRead   (int addr, uint16_t value);
    void registerWritten(int addr, uint16_t value);
    void errorOccurred  (const QString& message);

private:
    // ── Worker thread ─────────────────────────────────────────
    class Worker : public QThread {
    public:
        Worker(int port, ModbusTcpServer* owner);
        void requestStop();

    protected:
        void run() override;

    private:
        int                m_port;
        ModbusTcpServer*   m_owner;
        std::atomic<bool>  m_stop { false };
        int                m_stopPipe[2] { -1, -1 };
    };

    mutable QMutex                        m_mutex;
    std::array<uint16_t, kRegisterCount>  m_regs {};
    Worker*                               m_worker { nullptr };
    int                                   m_port   { 502 };
    std::atomic<bool>                     m_running { false };
    QString                               m_lastError;

    void setLastError(const QString& err);
    friend class Worker;
};

} // namespace rtu::core::modbus
