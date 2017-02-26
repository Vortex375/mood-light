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

#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QSerialPort>

#include <stdint.h>

#define BAUD_RATE 19200

class Serial : public QObject
{
Q_OBJECT

public:
    Serial(QObject *parent);
    ~Serial();

    void writePixelValue(uint32_t pixel);


private:
    QSerialPort port;

    constexpr static uint8_t syncHeader[] = {0x00, 0x55, 0xAA, 0xFF};
};

#endif // SERIAL_H
