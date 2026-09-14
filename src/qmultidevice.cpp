#include "qmultidevice.h"

#include <QApplication>
#include <QElapsedTimer>

#include <QtNetwork/QNetworkInterface>
#include <QtSerialPort/QSerialPortInfo>

QMultiDevice::QMultiDevice(QObject* parent) : QObject{parent} {}

QMultiDevice::~QMultiDevice()
{
    disconnect();
    close();
}

/*static*/ QStringList QMultiDevice::serialBaudrates()
{
    QStringList result;
    for (const auto& baudrate : QSerialPortInfo::standardBaudRates())
    {
        result.append(QString::number(baudrate));
    }
    if (result.indexOf("921600") == -1)
    {
        result.append("921600");
    }
    return result;
}

/*static*/ QPair<QStringList, int> QMultiDevice::getSerialPorts(
    const QString& current_portname, const QPair<uint16_t, uint16_t>& vid_pid)
{
    QStringList ports;
    QVector<int> found_by_vidpid;
    for (const auto& port : QSerialPortInfo::availablePorts())
    {
        ports.append(port.portName() + " (" + port.description() + ")");
        if ((port.vendorIdentifier() == vid_pid.first) &&
            (port.productIdentifier() == vid_pid.second))
        {
            found_by_vidpid.append(ports.count() - 1);
        }
    }

    int index = -1;
    int prev_index = ports.indexOf(current_portname);
    if ((prev_index != -1) && (found_by_vidpid.isEmpty() || found_by_vidpid.contains(prev_index)))
    {
        index = prev_index;
    }
    else if (!found_by_vidpid.isEmpty())
    {
        index = found_by_vidpid[0];
    }
    else if (!ports.isEmpty())
    {
        index = 0;
    }
    return qMakePair(ports, index);
}

QPair<uint16_t, uint16_t> QMultiDevice::getSerialVIDPID(const QString& port_name)
{
    QString name = port_name.split(' ')[0];
    for (const auto& port : QSerialPortInfo::availablePorts())
    {
        if (port.portName().compare(name, Qt::CaseInsensitive) == 0)
        {
            return qMakePair(port.vendorIdentifier(), port.productIdentifier());
        }
    }
    return qMakePair(0, 0);
}

/*static*/ QList<QNetworkAddressEntry> QMultiDevice::getNetworkAddresses()
{
    QList<QNetworkAddressEntry> addresses;
    for (const QNetworkInterface& interface : QNetworkInterface::allInterfaces())
    {
        if (!interface.flags().testFlag(QNetworkInterface::IsRunning))
            continue;

        for (const auto& address : interface.addressEntries())
            if (address.ip().protocol() == QAbstractSocket::IPv4Protocol)
                addresses.append(address);
    }
    return addresses;
}

/*static*/ QHostAddress QMultiDevice::getLocalAddress(const QHostAddress& remote_host)
{
    for (const auto& address : getNetworkAddresses())
    {
        uint32_t subnet_mask_value = address.netmask().toIPv4Address();
        uint32_t local_host_value = address.ip().toIPv4Address();
        uint32_t remote_host_value = remote_host.toIPv4Address();

        if ((local_host_value & subnet_mask_value) == (remote_host_value & subnet_mask_value))
        {
            return address.ip();
        }
    }
    return QHostAddress::Null;
}

/*static*/ QStringList QMultiDevice::getAvailableIPs()
{
    QStringList ips;
    for (const auto& address : getNetworkAddresses())
        ips.append(QHostAddress(address.ip().toIPv4Address()).toString());
    return ips;
}

bool QMultiDevice::isActive() const
{
    return m_serial || m_udp_socket || m_tcp_socket || m_tcp_server;
}

QMultiDevice::ConnectionType QMultiDevice::connectionType() const
{
    if (m_serial)
        return ConnectionType::Serial;
    else if (m_udp_socket)
        return ConnectionType::UDPSocket;
    else if (m_tcp_socket)
        return ConnectionType::TCPSocket;
    else if (m_tcp_server)
        return ConnectionType::TCPServer;
    else
        return ConnectionType::Unknonwn;
}

QPair<int, QString> QMultiDevice::lastError() const
{
    return qMakePair(m_last_error, m_last_error_string);
}

void QMultiDevice::clearError()
{
    m_last_error = 0;
    m_last_error_string.clear();
}

