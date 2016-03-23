/*
 * QML Material - An application framework implementing Material Design.
 *
 * Copyright (C) 2016 Michael Spencer <sonrisesoftware@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "units.h"

#include <QGuiApplication>
#include <QQuickItem>

UnitsAttached::UnitsAttached(QObject *attachee)
    : QObject(attachee)
    , m_screen(nullptr)
    , m_window(nullptr)
{
    m_attachee = qobject_cast<QQuickItem *>(attachee);

    if (m_attachee) {
        if (m_attachee->window()) //It might not be assigned to a window yet
            windowChanged(m_attachee->window());
    } else {
        QQuickWindow *window = qobject_cast<QQuickWindow*>(attachee);
        if (window)
            windowChanged(window);
    }

    if (!m_screen)
        screenChanged(QGuiApplication::primaryScreen());
}

void UnitsAttached::windowChanged(QQuickWindow *window)
{
    if (m_window)
        disconnect(m_window, &QQuickWindow::screenChanged, this, &UnitsAttached::screenChanged);

    m_window = window;
    screenChanged(window ? window->screen() : nullptr);

    if (window)
        connect(window, &QQuickWindow::screenChanged, this, &UnitsAttached::screenChanged);
}

void UnitsAttached::screenChanged(QScreen *screen)
{
    if (screen != m_screen) {
        QScreen *oldScreen = m_screen;
        m_screen = screen;

        if (oldScreen)
            oldScreen->disconnect(this);

        if (oldScreen == nullptr || screen == nullptr ||
                screen->physicalDotsPerInch() != oldScreen->physicalDotsPerInch() ||
                screen->logicalDotsPerInch() != oldScreen->logicalDotsPerInch() ||
                screen->devicePixelRatio() != oldScreen->devicePixelRatio()) {
            emit dpChanged();
        }
    }
}

int UnitsAttached::dp() const {
    QString qtVersion = qVersion();

    if (qtVersion > "5.6") {
        return 1;
    } else {
        return dpi()/160;
    }
}

int UnitsAttached::dpi() const
{
    if (m_screen == nullptr)
        return 72;

#if defined(Q_OS_IOS)
    // iOS integration of scaling (retina, non-retina, 4K) does itself.
    return m_screen->physicalDotsPerInch();
#elif defined(Q_OS_ANDROID)
    // https://bugreports.qt-project.org/browse/QTBUG-35701
    // recalculate dpi for Android

    QAndroidJniEnvironment env;
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject resources = activity.callObjectMethod("getResources", "()Landroid/content/res/Resources;");
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        return EXIT_FAILURE;
    }

    QAndroidJniObject displayMetrics = resources.callObjectMethod("getDisplayMetrics", "()Landroid/util/DisplayMetrics;");
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        return EXIT_FAILURE;
    }
    return displayMetrics.getField<int>("densityDpi");
#else
    // standard dpi
    return m_screen->logicalDotsPerInch() * m_screen->devicePixelRatio();
#endif
}
