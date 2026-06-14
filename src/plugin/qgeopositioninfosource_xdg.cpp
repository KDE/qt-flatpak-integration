/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qgeopositioninfosource_xdg.h"

#include "locationinterface.h"

#include <QDBusConnection>
#include <QDebug>

using namespace Qt::Literals;

QGeoPositionInfoSourceXdg::QGeoPositionInfoSourceXdg(const QVariantMap &parameters, QObject *parent)
    : QGeoPositionInfoSource(parent)
{
}

QGeoPositionInfoSourceXdg::~QGeoPositionInfoSourceXdg() = default;

QGeoPositionInfo QGeoPositionInfoSourceXdg::lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const
{
    return m_lastPos;
}

QGeoPositionInfoSource::PositioningMethods QGeoPositionInfoSourceXdg::supportedPositioningMethods() const
{
    // we don't know any better
    return QGeoPositionInfoSource::AllPositioningMethods;
}

int QGeoPositionInfoSourceXdg::minimumUpdateInterval() const
{
    return 0;
}

QGeoPositionInfoSource::Error QGeoPositionInfoSourceXdg::error() const
{
    return {};
}

void QGeoPositionInfoSourceXdg::startUpdates()
{
    // TODO
    qDebug() << "START";
    m_interface = new OrgFreedesktopPortalLocationInterface(u"org.freedesktop.portal.Desktop"_s, u"/org/freedesktop/portal/desktop"_s, QDBusConnection::sessionBus(), this);
    connect(m_interface, &OrgFreedesktopPortalLocationInterface::LocationUpdated, this, &QGeoPositionInfoSourceXdg::locationUpdated);
    QVariantMap args{
        { "session_handle_token"_L1, u"1"_s },
        { "distance-threshold"_L1, 0 },
        { "time-threshold"_L1, 0 },
        { "accuracy"_L1, 5 },
        { "handle_token"_L1, u"2"_s }
    };
    const QDBusObjectPath h = m_interface->CreateSession(args);
    qDebug() << h;
    m_interface->Start(h, {}, {});
}

void QGeoPositionInfoSourceXdg::stopUpdates()
{
    qDebug() << "STOP";
}

void QGeoPositionInfoSourceXdg::requestUpdate(int timeout)
{
    qDebug() << "SINGLE REQUEST" << timeout;
}

void QGeoPositionInfoSourceXdg::locationUpdated(const QDBusObjectPath &handle, const QVariantMap &location)
{
    qDebug() << "UPDATE" << handle << location;
    QGeoCoordinate coord(
        location.value("Latitude"_L1).toDouble(),
        location.value("Longitude"_L1).toDouble(),
        location.value("Altitude"_L1).toDouble()
    );
    m_lastPos = {};
    m_lastPos.setCoordinate(coord);
    m_lastPos.setAttribute(QGeoPositionInfo::GroundSpeed, location.value("Speed"_L1).toDouble());
    m_lastPos.setAttribute(QGeoPositionInfo::Direction, location.value("Heading"_L1).toDouble());

    const auto ts = location.value("Timestamp"_L1).value<QDBusArgument>();
    quint64 secs, µsecs;
    ts.beginStructure();
    ts >> secs >> µsecs;
    ts.endStructure();
    m_lastPos.setTimestamp(QDateTime::fromMSecsSinceEpoch(secs * 1000 + µsecs / 1000, QTimeZone::UTC));

    // TODO
    // Accuracy
    // Description

    Q_EMIT positionUpdated(m_lastPos);
}

#include "moc_qgeopositioninfosource_xdg.cpp"
