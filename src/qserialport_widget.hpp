#pragma once

#include <qmultidevice.h>

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>

// #include <QtSerialPort/QSerialPortInfo>

class QSerialPortWidget : public QWidget
{
    Q_OBJECT
public:
    static inline const char parity_char[] = {'N', 'E', 'O', 'S', 'M'};

    explicit QSerialPortWidget(QWidget* parent = 0) : QWidget(parent)
    {
        m_serial_port_label = new QLabel(tr("Serial port name"), this);
        m_serial_port_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_serial_port_combobox = new QComboBox(this);
        connect(m_serial_port_combobox,
                &QComboBox::currentTextChanged,
                this,
                &QSerialPortWidget::comboBoxSerialPortChanged);

        m_refresh_ports_button = new QToolButton(this);
        m_refresh_ports_button->setText(QString(QChar(0x27F3)));
        connect(m_refresh_ports_button, &QToolButton::clicked, this, &QSerialPortWidget::refresh);

        m_vidpid_label = new QLabel(tr("VID&PID"), this);
        m_vidpid_edit = new QLineEdit(this);
        m_vidpid_edit->setEnabled(false);

        m_baudrate_label = new QLabel(tr("Baudrate"), this);
        m_baudrate_combobox = new QComboBox(this);
        m_baudrate_combobox->setEditable(true);
        m_baudrate_combobox->addItems(QMultiDevice::serialBaudrates());
        m_baudrate_combobox->setCurrentIndex(m_baudrate_combobox->count() - 1);

        m_parity_label = new QLabel(tr("Parity"), this);
        m_parity_combobox = new QComboBox(this);
        m_parity_combobox->addItems({tr("None"), tr("Even"), tr("Odd"), tr("Space"), tr("Mark")});
        m_parity_combobox->setCurrentIndex(0);

        m_open_button = new QPushButton(tr("Serial open"), this);
        connect(m_open_button, &QPushButton::clicked, this, &QSerialPortWidget::openClicked);

        auto layout = new QGridLayout(this);
        layout->addWidget(m_serial_port_label, 0, 0);
        layout->addWidget(m_refresh_ports_button, 0, 1);
        layout->addWidget(m_serial_port_combobox, 1, 0, 1, 2);
        layout->addWidget(m_vidpid_label, 0, 2);
        layout->addWidget(m_vidpid_edit, 1, 2);
        layout->addWidget(m_baudrate_label, 0, 3);
        layout->addWidget(m_baudrate_combobox, 1, 3);
        layout->addWidget(m_parity_label, 0, 4);
        layout->addWidget(m_parity_combobox, 1, 4);
        layout->addWidget(m_open_button, 1, 5);

        layout->setColumnStretch(0, 2);
        layout->setColumnStretch(1, 1);
        layout->setColumnStretch(2, 1);
        layout->setColumnStretch(3, 1);
        layout->setColumnStretch(4, 1);
        layout->setColumnStretch(5, 1);

        setLayout(layout);
    }

    void setExpectedVidPid(const QPair<uint16_t, uint16_t> id = {})
    {
        m_expected_id = id;
    }
    QPair<uint16_t, uint16_t> expectedVidPid() const
    {
        return m_expected_id;
    }
    QPair<uint16_t, uint16_t> selectedVidPid() const
    {
        return m_selected_id;
    }
    bool matchesVidPid() const
    {
        return m_expected_id == m_selected_id;
    }
    QPushButton* openButton() const
    {
        return m_open_button;
    }
    QString portName() const
    {
        return m_serial_port_combobox->currentText();
    }
    uint32_t baudrate() const
    {
        return m_baudrate_combobox->currentText().toInt();
    }
    char parity() const
    {
        return parity_char[m_parity_combobox->currentIndex() % sizeof(parity_char)];
    }
    void setMultidevice(QMultiDevice* device)
    {
        if (m_device != device)
        {
            if (m_device)
            {
                disconnect(
                    m_device, &QMultiDevice::connected, this, &QSerialPortWidget::deviceOpened);
                disconnect(
                    m_device, &QMultiDevice::disconnected, this, &QSerialPortWidget::deviceClosed);
            }
            m_device = device;
            if (device)
            {
                connect(m_device, &QMultiDevice::connected, this, &QSerialPortWidget::deviceOpened);
                connect(
                    m_device, &QMultiDevice::disconnected, this, &QSerialPortWidget::deviceClosed);
            }
        }
    }

public slots:
    void refresh()
    {
        if (m_serial_port_combobox->isEnabled())
        {
            // Receive StringList and index
            auto availabel_ports =
                QMultiDevice::getSerialPorts(m_serial_port_combobox->currentText(), m_expected_id);
            m_serial_port_combobox->clear();
            m_serial_port_combobox->addItems(availabel_ports.first);
            m_serial_port_combobox->setCurrentIndex(availabel_ports.second);
        }
    }

    void open()
    {
        if (m_device)
        {
            m_device->openSerial(portName(), baudrate(), parity());
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
                (m_device->connectionType() == QMultiDevice::ConnectionType::Serial))
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
        if (type == QMultiDevice::ConnectionType::Serial)
        {
            m_serial_port_combobox->setEnabled(false);
            m_baudrate_combobox->setEnabled(false);
            m_parity_combobox->setEnabled(false);
            m_open_button->setText(tr("Serial close"));
        }
    }

    void deviceClosed(const QMultiDevice::ConnectionType& type)
    {
        if (type == QMultiDevice::ConnectionType::Serial)
        {
            m_serial_port_combobox->setEnabled(true);
            m_baudrate_combobox->setEnabled(true);
            m_parity_combobox->setEnabled(true);
            m_open_button->setText(tr("Serial open"));
        }
        refresh();
    }

    void comboBoxSerialPortChanged(const QString& port_name)
    {
        m_selected_id = QMultiDevice::getSerialVIDPID(port_name);
        if (m_selected_id.first && m_selected_id.second)
        {
            m_vidpid_edit->setText(
                QString("%1&%2")
                    .arg(QString::number(m_selected_id.first, 16).toUpper().rightJustified(4, '0'))
                    .arg(QString::number(m_selected_id.second, 16)
                             .toUpper()
                             .rightJustified(4, '0')));
        }
        else
        {
            m_vidpid_edit->clear();
        }
    }

private:
    QLabel* m_serial_port_label;
    QComboBox* m_serial_port_combobox;
    QToolButton* m_refresh_ports_button;

    QLabel* m_vidpid_label;
    QLineEdit* m_vidpid_edit;

    QLabel* m_baudrate_label;
    QComboBox* m_baudrate_combobox;

    QLabel* m_parity_label;
    QComboBox* m_parity_combobox;

    QPushButton* m_open_button;

    QPair<uint16_t, uint16_t> m_expected_id = {};
    QPair<uint16_t, uint16_t> m_selected_id = {};

    QMultiDevice* m_device = nullptr;
};
