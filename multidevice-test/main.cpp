#include <qmultidevice.h>

#include <QApplication>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QWidget>

#include <qserialport_widget.hpp>
#include <qtcpclient_widget.hpp>
#include <qtcpserver_widget.hpp>
#include <qudpsocket_widget.hpp>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QWidget w;
    w.setWindowTitle(APP_NAME " v" APP_VERSION);

    QVBoxLayout* main_layout = new QVBoxLayout(&w);
    w.setLayout(main_layout);

    QTabWidget* connection_tabs = new QTabWidget(&w);
    main_layout->addWidget(connection_tabs);

    QPlainTextEdit* log = new QPlainTextEdit(&w);
    QFont f("Monospace");
    f.setStyleHint(QFont::Monospace);
    log->setFont(f);
    main_layout->addWidget(log);

    auto addLog = [log](const QString& text, const QColor& color, const QString& prefix = {})
    {
        QString html = QString("<span style='color: %1; white-space: pre-wrap;'>%2%3</span>")
                           .arg(color.name(), prefix.toHtmlEscaped(), text.toHtmlEscaped());
        log->appendHtml(html);
        log->moveCursor(QTextCursor::End);
    };

    QMultiDevice* device = new QMultiDevice(&w);
    QObject::connect(device,
                     &QMultiDevice::interfaceStatus,
                     &w,
                     [addLog](const QString& t) { addLog(t, Qt::darkYellow); });
    QObject::connect(device,
                     &QMultiDevice::errorOccurred,
                     &w,
                     [&w, addLog](const QString& t)
                     { addLog(w.tr("Error: %1").arg(t), Qt::darkRed); });
    QObject::connect(device,
                     &QMultiDevice::dataReceivedFrom,
                     &w,
                     [&w, addLog](const QByteArray& data, const QString& from)
                     {
                         addLog(w.tr("Received from %1 %2 bytes").arg(from).arg(data.size()),
                                Qt::green);
                         addLog(data.toHex(' ').toUpper(), Qt::darkGreen, ">> ");
                     });
    QObject::connect(device,
                     &QMultiDevice::dataTransmittedTo,
                     &w,
                     [&w, addLog](const QByteArray& data, const QString& to)
                     {
                         addLog(w.tr("Transmitted to %1 %2 bytes").arg(to).arg(data.size()),
                                Qt::blue);
                         addLog(data.toHex(' ').toUpper(), Qt::darkBlue, "<< ");
                     });

    // !!! Be sure to connect readyRead !!!
    QObject::connect(device, &QMultiDevice::readyRead, device, [&device] { device->read(); });

    // Serial port connection
    QSerialPortWidget* serial_port_tab = new QSerialPortWidget(&w);
    connection_tabs->addTab(serial_port_tab, w.tr("Serial port"));
    serial_port_tab->setMultidevice(device);

    // TCP Client connection
    QTCPClientWidget* tcp_client_tab = new QTCPClientWidget(&w);
    connection_tabs->addTab(tcp_client_tab, w.tr("TCP Client"));
    tcp_client_tab->setMultidevice(device);

    // TCP Server connection
    QTCPServerWidget* tcp_server_tab = new QTCPServerWidget(&w);
    connection_tabs->addTab(tcp_server_tab, w.tr("TCP Server"));
    tcp_server_tab->setMultidevice(device);

    // UDP socket connection
    QUDPSocketWidget* udp_socket_tab = new QUDPSocketWidget(&w);
    connection_tabs->addTab(udp_socket_tab, w.tr("UDP Socket"));
    udp_socket_tab->setMultidevice(device);

    serial_port_tab->refresh();
    udp_socket_tab->refresh();

    // Sending message
    QHBoxLayout* message_layout = new QHBoxLayout(&w);
    main_layout->addLayout(message_layout);

    message_layout->addWidget(new QLabel(w.tr("Message"), &w));
    QLineEdit* message_edit = new QLineEdit(&w);
    message_edit->setPlaceholderText(w.tr("<Type here message to send>"));
    message_layout->addWidget(message_edit);
    QPushButton* send_button = new QPushButton(w.tr("Send"), &w);
    message_layout->addWidget(send_button);

    auto send_message = [message_edit, device]
    {
        device->write(message_edit->text().toUtf8());
        message_edit->clear();
    };

    QObject::connect(send_button, &QPushButton::clicked, &w, send_message);
    QObject::connect(message_edit, &QLineEdit::returnPressed, &w, send_message);

    connection_tabs->setCurrentIndex(3);

    w.show();
    return a.exec();
}
