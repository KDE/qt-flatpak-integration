/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qgeopositioninfosource_xdg.h"
#include "qgeopositioninfosourcefactory_xdg.h"

QT_BEGIN_NAMESPACE

QGeoPositionInfoSource *QGeoPositionInfoSourceFactoryXdg::positionInfoSource(QObject *parent, const QVariantMap &parameters)
{
    Q_UNUSED(parameters)
    return new QGeoPositionInfoSourceXdg(parameters, parent);
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
