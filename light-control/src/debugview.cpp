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

#include "debugview.h"

#include <QLabel>
#include <QGridLayout>

#include <stdint.h>

DebugView::DebugView(Serial * serial) :
        QWidget(),
        serial(serial),
        labelR(this),
        labelG(this),
        labelB(this),
        sliderR(Qt::Horizontal, this),
        sliderG(Qt::Horizontal, this),
        sliderB(Qt::Horizontal, this),
        sliderAll(Qt::Horizontal, this),
        buttonSend(this)
{
  setBaseSize(QSize(300, 300));

  sliderR.setRange(0, 255);
  sliderG.setRange(0, 255);
  sliderB.setRange(0, 255);
  sliderAll.setRange(0, 255);

  QGridLayout *layout = new QGridLayout();

  setLayout(layout);

  layout->addWidget(new QLabel("R"), 0, 0);
  layout->addWidget(&sliderR, 0, 1);
  layout->addWidget(&labelR, 0, 2);
  layout->addWidget(new QLabel("G"), 1, 0);
  layout->addWidget(&sliderG, 1, 1);
  layout->addWidget(&labelG, 1, 2);
  layout->addWidget(new QLabel("B"), 2, 0);
  layout->addWidget(&sliderB, 2, 1);
  layout->addWidget(&labelB, 2, 2);
  layout->addWidget(new QLabel("All"), 3, 0);
  layout->addWidget(&sliderAll, 3, 1);
  layout->addWidget(&buttonSend, 4, 2);

  connect(&sliderR, &QSlider::valueChanged, this, &DebugView::updatePixelValue);
  connect(&sliderG, &QSlider::valueChanged, this, &DebugView::updatePixelValue);
  connect(&sliderB, &QSlider::valueChanged, this, &DebugView::updatePixelValue);
  connect(&sliderAll, &QSlider::valueChanged, this, &DebugView::setAll);
  connect(&buttonSend, &QPushButton::clicked, this, &DebugView::updatePixelValue);
}

DebugView::~DebugView()
{
  
}

void DebugView::updatePixelValue() {
  labelR.setText(QString::number(sliderR.value()));
  labelG.setText(QString::number(sliderG.value()));
  labelB.setText(QString::number(sliderB.value()));

  uint32_t pixel = ((sliderR.value() & 0xFF) << 24) | ((sliderG.value() & 0xFF) << 16) | ((sliderB.value() & 0xFF) << 8);
  serial->writePixelValue(pixel);
}

void DebugView::setAll() {
  sliderR.setValue(sliderAll.value());
  sliderG.setValue(sliderAll.value());
  sliderB.setValue(sliderAll.value());
  updatePixelValue();
}