bool QMultiDevice::close()
{
    clearError();
    if (m_serial)
    {
        if (m_serial->isOpen())
        {
            m_serial->close();
            emit disconnected(ConnectionType::Serial);
            emit interfaceStatus(tr("Serial port closed"));
        }
        m_serial->deleteLater();
        m_serial = nullptr;
    }
    else if (m_udp_socket)
    {
        m_udp_socket->close();
        emit disconnected(ConnectionType::UDPSocket);
        emit interfaceStatus(tr("UDP socket unbinded"));

        m_udp_socket->deleteLater();
        m_udp_socket = nullptr;
    }
    else if (m_tcp_socket)
    {
        if (m_tcp_socket->isOpen())
        {
            m_tcp_socket->close();
            emit disconnected(ConnectionType::TCPSocket);
            emit interfaceStatus(tr("TCP socket closed"));
        }
        m_tcp_socket->deleteLater();
        m_tcp_socket = nullptr;
    }
    else if (m_tcp_server)
    {
        closePeer();
        m_tcp_server->close();
        emit disconnected(ConnectionType::TCPServer);
        emit interfaceStatus(tr("TCP server unbinded"));

        m_tcp_server->deleteLater();
        m_tcp_server = nullptr;
    }

    m_remote_address = IPPORT();

    return true;
}

bool QMultiDevice::closePeer(const QString& peer)
{
    if (m_tcp_server)
    {
        if (!peer.isEmpty())
        {
            IPPORT remote(peer);
            for (const auto& socket : m_tcp_server_sockets)
            {
                if (equalSocketAddress(*socket, remote))
                    socket->close();
                return true;
            }
            return false;
        }
        else
        {
            for (const auto& socket : m_tcp_server_sockets)
            {
                socket->close();
            }
            return true;
        }
    }
    return false;
}

bool QMultiDevice::openSerial(const QString& port, const uint32_t& baudrate, const char& parity)
{
    close();
    clearError();

    m_serial = new QSerialPort(this);
    m_serial->setPortName(port.split(' ')[0]);
    m_serial->setBaudRate(baudrate);
    m_serial->setParity(serialParityValues.value(std::toupper(parity), QSerialPort::NoParity));

    QObject::connect(
        m_serial, &QSerialPort::errorOccurred, this, &QMultiDevice::serialErrorOccurred);
    QObject::connect(m_serial, &QSerialPort::readyRead, this, &QMultiDevice::readyRead);

    if (m_serial->open(QIODevice::ReadWrite))
    {
        emit interfaceStatus(tr("Serial port %1 opened").arg(m_serial->portName()));
        emit connected(ConnectionType::Serial);
        return true;
    }
    close();
    return false;
}

QString QMultiDevice::serialPortName() const
{
    return m_serial ? m_serial->portName() : QString();
}

uint32_t QMultiDevice::serialBaudrate() const
{
    return m_serial ? m_serial->baudRate() : 0;
}

char QMultiDevice::serialParity() const
{
    if (m_serial)
    {
        for (auto& key : serialParityValues.keys())
        {
            if (m_serial->parity() == serialParityValues[key])
                return key;
        }
    }
    return '\0';
}

void QMultiDevice::serialErrorOccurred(const QSerialPort::SerialPortError& error)
{
    if (m_serial && (error != QSerialPort::SerialPortError::NoError))
    {
        m_last_error = error;
        m_last_error_string = m_serial->errorString();
        emit errorOccurred(m_last_error_string);
        if ((error == QSerialPort::SerialPortError::ResourceError) ||
            (error == QSerialPort::SerialPortError::PermissionError))
        {
            close();
        }
    }
}

bool QMultiDevice::bindUDPSocket(const QString& remote_host,
                                 const uint16_t& remote_port,
                                 const uint16_t& local_port)
{
    QHostAddress remote(remote_host);
    if (remote_host.compare("localhost", Qt::CaseInsensitive) == 0)
        remote = QHostAddress::LocalHost;
    return bindUDPSocket(remote, remote_port, local_port);
}

bool QMultiDevice::bindUDPSocket(const QHostAddress& remote_host,
                                 const uint16_t& remote_port,
                                 const uint16_t& local_port)
{
    close();
    m_udp_socket = new QUdpSocket(this);
    QObject::connect(
        m_udp_socket, &QUdpSocket::errorOccurred, this, &QMultiDevice::socketErrorOccurred);
    QObject::connect(m_udp_socket, &QUdpSocket::readyRead, this, &QMultiDevice::readyRead);

    if (m_udp_socket->bind(/*getLocalAddress(remote_host),*/ local_port))
    {
        m_remote_address.host = remote_host.toIPv4Address();
        m_remote_address.port = remote_port;
        emit interfaceStatus(tr("UDP socket binded to port %1").arg(localPort()));
        emit connected(ConnectionType::UDPSocket);
        return true;
    }
    close();
    return false;
}

