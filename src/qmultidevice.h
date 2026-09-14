#pragma once

#include <QtNetwork/QNetworkAddressEntry>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtSerialPort/QSerialPort>

class QMultiDevice : public QObject
{
    Q_OBJECT

    static constexpr uint32_t default_receive_timeout = 500;
    static constexpr uint32_t default_connect_timeout = 200;

public:
    enum ConnectionType
    {
        Unknonwn,
        Serial,
        UDPSocket,
        TCPSocket,
        TCPServer
    };

    struct IPPORT
    {
        uint32_t host;
        uint16_t port;
        IPPORT(uint32_t h = 0, uint16_t p = 0) : host(h), port(p) {}
        IPPORT(const QString& a)
        {
            auto a_list = a.split(':');
            if (a_list[0].compare("localhost", Qt::CaseInsensitive) == 0)
            {
                host = QHostAddress(QHostAddress::LocalHost).toIPv4Address();
            }
            else
            {
                host = QHostAddress(a_list[0]).toIPv4Address();
            }
            port = a_list.size() > 1 ? a_list[1].toUInt() : 0;
        }
        bool valid() const
        {
            return host && port;
        }

        bool operator==(const IPPORT& rhs) const
        {
            return (rhs.host == host) && (rhs.port == port);
        }
    };

    const QMap<char, QSerialPort::Parity> serialParityValues = {{'N', QSerialPort::NoParity},
                                                                {'E', QSerialPort::EvenParity},
                                                                {'O', QSerialPort::OddParity},
                                                                {'S', QSerialPort::SpaceParity},
                                                                {'M', QSerialPort::MarkParity}};

    explicit QMultiDevice(QObject* parent = nullptr);
    ~QMultiDevice();

    bool isActive() const;
    ConnectionType connectionType() const;
    QPair<int, QString> lastError() const;

    bool close();
    bool closePeer(const QString& peer = {});
    bool openSerial(const QString& port,
                    const uint32_t& baudrate = 921600,
                    const char& parity = 'N');
    QString serialPortName() const;
    uint32_t serialBaudrate() const;
    char serialParity() const;

    bool bindUDPSocket(const QString& remote_host,
                       const uint16_t& remote_port,
                       const uint16_t& local_port = 0);
    bool bindUDPSocket(const QHostAddress& remote_host,
                       const uint16_t& remote_port,
                       const uint16_t& local_port = 0);

    bool openTCPSocket(const QString& remote_host, const uint16_t& remote_port);
    bool openTCPSocket(const QHostAddress& remote_host, const uint16_t& remote_port);

    bool bindTCPServer(const uint16_t& local_port);
    QList<QTcpSocket*> tcpServerPeers() const&;

    QHostAddress remoteHost() const;
    QHostAddress localHost() const;
    uint16_t remotePort() const;
    uint16_t localPort() const;

    uint32_t write(const QByteArray& data);
    uint32_t writeTo(const QString& dst, const QByteArray& data);
    uint32_t available();
    uint32_t wait(const uint32_t& expected_bytes, const uint32_t& timeout_ms);
    QByteArray readExpected(const uint32_t& expected_bytes = 0);
    QByteArray read(const uint32_t& expected_bytes = 0, const uint32_t& timeout_ms = 0);
    QByteArray transceive(const QByteArray& data,
                          const uint32_t& expected_bytes = 0,
                          const uint32_t& timeout_ms = 0);

    // Static section
    static QStringList serialBaudrates();
    static QPair<QStringList, int> getSerialPorts(const QString& current_portname,
                                                  const QPair<uint16_t, uint16_t>& vid_pid = {});
    static QPair<uint16_t, uint16_t> getSerialVIDPID(const QString& port_name);
    static QList<QNetworkAddressEntry> getNetworkAddresses();
    static QHostAddress getLocalAddress(const QHostAddress& remote_host);
    static QStringList getAvailableIPs();

signals:
    void connected(const ConnectionType& connection_type);
    void disconnected(const ConnectionType& connection_type);
    void errorOccurred(const QString& error);
    void interfaceStatus(const QString& status);
    void dataTransmitted(const QByteArray& data);
    void dataReceived(const QByteArray& data);

    void dataTransmittedTo(const QByteArray& data, const QString& dst);
    void dataReceivedFrom(const QByteArray& data, const QString& src);
    void peerConnected(const QString& peer);
    void peerDisconnected(const QString& peer);

    // For async reading
    void readyRead();

private slots:
    void serialErrorOccurred(const QSerialPort::SerialPortError& error);
    void socketErrorOccurred(const QAbstractSocket::SocketError& error);
    void tcpServerNewConnection();
    void tcpServerCloseConnection();

protected:
    int m_last_error = 0;
    QString m_last_error_string;

    // Serial port
    QSerialPort* m_serial = nullptr;

    // UDP socket
    QUdpSocket* m_udp_socket = nullptr;

    // TCP sockets
    QTcpSocket* m_tcp_socket = nullptr;
    QTcpServer* m_tcp_server = nullptr;
    QList<QTcpSocket*> m_tcp_server_sockets;

    // Last active peer
    IPPORT m_remote_address;

    void clearError();
    static bool equalSocketAddress(const QTcpSocket& l_socket, const IPPORT& r_address);
};
