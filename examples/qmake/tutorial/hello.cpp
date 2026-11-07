// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "hello.h"
#include <QDebug>

MyPushButton::MyPushButton(const QString& text) : QPushButton(text) {
    setObjectName("mypushbutton");
    qDebug() << "My PushButton has been constructed";
}
