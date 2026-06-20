/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qgeopositioninfosource_xdg.h"
#include "qgeopositioninfosourcefactory_xdg.h"

#include "logging.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

QGeoPositionInfoSource *QGeoPositionInfoSourceFactoryXdg::positionInfoSource(QObject *parent, const QVariantMap &parameters)
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(u"org.freedesktop.portal.Desktop"_s)) {
        return new QGeoPositionInfoSourceXdg(parameters, parent);
    }
    qCDebug(Log) << "XDG desktop portal not found";
    return nullptr;
}

QGeoSatelliteInfoSource *QGeoPositionInfoSourceFactoryXdg::satelliteInfoSource(QObject *parent, const QVariantMap &parameters)
{
    Q_UNUSED(parent)
    Q_UNUSED(parameters)
    return nullptr;
}

QGeoAreaMonitorSource *QGeoPositionInfoSourceFactoryXdg::areaMonitor(QObject *parent, const QVariantMap &parameters)
{
    Q_UNUSED(parent)
    Q_UNUSED(parameters)
    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qgeopositioninfosourcefactory_xdg.cpp"
