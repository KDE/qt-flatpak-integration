/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef QGEOPOSITIONINFOSOURCE_XDG_H
#define QGEOPOSITIONINFOSOURCE_XDG_H

#include <QtPositioning/QGeoPositionInfoSource>
#include <QtCore/QPointer>

class OrgFreedesktopPortalLocationInterface;

QT_BEGIN_NAMESPACE

class QDBusObjectPath;

class QGeoPositionInfoSourceXdg : public QGeoPositionInfoSource
{
    Q_OBJECT
public:
    explicit QGeoPositionInfoSourceXdg(const QVariantMap &parameters, QObject *parent = nullptr);
    ~QGeoPositionInfoSourceXdg();

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const override;
    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout) override;

private:
    void locationUpdated(const QDBusObjectPath &handle, const QVariantMap &location);

    OrgFreedesktopPortalLocationInterface *m_interface = nullptr;
    QGeoPositionInfo m_lastPos;
};

QT_END_NAMESPACE

#endif