bool QMultiDevice::openTCPSocket(const QString& remote_host, const uint16_t& remote_port)
{
    QHostAddress remote(remote_host);
    if (remote_host.compare("localhost", Qt::CaseInsensitive) == 0)
        remote = QHostAddress::LocalHost;
    return openTCPSocket(remote, remote_port);
}

bool QMultiDevice::openTCPSocket(const QHostAddress& remote_host, const uint16_t& remote_port)
{
    close();
    m_tcp_socket = new QTcpSocket(this);
    QObject::connect(
        m_tcp_socket, &QTcpSocket::errorOccurred, this, &QMultiDevice::socketErrorOccurred);
    QObject::connect(m_tcp_socket, &QUdpSocket::readyRead, this, &QMultiDevice::readyRead);

    m_tcp_socket->connectToHost(remote_host, remote_port);
    if (m_tcp_socket->waitForConnected(default_connect_timeout))
    {
        m_remote_address.host = remote_host.toIPv4Address();
        m_remote_address.port = remote_port;
        emit interfaceStatus(
            tr("TCP socket connected to %1:%2").arg(remoteHost().toString()).arg(remote_port));
        emit connected(ConnectionType::TCPSocket);
        return true;
    }
    close();
    return false;
}

bool QMultiDevice::bindTCPServer(const uint16_t& local_port)
{
    close();
    m_tcp_server = new QTcpServer(this);

    QObject::connect(
        m_tcp_server, &QTcpServer::acceptError, this, &QMultiDevice::socketErrorOccurred);
    QObject::connect(
        m_tcp_server, &QTcpServer::newConnection, this, &QMultiDevice::tcpServerNewConnection);

    if (m_tcp_server->listen(QHostAddress::Any, local_port))
    {
        m_remote_address = {};
        emit interfaceStatus(tr("TCP server binded to port %1").arg(localPort()));
        emit connected(ConnectionType::TCPServer);
        return true;
    }
    close();
    return false;
}

QList<QTcpSocket*> QMultiDevice::tcpServerPeers() const&
{
    return m_tcp_server_sockets;
}

QHostAddress QMultiDevice::remoteHost() const
{
    return (m_udp_socket || m_tcp_socket || m_tcp_server) ? QHostAddress(m_remote_address.host)
                                                          : QHostAddress::Null;
}

QHostAddress QMultiDevice::QMultiDevice::localHost() const
{
    if (m_udp_socket)
    {
        return QHostAddress(m_udp_socket->localAddress().toIPv4Address());
    }
    else if (m_tcp_socket)
    {
        return QHostAddress(m_tcp_socket->localAddress().toIPv4Address());
    }
    else if (m_tcp_server)
    {
        return QHostAddress(m_tcp_server->serverAddress().toIPv4Address());
    }
    return QHostAddress::Null;
}

uint16_t QMultiDevice::QMultiDevice::remotePort() const
{
    return (m_udp_socket || m_tcp_socket || m_tcp_server) ? m_remote_address.port : 0;
}

uint16_t QMultiDevice::QMultiDevice::localPort() const
{
    if (m_udp_socket)
    {
        return m_udp_socket->localPort();
    }
    else if (m_tcp_socket)
    {
        return m_tcp_socket->localPort();
    }
    else if (m_tcp_server)
    {
        return m_tcp_server->serverPort();
    }
    return QHostAddress::Null;
}

void QMultiDevice::socketErrorOccurred(const QAbstractSocket::SocketError& error)
{
    m_last_error = error;
    if (m_udp_socket)
    {
        m_last_error_string = m_udp_socket->errorString();
    }
    else if (m_tcp_socket)
    {
        m_last_error_string = m_tcp_socket->errorString();
    }
    else if (m_tcp_server)
    {
        m_last_error_string = m_tcp_server->errorString();
    }
    emit errorOccurred(m_last_error_string);
}

