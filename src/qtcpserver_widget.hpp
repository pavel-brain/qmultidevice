#pragma once

#include <qmultidevice.h>

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSpinBox>

class QTCPServerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QTCPServerWidget(QWidget* parent = 0) : QWidget(parent)
    {
        m_local_port_label = new QLabel(tr("Local port"), this);
        m_local_port_spinbox = new QSpinBox(this);
        m_local_port_spinbox->setRange(0, 65535);

        m_peers_label = new QLabel(tr("Peers"), this);
        m_peers_combobox = new QComboBox(this);
        m_peers_combobox->setEnabled(false);
        connect(
            m_peers_combobox, &QComboBox::currentTextChanged, this, &QTCPServerWidget::peerChanged);

        m_peer_detach_button = new QPushButton(tr("Detach"), this);
        m_peer_detach_button->setEnabled(false);
        connect(
            m_peer_detach_button, &QPushButton::clicked, this, &QTCPServerWidget::detachClicked);

        m_open_button = new QPushButton(tr("Server bind"), this);
        connect(m_open_button, &QPushButton::clicked, this, &QTCPServerWidget::openClicked);

        auto layout = new QGridLayout(this);
        layout->addWidget(m_local_port_label, 0, 0);
        layout->addWidget(m_local_port_spinbox, 1, 0);
        layout->addWidget(m_peers_label, 0, 1);
        layout->addWidget(m_peers_combobox, 1, 1);

        layout->addWidget(m_peer_detach_button, 1, 2);
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 3);
        layout->addWidget(m_open_button, 1, 4);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 2);
        layout->setColumnStretch(2, 1);
        layout->setColumnStretch(3, 1);
        layout->setColumnStretch(4, 1);

        setLayout(layout);
    }

    QPushButton* openButton() const
    {
        return m_open_button;
    }

    uint16_t localPort() const
    {
        return m_local_port_spinbox->value();
    }

    QList<QPair<QString, uint16_t>> peers() const
    {
        QList<QPair<QString, uint16_t>> result;
        for (int index = 0; index < m_peers_combobox->count(); index++)
        {
            QMultiDevice::IPPORT peer(m_peers_combobox->itemText(index));
            result.append(qMakePair(QHostAddress(peer.host).toString(), peer.port));
        }
        return result;
    }

    void setMultidevice(QMultiDevice* device)
    {
        if (m_device != device)
        {
            if (m_device)
            {
                disconnect(
                    m_device, &QMultiDevice::connected, this, &QTCPServerWidget::deviceOpened);
                disconnect(
                    m_device, &QMultiDevice::disconnected, this, &QTCPServerWidget::deviceClosed);
                disconnect(m_device,
                           &QMultiDevice::peerConnected,
                           this,
                           &QTCPServerWidget::devicePeerAttached);
                disconnect(m_device,
                           &QMultiDevice::peerDisconnected,
                           this,
                           &QTCPServerWidget::devicePeerDetached);
            }
            m_device = device;
            if (device)
            {
                connect(m_device, &QMultiDevice::connected, this, &QTCPServerWidget::deviceOpened);
                connect(
                    m_device, &QMultiDevice::disconnected, this, &QTCPServerWidget::deviceClosed);
                connect(m_device,
                        &QMultiDevice::peerConnected,
                        this,
                        &QTCPServerWidget::devicePeerAttached);
                connect(m_device,
                        &QMultiDevice::peerDisconnected,
                        this,
                        &QTCPServerWidget::devicePeerDetached);
            }
        }
    }

public slots:
    void open()
    {
        if (m_device)
        {
            m_device->bindTCPServer(localPort());
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
                (m_device->connectionType() == QMultiDevice::ConnectionType::TCPServer))
            {
                close();
            }
            else
            {
                open();
            }
        }
    }

    void detachClicked()
    {
        if (m_device)
        {
            m_device->closePeer(m_peers_combobox->currentText());
        }
    }

    void deviceOpened(const QMultiDevice::ConnectionType& type)
    {
        if (type == QMultiDevice::ConnectionType::TCPServer)
        {
            m_local_port_spinbox->setEnabled(false);
            m_peers_combobox->setEnabled(true);
            m_peer_detach_button->setEnabled(true);
            m_open_button->setText(tr("Server unbind"));
            if (m_device)
            {
                m_local_port_spinbox->setValue(m_device->localPort());
            }
        }
    }

    void deviceClosed(const QMultiDevice::ConnectionType& type)
    {
        if (type == QMultiDevice::ConnectionType::TCPServer)
        {
            m_local_port_spinbox->setEnabled(true);
            m_peers_combobox->clear();
            m_peers_combobox->setEnabled(false);
            m_peer_detach_button->setEnabled(false);
            m_open_button->setText(tr("Server bind"));
        }
    }

    void devicePeerAttached(const QString& peer)
    {
        if (m_device && (m_device->connectionType() == QMultiDevice::ConnectionType::TCPServer))
        {
            m_peers_combobox->addItem(peer);
        }
    }

    void devicePeerDetached(const QString& peer)
    {
        if (m_device && (m_device->connectionType() == QMultiDevice::ConnectionType::TCPServer))
        {
            m_peers_combobox->removeItem(m_peers_combobox->findText(peer));
        }
    }

    void peerChanged(const QString& text)
    {
        if (m_device)
        {
            m_device->setDestination(text);
        }
    }

private:
    QLabel* m_local_port_label;
    QSpinBox* m_local_port_spinbox;

    QLabel* m_peers_label;
    QComboBox* m_peers_combobox;

    QPushButton* m_peer_detach_button;

    QPushButton* m_open_button;

    QMultiDevice* m_device = nullptr;
};
