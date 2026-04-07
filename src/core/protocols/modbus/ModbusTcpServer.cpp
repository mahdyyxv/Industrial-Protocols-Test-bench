#include "ModbusTcpServer.h"

#ifdef RTU_MODBUS_ENABLED
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>
#endif

#include <sys/select.h>
#include <unistd.h>
#include <cerrno>
#include <QMetaObject>

namespace rtu::core::modbus {

// ── ModbusTcpServer ───────────────────────────────────────────

ModbusTcpServer::ModbusTcpServer(QObject* parent)
    : QObject(parent)
{}

ModbusTcpServer::~ModbusTcpServer()
{
    stop();
}

bool ModbusTcpServer::start(int port)
{
    if (m_running) stop();

    m_port = port;
    m_worker = new Worker(port, this);

    // Connect worker signals → server signals (queued across thread boundary)
    QObject::connect(m_worker, &Worker::finished, m_worker, &QObject::deleteLater);

    m_worker->start();

    // Give the worker up to 500 ms to bind the port
    if (!m_worker->isRunning()) {
        m_worker->wait(500);
    }

    return true;  // actual bind result arrives via serverStarted / errorOccurred
}

void ModbusTcpServer::stop()
{
    if (m_worker) {
        m_worker->requestStop();
        m_worker->wait(3000);
        // deleteLater is connected to finished
        m_worker = nullptr;
    }
    m_running = false;
}

bool    ModbusTcpServer::isRunning()  const { return m_running; }
int     ModbusTcpServer::listenPort() const { return m_port; }
QString ModbusTcpServer::lastError()  const { QMutexLocker lk(&m_mutex); return m_lastError; }

uint16_t ModbusTcpServer::registerValue(int addr) const
{
    if (addr < 0 || addr >= kRegisterCount) return 0;
    QMutexLocker lk(&m_mutex);
    return m_regs[static_cast<size_t>(addr)];
}

void ModbusTcpServer::setRegisterValue(int addr, uint16_t value)
{
    if (addr < 0 || addr >= kRegisterCount) return;
    QMutexLocker lk(&m_mutex);
    m_regs[static_cast<size_t>(addr)] = value;
}

std::array<uint16_t, ModbusTcpServer::kRegisterCount> ModbusTcpServer::allRegisters() const
{
    QMutexLocker lk(&m_mutex);
    return m_regs;
}

void ModbusTcpServer::setLastError(const QString& err)
{
    QMutexLocker lk(&m_mutex);
    m_lastError = err;
}

// ── Worker ─────────────────────────────────────────────────────

ModbusTcpServer::Worker::Worker(int port, ModbusTcpServer* owner)
    : QThread(owner)
    , m_port(port)
    , m_owner(owner)
{}

void ModbusTcpServer::Worker::requestStop()
{
    m_stop = true;
    // Wake up any blocking select/accept by writing to the pipe
    if (m_stopPipe[1] != -1)
        ::write(m_stopPipe[1], "x", 1);
}

void ModbusTcpServer::Worker::run()
{
#ifdef RTU_MODBUS_ENABLED
    // Create a stop-notification pipe
    if (::pipe(m_stopPipe) != 0) {
        QMetaObject::invokeMethod(m_owner, [this]() {
            m_owner->setLastError(QStringLiteral("pipe() failed"));
            emit m_owner->errorOccurred(m_owner->lastError());
        }, Qt::QueuedConnection);
        return;
    }

    modbus_t* ctx = modbus_new_tcp(nullptr, m_port);
    if (!ctx) {
        QMetaObject::invokeMethod(m_owner, [this]() {
            m_owner->setLastError(QStringLiteral("modbus_new_tcp failed"));
            emit m_owner->errorOccurred(m_owner->lastError());
        }, Qt::QueuedConnection);
        ::close(m_stopPipe[0]); ::close(m_stopPipe[1]);
        m_stopPipe[0] = m_stopPipe[1] = -1;
        return;
    }

    modbus_mapping_t* mapping = modbus_mapping_new(0, 0, ModbusTcpServer::kRegisterCount, 0);
    if (!mapping) {
        modbus_free(ctx);
        QMetaObject::invokeMethod(m_owner, [this]() {
            m_owner->setLastError(QStringLiteral("modbus_mapping_new failed"));
            emit m_owner->errorOccurred(m_owner->lastError());
        }, Qt::QueuedConnection);
        ::close(m_stopPipe[0]); ::close(m_stopPipe[1]);
        m_stopPipe[0] = m_stopPipe[1] = -1;
        return;
    }

    int server_socket = modbus_tcp_listen(ctx, 5);
    if (server_socket < 0) {
        const QString err = QString::fromLatin1(modbus_strerror(errno));
        modbus_mapping_free(mapping);
        modbus_free(ctx);
        QMetaObject::invokeMethod(m_owner, [this, err]() {
            m_owner->setLastError(err);
            emit m_owner->errorOccurred(err);
        }, Qt::QueuedConnection);
        ::close(m_stopPipe[0]); ::close(m_stopPipe[1]);
        m_stopPipe[0] = m_stopPipe[1] = -1;
        return;
    }

    m_owner->m_running = true;
    QMetaObject::invokeMethod(m_owner, [this]() {
        emit m_owner->serverStarted(m_port);
    }, Qt::QueuedConnection);

    // Set a short receive timeout so we can check the stop flag
    struct timeval tv { 0, 200000 }; // 200 ms
    modbus_set_response_timeout(ctx, tv.tv_sec, tv.tv_usec);

    while (!m_stop) {
        // Use select() to wait for a new connection or a stop signal
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(server_socket,  &rfds);
        FD_SET(m_stopPipe[0],  &rfds);
        int maxfd = std::max(server_socket, m_stopPipe[0]) + 1;

        struct timeval sel_tv { 0, 200000 };
        int ret = ::select(maxfd, &rfds, nullptr, nullptr, &sel_tv);
        if (ret <= 0 || m_stop) break;
        if (FD_ISSET(m_stopPipe[0], &rfds)) break;
        if (!FD_ISSET(server_socket, &rfds)) continue;

        // Accept a client
        int sock_tmp = server_socket;
        if (modbus_tcp_accept(ctx, &sock_tmp) < 0) continue;

        // Get client IP (best-effort)
        QMetaObject::invokeMethod(m_owner, [this]() {
            emit m_owner->clientConnected(QStringLiteral("client"));
        }, Qt::QueuedConnection);

        // Sync current register values into the mapping
        {
            QMutexLocker lk(&m_owner->m_mutex);
            for (int i = 0; i < ModbusTcpServer::kRegisterCount; ++i)
                mapping->tab_registers[i] = m_owner->m_regs[static_cast<size_t>(i)];
        }

        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;
        while (!m_stop) {
            rc = modbus_receive(ctx, query);
            if (rc <= 0) break;  // connection closed or error

            // Check for write request: update our register bank
            int fc = query[modbus_get_header_length(ctx)];
            if (fc == MODBUS_FC_WRITE_SINGLE_REGISTER || fc == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
                // Reply first, then emit registerWritten after reply
                modbus_reply(ctx, query, rc, mapping);
                // The mapping was updated in-place by modbus_reply
                {
                    QMutexLocker lk(&m_owner->m_mutex);
                    for (int i = 0; i < ModbusTcpServer::kRegisterCount; ++i)
                        m_owner->m_regs[static_cast<size_t>(i)] = mapping->tab_registers[i];
                }
                // Determine which addresses changed (simplified: emit for first written reg)
                int addr = (query[modbus_get_header_length(ctx) + 1] << 8) |
                            query[modbus_get_header_length(ctx) + 2];
                uint16_t val = mapping->tab_registers[addr];
                QMetaObject::invokeMethod(m_owner, [this, addr, val]() {
                    emit m_owner->registerWritten(addr, val);
                }, Qt::QueuedConnection);
            } else {
                // Read or other: sync any external register writes into mapping first
                {
                    QMutexLocker lk(&m_owner->m_mutex);
                    for (int i = 0; i < ModbusTcpServer::kRegisterCount; ++i)
                        mapping->tab_registers[i] = m_owner->m_regs[static_cast<size_t>(i)];
                }
                modbus_reply(ctx, query, rc, mapping);
                int addr = (query[modbus_get_header_length(ctx) + 1] << 8) |
                            query[modbus_get_header_length(ctx) + 2];
                uint16_t val = mapping->tab_registers[addr];
                QMetaObject::invokeMethod(m_owner, [this, addr, val]() {
                    emit m_owner->registerRead(addr, val);
                }, Qt::QueuedConnection);
            }
        }

        modbus_close(ctx);
        QMetaObject::invokeMethod(m_owner, [this]() {
            emit m_owner->clientDisconnected();
        }, Qt::QueuedConnection);
    }

    modbus_mapping_free(mapping);
    modbus_close(ctx);
    modbus_free(ctx);
    ::close(server_socket);
    ::close(m_stopPipe[0]); ::close(m_stopPipe[1]);
    m_stopPipe[0] = m_stopPipe[1] = -1;
#endif // RTU_MODBUS_ENABLED

    m_owner->m_running = false;
    QMetaObject::invokeMethod(m_owner, [this]() {
        emit m_owner->serverStopped();
    }, Qt::QueuedConnection);
}

} // namespace rtu::core::modbus