void QMultiDevice::tcpServerNewConnection()
{
    if (m_tcp_server)
    {
        auto socket = m_tcp_server->nextPendingConnection();
        if (socket)
        {
            m_tcp_server_sockets.append(socket);
            QObject::connect(socket, &QAbstractSocket::readyRead, this, &QMultiDevice::readyRead);
            QObject::connect(socket,
                             &QAbstractSocket::disconnected,
                             this,
                             &QMultiDevice::tcpServerCloseConnection);
            emit interfaceStatus(
                tr("TCP client %1:%2 connected")
                    .arg(QHostAddress(socket->peerAddress().toIPv4Address()).toString())
                    .arg(socket->peerPort()));
            emit peerConnected(
                QString("%1:%2")
                    .arg(QHostAddress(socket->peerAddress().toIPv4Address()).toString())
                    .arg(socket->peerPort()));
        }
    }
}

void QMultiDevice::tcpServerCloseConnection()
{
    auto socket = dynamic_cast<QTcpSocket*>(sender());
    if (socket)
    {
        m_tcp_server_sockets.removeAll(socket);
        socket->close();
        emit interfaceStatus(
            tr("TCP client %1:%2 disconnected")
                .arg(QHostAddress(socket->peerAddress().toIPv4Address()).toString())
                .arg(socket->peerPort()));
        emit peerDisconnected(
            QString("%1:%2")
                .arg(QHostAddress(socket->peerAddress().toIPv4Address()).toString())
                .arg(socket->peerPort()));
    }
}

uint32_t QMultiDevice::write(const QByteArray& data)
{
    uint32_t count = 0;
    clearError();
    if (m_serial)
    {
        count = m_serial->write(data);
    }
    else if (m_udp_socket)
    {
        count = m_udp_socket->writeDatagram(
            data, QHostAddress(m_remote_address.host), m_remote_address.port);
    }
    else if (m_tcp_socket)
    {
        count = m_tcp_socket->write(data);
    }
    else if (m_tcp_server)
    {
        if ((!m_remote_address.host || !m_remote_address.port) && !m_tcp_server_sockets.empty())
        {
            m_remote_address.host = m_tcp_server_sockets.last()->peerAddress().toIPv4Address();
            m_remote_address.port = m_tcp_server_sockets.last()->peerPort();
        }
        if (m_remote_address.host && m_remote_address.port)
        {
            bool socket_found = false;
            for (const auto& socket : m_tcp_server_sockets)
            {
                if (equalSocketAddress(*socket, m_remote_address))
                {
                    socket_found = true;
                    count = socket->write(data);
                    break;
                }
            }
            if (!socket_found)
            {
                m_last_error = -1;
                m_last_error_string = tr("Wrong destination address/port");
                emit errorOccurred(m_last_error_string);
            }
        }
    }
    else
    {
        m_last_error = -1;
        m_last_error_string = tr("No active connection");
        emit errorOccurred(m_last_error_string);
    }
    if (count)
    {
        emit dataTransmitted(data);
        if (m_serial)
            emit dataTransmittedTo(data, serialPortName());
        else if (m_udp_socket || m_tcp_socket || m_tcp_server)
            emit dataTransmittedTo(data,
                                   QString("%1:%2").arg(remoteHost().toString()).arg(remotePort()));
    }
    return count;
}

uint32_t QMultiDevice::writeTo(const QString& dst, const QByteArray& data)
{
    if (m_serial)
    {
        if (dst == serialPortName())
        {
            return write(data);
        }
        else
        {
            m_last_error_string = tr("Wrong destination port name");
        }
    }
    else if (m_udp_socket || m_tcp_server)
    {
        m_remote_address = IPPORT(dst);
        if (m_remote_address.valid())
        {
            return write(data);
        }
        else
        {
            m_last_error_string = tr("Wrong destination address");
        }
    }
    else if (m_tcp_socket)
    {
        if (m_remote_address == IPPORT(dst))
        {
            return write(data);
        }
        else
        {
            m_last_error_string = tr("Wrong destination address");
        }
    }
    else
    {
        m_last_error_string = tr("No active connection");
    }
    m_last_error = -1;
    emit errorOccurred(m_last_error_string);
    return 0;
}

