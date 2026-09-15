#pragma once

#include <qmultidevice.h>

#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

class QTCPClientWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QTCPClientWidget(QWidget* parent = 0) : QWidget(parent)
    {
        m_remote_host_label = new QLabel(tr("Remote host"), this);
        m_remote_host_edit = new QLineEdit("localhost", this);

        m_remote_port_label = new QLabel(tr("Remote port"), this);
        m_remote_port_spinbox = new QSpinBox(this);
        m_remote_port_spinbox->setRange(0, 65535);

        m_local_port_label = new QLabel(tr("Local port"), this);
        m_local_port_edit = new QLineEdit(this);
        m_local_port_edit->setEnabled(false);

        m_open_button = new QPushButton(tr("Client open"), this);
        connect(m_open_button, &QPushButton::clicked, this, &QTCPClientWidget::openClicked);

        auto layout = new QGridLayout(this);
        layout->addWidget(m_remote_host_label, 0, 0);
        layout->addWidget(m_remote_host_edit, 1, 0);
        layout->addWidget(m_remote_port_label, 0, 1);
        layout->addWidget(m_remote_port_spinbox, 1, 1);
        layout->addWidget(m_local_port_label, 0, 2);
        layout->addWidget(m_local_port_edit, 1, 2);
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
        return m_remote_host_edit->text();
    }
    uint16_t remotePort() const
    {
        return m_remote_port_spinbox->value();
    }
    void setLocalPort(const uint16_t& port)
    {
        return m_local_port_edit->setText(QString::number(port));
    }
    uint16_t localPort() const
    {
        return m_local_port_edit->text().toInt();
    }

    void setMultidevice(QMultiDevice* device)
    {
        if (m_device != device)
        {
            if (m_device)
            {
                disconnect(
                    m_device, &QMultiDevice::connected, this, &QTCPClientWidget::deviceOpened);
                disconnect(
                    m_device, &QMultiDevice::disconnected, this, &QTCPClientWidget::deviceClosed);
            }
            m_device = device;
            if (device)
            {
                connect(m_device, &QMultiDevice::connected, this, &QTCPClientWidget::deviceOpened);
                connect(
                    m_device, &QMultiDevice::disconnected, this, &QTCPClientWidget::deviceClosed);
            }
        }
    }

public slots:
    void open()
    {
        if (m_device)
        {
            m_device->openTCPSocket(remoteHost(), remotePort());
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
                (m_device->connectionType() == QMultiDevice::ConnectionType::TCPSocket))
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
        if (type == QMultiDevice::ConnectionType::TCPSocket)
        {
            m_remote_host_edit->setEnabled(false);
            m_remote_port_spinbox->setEnabled(false);
            m_open_button->setText(tr("Client close"));
            if (m_device)
            {
                m_local_port_edit->setText(QString::number(m_device->localPort()));
            }
        }
    }

    void deviceClosed(const QMultiDevice::ConnectionType& type)
    {
        if (type == QMultiDevice::ConnectionType::TCPSocket)
        {
            m_remote_host_edit->setEnabled(true);
            m_remote_port_spinbox->setEnabled(true);
            m_local_port_edit->clear();
            m_open_button->setText(tr("Client open"));
            m_local_port_edit->clear();
        }
    }

private:
    QLabel* m_remote_host_label;
    QLineEdit* m_remote_host_edit;

    QLabel* m_remote_port_label;
    QSpinBox* m_remote_port_spinbox;

    QLabel* m_local_port_label;
    QLineEdit* m_local_port_edit;

    QPushButton* m_open_button;

    QMultiDevice* m_device = nullptr;
};
