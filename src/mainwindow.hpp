#pragma once

#include <qmultidevice.h>

#include <QCloseEvent>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTableWidget>
#include <QWidget>

#include <qserialport_widget.hpp>
#include <qtcpclient_widget.hpp>
#include <qtcpserver_widget.hpp>
#include <qudpsocket_widget.hpp>

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr) : QWidget(parent)
    {
        setWindowTitle(APP_NAME " v" APP_VERSION);

        m_main_layout = new QVBoxLayout(this);
        setLayout(m_main_layout);

        m_connection_tabs = new QTabWidget(this);
        m_main_layout->addWidget(m_connection_tabs);

        m_log = new QPlainTextEdit(this);
        QFont f("Monospace");
        f.setStyleHint(QFont::Monospace);
        m_log->setFont(f);
        m_main_layout->addWidget(m_log);

        m_device = new QMultiDevice(this);
        QObject::connect(m_device,
                         &QMultiDevice::interfaceStatus,
                         this,
                         [this](const QString& t) { addLog(t, Qt::darkYellow); });
        QObject::connect(m_device,
                         &QMultiDevice::errorOccurred,
                         this,
                         [this](const QString& t) { addLog(tr("Error: %1").arg(t), Qt::darkRed); });
        QObject::connect(
            m_device, &QMultiDevice::dataReceivedFrom, this, &MainWindow::deviceReceiveFrom);
        QObject::connect(
            m_device, &QMultiDevice::dataTransmittedTo, this, &MainWindow::deviceSendTo);

        // !!! Be sure to connect readyRead !!!
        QObject::connect(m_device, &QMultiDevice::readyRead, this, [this] { m_device->read(); });

        // Serial port connection
        m_serial_port_tab = new QSerialPortWidget(this);
        m_connection_tabs->addTab(m_serial_port_tab, tr("Serial port"));
        m_serial_port_tab->setMultidevice(m_device);

        // TCP Client connection
        m_tcp_client_tab = new QTCPClientWidget(this);
        m_connection_tabs->addTab(m_tcp_client_tab, tr("TCP Client"));
        m_tcp_client_tab->setMultidevice(m_device);

        // TCP Server connection
        m_tcp_server_tab = new QTCPServerWidget(this);
        m_connection_tabs->addTab(m_tcp_server_tab, tr("TCP Server"));
        m_tcp_server_tab->setMultidevice(m_device);

        // UDP socket connection
        m_udp_socket_tab = new QUDPSocketWidget(this);
        m_connection_tabs->addTab(m_udp_socket_tab, tr("UDP Socket"));
        m_udp_socket_tab->setMultidevice(m_device);

        m_serial_port_tab->refresh();
        m_udp_socket_tab->refresh();

        // Sending message
        m_message_layout = new QHBoxLayout(this);
        m_main_layout->addLayout(m_message_layout);

        m_message_layout->addWidget(new QLabel(tr("Message"), this));
        m_message_edit = new QLineEdit(this);
        m_message_edit->setPlaceholderText(tr("<Type here message to send>"));
        m_message_layout->addWidget(m_message_edit);
        m_send_button = new QPushButton(tr("Send"), this);
        m_message_layout->addWidget(m_send_button);

        connect(m_send_button, &QPushButton::clicked, this, &MainWindow::sendMessage);
        connect(m_message_edit, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);

        loadSettings();
    }

    ~MainWindow() {}

    void addLog(const QString& text, const QColor& color, const QString& prefix = {})
    {
        QString html = QString("<span style='color: %1; white-space: pre-wrap;'>%2%3</span>")
                           .arg(color.name(), prefix.toHtmlEscaped(), text.toHtmlEscaped());
        m_log->appendHtml(html);
        m_log->moveCursor(QTextCursor::End);
    }

    void sendMessage()
    {
        m_device->write(m_message_edit->text().toUtf8());
        m_message_edit->clear();
    }

    void deviceReceiveFrom(const QByteArray& data, const QString& from)
    {
        addLog(tr("Received from %1 %2 bytes").arg(from).arg(data.size()), Qt::green);
        addLog(data.toHex(' ').toUpper(), Qt::darkGreen, ">> ");
    }

    void deviceSendTo(const QByteArray& data, const QString& to)
    {
        addLog(tr("Transmitted to %1 %2 bytes").arg(to).arg(data.size()), Qt::blue);
        addLog(data.toHex(' ').toUpper(), Qt::darkBlue, "<< ");
    }

    void saveSettings()
    {
        QSettings settings("settings.ini", QSettings::Format::IniFormat);
        settings.beginGroup("gui");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("current_tab", m_connection_tabs->currentIndex());
        settings.endGroup();
        m_serial_port_tab->saveSettings(settings);
        m_tcp_client_tab->saveSettings(settings);
        m_tcp_server_tab->saveSettings(settings);
        m_udp_socket_tab->saveSettings(settings);
        settings.sync();
    }

    void loadSettings()
    {
        QSettings settings("settings.ini", QSettings::Format::IniFormat);
        settings.beginGroup("gui");
        restoreGeometry(settings.value("geometry").toByteArray());
        m_connection_tabs->setCurrentIndex(settings.value("current_tab", 0).toInt());
        settings.endGroup();
        m_serial_port_tab->loadSettings(settings);
        m_tcp_client_tab->loadSettings(settings);
        m_tcp_server_tab->loadSettings(settings);
        m_udp_socket_tab->loadSettings(settings);
    }

protected:
    void closeEvent(QCloseEvent* event) override
    {
        saveSettings();
        QWidget::closeEvent(event);
    }

private:
    QVBoxLayout* m_main_layout;
    QTabWidget* m_connection_tabs;
    QPlainTextEdit* m_log;
    QMultiDevice* m_device;
    QSerialPortWidget* m_serial_port_tab;
    QTCPClientWidget* m_tcp_client_tab;
    QTCPServerWidget* m_tcp_server_tab;
    QUDPSocketWidget* m_udp_socket_tab;
    QHBoxLayout* m_message_layout;
    QLineEdit* m_message_edit;
    QPushButton* m_send_button;
};