uint32_t QMultiDevice::available()
{
    clearError();
    if (m_serial)
    {
        uint32_t count = m_serial->bytesAvailable();
        return count;
    }
    else if (m_udp_socket)
    {
        uint32_t count = m_udp_socket->pendingDatagramSize();
        return count;
    }
    else if (m_tcp_socket)
    {
        uint32_t count = m_tcp_socket->bytesAvailable();
        return count;
    }
    else if (m_tcp_server)
    {
        for (const auto& socket : m_tcp_server_sockets)
        {
            uint32_t count = socket->bytesAvailable();
            if (count)
            {
                m_remote_address.host = socket->peerAddress().toIPv4Address();
                m_remote_address.port = socket->peerPort();
                return count;
            }
        }
    }

    m_last_error = -1;
    m_last_error_string = tr("No active connection");
    emit errorOccurred(m_last_error_string);
    return 0;
}

uint32_t QMultiDevice::wait(const uint32_t& expected_bytes, const uint32_t& timeout_ms)
{
    auto available_bytes = available();
    QElapsedTimer timer;
    timer.start();
    if (expected_bytes > 0)
    {
        while (isActive() && !lastError().first && (available_bytes < expected_bytes) &&
               (static_cast<uint32_t>(timer.elapsed()) < timeout_ms))
        {
            QApplication::processEvents();
            available_bytes = available_bytes;
        }
        available_bytes = std::min<int32_t>(available_bytes, expected_bytes);
    }
    else
    {
        // Читаем данные, пока данные валятся
        auto previous_available_bytes = available_bytes;
        while (isActive() && !lastError().first && (timer.elapsed() < timeout_ms))
        {
            QApplication::processEvents();
            available_bytes = available_bytes;
            if (available_bytes != previous_available_bytes)
            {
                timer.restart();
                previous_available_bytes = available_bytes;
            }
        }
    }
    return available_bytes;
}

QByteArray QMultiDevice::readExpected(const uint32_t& expected_bytes)
{
    QByteArray data;
    clearError();
    if (m_serial)
    {
        data = expected_bytes ? m_serial->read(expected_bytes) : m_serial->readAll();
        if (m_last_error)
        {
            data.clear();
        }
    }
    else if (m_udp_socket)
    {
        uint32_t count = expected_bytes ? expected_bytes : available();
        if (count)
        {
            data.resize(count);
            QHostAddress remote_host;
            uint16_t remote_port;
            auto received_bytes =
                m_udp_socket->readDatagram(data.data(), count, &remote_host, &remote_port);
            if (received_bytes <= 0)
            {
                data.clear();
            }
            else
            {
                m_remote_address.host = remote_host.toIPv4Address();
                m_remote_address.port = remote_port;
            }
        }
    }
    else if (m_tcp_socket)
    {
        uint32_t count = expected_bytes ? expected_bytes : available();
        data = m_tcp_socket->read(count);
        if (m_last_error)
        {
            data.clear();
        }
    }
    else if (m_tcp_server)
    {
        uint32_t count = expected_bytes ? expected_bytes : available();
        if (count)
        {
            for (const auto& socket : m_tcp_server_sockets)
            {
                if (equalSocketAddress(*socket, m_remote_address))
                {
                    data = socket->read(count);
                    if (m_last_error)
                    {
                        data.clear();
                    }
                    break;
                }
            }
        }
    }
    else
    {
        m_last_error = -1;
        m_last_error_string = tr("No active connection");
        emit errorOccurred(m_last_error_string);
    }
    if (!data.isEmpty())
    {
        emit dataReceived(data);
        if (m_serial)
        {
            emit dataReceivedFrom(data, serialPortName());
        }
        else if (m_udp_socket || m_tcp_socket || m_tcp_server)
        {
            emit dataReceivedFrom(data,
                                  QString("%1:%2").arg(remoteHost().toString()).arg(remotePort()));
        }
    }
    return data;
}

QByteArray QMultiDevice::read(const uint32_t& expected_bytes, const uint32_t& timeout_ms)
{
    if (timeout_ms)
    {
        auto available_bytes = wait(expected_bytes, timeout_ms);
        return readExpected(available_bytes);
    }
    else
    {
        return readExpected(expected_bytes);
    }
}

QByteArray QMultiDevice::transceive(const QByteArray& data,
                                    const uint32_t& expected_bytes,
                                    const uint32_t& timeout_ms)
{
    if (write(data))
        return read(expected_bytes, timeout_ms ? timeout_ms : default_receive_timeout);
    return {};
}

/*static*/ bool QMultiDevice::equalSocketAddress(const QTcpSocket& l_socket,
                                                 const IPPORT& r_address)
{
    return (l_socket.peerAddress().toIPv4Address() == r_address.host) &&
           (l_socket.peerPort() == r_address.port);
}
