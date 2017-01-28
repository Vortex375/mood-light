/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright 2017  <copyright holder> <email>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "serial.h"

#include <QDebug>
#include <QtSerialPort/QSerialPortInfo>

Serial::Serial(QObject *parent) :
    port(this)
{
    qDebug() << "available serial ports:";
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        qDebug() << "Opening serial port:" << info.portName();
        port.setPort(info);
        break;
    }

    port.setBaudRate(BAUD_RATE);
    port.open(QIODevice::WriteOnly);
}

Serial::~Serial()
{
}

void Serial::writePixelValue(uint32_t pixel)
{
    if (port.bytesToWrite() > 0) {
        // do not queue
        return;
    }
    port.write((const char *) &pixel, sizeof(pixel));
}
