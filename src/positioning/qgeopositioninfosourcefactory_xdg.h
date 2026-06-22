/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef QGEOPOSITIONINFOSOURCEFACTORY_XDG_H
#define QGEOPOSITIONINFOSOURCEFACTORY_XDG_H

#include <QtCore/QObject>
#include <QtPositioning/QGeoPositionInfoSourceFactory>

QT_BEGIN_NAMESPACE

class QGeoPositionInfoSourceFactoryXdg : public QObject, public QGeoPositionInfoSourceFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.position.sourcefactory/6.0" FILE "plugin.json")
    Q_INTERFACES(QGeoPositionInfoSourceFactory)

public:
    QGeoPositionInfoSource *positionInfoSource(QObject *parent, const QVariantMap &parameters) override;
    QGeoSatelliteInfoSource *satelliteInfoSource(QObject *parent, const QVariantMap &parameters) override;
    QGeoAreaMonitorSource *areaMonitor(QObject *parent, const QVariantMap &parameters) override;
};

QT_END_NAMESPACE

#endif
