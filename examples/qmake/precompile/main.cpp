// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include "mydialog.h"
#include "myobject.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MyObject obj;
    MyDialog dialog;

    dialog.connect(dialog.aButton, SIGNAL(clicked()), SLOT(close()));
    dialog.show();

    return app.exec();
}
//! [0]
