#pragma once

#include <qmultidevice.h>

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>

class QUDPSocketWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QUDPSocketWidget(QWidget* parent = 0) : QWidget(parent)
    {
        m_remote_host_label = new QLabel(tr("Remote host"), this);

        m_remote_host_combobox = new QComboBox(this);
        m_remote_host_combobox->setEditable(true);
        m_remote_host_combobox->setCurrentText("localhost");
        connect(m_remote_host_combobox,
                &QComboBox::currentTextChanged,
                this,
                &QUDPSocketWidget::remoteHostChanged);

        m_remote_port_label = new QLabel(tr("Remote port"), this);

        m_remote_port_spinbox = new QSpinBox(this);
        m_remote_port_spinbox->setRange(0, 65535);
        connect(m_remote_port_spinbox,
                static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this,
                &QUDPSocketWidget::remotePortChanged);

        m_local_port_label = new QLabel(tr("Local port"), this);

        m_local_port_spinbox = new QSpinBox(this);
        m_local_port_spinbox->setRange(0, 65535);

        m_open_button = new QPushButton(tr("Socket bind"), this);
        connect(m_open_button, &QPushButton::clicked, this, &QUDPSocketWidget::openClicked);

        auto layout = new QGridLayout(this);
        layout->addWidget(m_remote_host_label, 0, 0);
        layout->addWidget(m_remote_host_combobox, 1, 0);
        layout->addWidget(m_remote_port_label, 0, 1);
        layout->addWidget(m_remote_port_spinbox, 1, 1);
        layout->addWidget(m_local_port_label, 0, 2);
        layout->addWidget(m_local_port_spinbox, 1, 2);
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 3);
        layout->addWidget(m_open_button, 1, 4);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 1);
        layout->setColumnStretch(2, 1);
        layout->setColumnStretch(3, 2);
        layout->setColumnStretch(4, 1);

        setLayout(layout);
    }

    QPushButton* openButton() const
    {
        return m_open_button;
    }

    QString remoteHost() const
    {
        return m_remote_host_combobox->currentText();
    }

    uint16_t remotePort() const
    {
        return m_remote_port_spinbox->value();
    }

    uint16_t localPort() const
    {
        return m_local_port_spinbox->value();
    }

    void setMultidevice(QMultiDevice* device)
    {
        if (m_device != device)
        {
            if (m_device)
            {
                disconnect(
                    m_device, &QMultiDevice::connected, this, &QUDPSocketWidget::deviceOpened);
                disconnect(
                    m_device, &QMultiDevice::disconnected, this, &QUDPSocketWidget::deviceClosed);
            }
            m_device = device;
            if (device)
            {
                connect(m_device, &QMultiDevice::connected, this, &QUDPSocketWidget::deviceOpened);
                connect(
                    m_device, &QMultiDevice::disconnected, this, &QUDPSocketWidget::deviceClosed);
            }
        }
    }

    void saveSettings(QSettings& settings)
    {
        settings.beginGroup("udp_socket_settings");
        settings.setValue("remote_host", m_remote_host_combobox->currentText());
        settings.setValue("remote_port", m_remote_port_spinbox->value());
        settings.setValue("local_port", m_local_port_spinbox->value());
        settings.endGroup();
    }
    void loadSettings(QSettings& settings)
    {
        settings.beginGroup("udp_socket_settings");
        m_remote_host_combobox->setCurrentText(
            settings.value("remote_host", m_remote_host_combobox->currentText()).toString());
        m_remote_port_spinbox->setValue(
            settings.value("remote_port", m_remote_port_spinbox->value()).toInt());
        m_remote_port_spinbox->setValue(
            settings.value("local_port", m_local_port_spinbox->value()).toInt());
        settings.endGroup();
    }

public slots:
    void refresh()
    {
        if (m_remote_host_combobox->isEnabled())
        {
            QStringList hosts{"localhost"};
            for (const auto host : QMultiDevice::getNetworkAddresses())
            {
                hosts.append(host.ip().toString());
                hosts.append(host.broadcast().toString());
            }
            hosts.append("255.255.255.255");
            QString current_host = m_remote_host_combobox->currentText();
            m_remote_host_combobox->clear();
            m_remote_host_combobox->addItems(hosts);
            m_remote_host_combobox->setCurrentText(current_host);
        }
    }

    void open()
    {
        if (m_device)
        {
            m_device->bindUDPSocket(remoteHost(), remotePort(), localPort());
        }
    }

    void close()
    {
        if (m_device)
        {
            m_device->close();
        }
    }

private slots:
    void openClicked()
    {
        if (m_device)
        {
            if (m_device->isActive() &&
                (m_device->connectionType() == QMultiDevice::ConnectionType::UDPSocket))
            {
                close();
            }
            else
            {
                open();
            }
        }
    }

    void deviceOpened(const QMultiDevice::ConnectionType& type)
    {
        if (type == QMultiDevice::ConnectionType::UDPSocket)
        {
            m_local_port_spinbox->setEnabled(false);
            m_open_button->setText(tr("Socket unbind"));
            if (m_device)
            {
                m_local_port_spinbox->setValue(m_device->localPort());
            }
        }
    }

    void deviceClosed(const QMultiDevice::ConnectionType& type)
    {
        if (type == QMultiDevice::ConnectionType::UDPSocket)
        {
            m_local_port_spinbox->setEnabled(true);
            m_open_button->setText(tr("Socket bind"));
        }
        refresh();
    }

    void remoteHostChanged(const QString&)
    {
        if (m_device)
        {
            m_device->setDestination(QString("%1:%2")
                                         .arg(m_remote_host_combobox->currentText())
                                         .arg(m_remote_port_spinbox->value()));
        }
    }
    void remotePortChanged(const int&)
    {
        if (m_device)
        {
            m_device->setDestination(QString("%1:%2")
                                         .arg(m_remote_host_combobox->currentText())
                                         .arg(m_remote_port_spinbox->value()));
        }
    }

private:
    QLabel* m_remote_host_label;
    QComboBox* m_remote_host_combobox;

    QLabel* m_remote_port_label;
    QSpinBox* m_remote_port_spinbox;

    QLabel* m_local_port_label;
    QSpinBox* m_local_port_spinbox;

    QPushButton* m_open_button;

    QMultiDevice* m_device = nullptr;
};
