/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qgeopositioninfosource_xdg.h"

#include "locationinterface.h"
#include "requestinterface.h"
#include "sessioninterface.h"

#include <QDBusConnection>
#include <QDebug>
#include <QUuid>

using namespace Qt::Literals;

static constexpr inline const auto PORTAL_SERVICE_NAME = "org.freedesktop.portal.Desktop"_L1;

QGeoPositionInfoSourceXdg::QGeoPositionInfoSourceXdg(const QVariantMap &parameters, QObject *parent)
    : QGeoPositionInfoSource(parent)
{
}

QGeoPositionInfoSourceXdg::~QGeoPositionInfoSourceXdg() = default;

QGeoPositionInfo QGeoPositionInfoSourceXdg::lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const
{
    if (fromSatellitePositioningMethodsOnly && m_lastPos.attribute(QGeoPositionInfo::HorizontalAccuracy) > 1) {
        return {};
    }
    return m_lastPos;
}

QGeoPositionInfoSource::PositioningMethods QGeoPositionInfoSourceXdg::supportedPositioningMethods() const
{
    // we don't know any better
    return QGeoPositionInfoSource::AllPositioningMethods;
}

int QGeoPositionInfoSourceXdg::minimumUpdateInterval() const
{
    return 1000;
}

QGeoPositionInfoSource::Error QGeoPositionInfoSourceXdg::error() const
{
    return m_error;
}

void QGeoPositionInfoSourceXdg::startUpdates()
{
    // TODO
    qDebug() << "START";
    m_interface = std::make_unique<OrgFreedesktopPortalLocationInterface>(PORTAL_SERVICE_NAME, u"/org/freedesktop/portal/desktop"_s, QDBusConnection::sessionBus());
    connect(m_interface.get(), &OrgFreedesktopPortalLocationInterface::LocationUpdated, this, &QGeoPositionInfoSourceXdg::locationUpdated);
    QVariantMap args{
        { "session_handle_token"_L1, QUuid::createUuid().toString(QUuid::Id128) },
        { "distance-threshold"_L1, 0 },
        { "time-threshold"_L1, updateInterval() / 1000 },
        { "accuracy"_L1, 5 }, // TODO
    };
    const QDBusObjectPath h = m_interface->CreateSession(args);
    qDebug() << h;
    m_session = std::make_unique<OrgFreedesktopPortalSessionInterface>(PORTAL_SERVICE_NAME, h.path(), QDBusConnection::sessionBus());
    connect(m_session.get(), &OrgFreedesktopPortalSessionInterface::Closed, this, [this](const auto &args) {
        qDebug() << "session closed" << args;
        m_error = QGeoPositionInfoSource::ClosedError;
        Q_EMIT errorOccurred(m_error);
        m_session.reset();
        m_request.reset();
        m_interface.reset();
    });
    QVariantMap startArgs{
        { "handle_token"_L1, QUuid::createUuid().toString(QUuid::Id128) }
    };
    const QDBusObjectPath req = m_interface->Start(h, {}, startArgs);
    qDebug() << req;
    m_request = std::make_unique<OrgFreedesktopPortalRequestInterface>(PORTAL_SERVICE_NAME, req.path(), QDBusConnection::sessionBus());
    connect(m_request.get(), &OrgFreedesktopPortalRequestInterface::Response, this, [this](auto response, const auto &results) {
        qDebug() << "request response" << response << results;
        if (response != 0) {
            m_error = QGeoPositionInfoSource::AccessError; // TODO diferentiate response codes
            Q_EMIT errorOccurred(m_error);
            m_session.reset();
            m_interface.reset();
        }
        m_request.release()->deleteLater();
    });
    m_error = QGeoPositionInfoSource::NoError;
}

void QGeoPositionInfoSourceXdg::stopUpdates()
{
    qDebug() << "STOP";
    if (m_request) {
        m_request->Close();
    }
    if (m_session) {
        m_session->Close();
    }
    m_session.reset();
    m_interface.reset();
}

void QGeoPositionInfoSourceXdg::requestUpdate(int timeout)
{
    qDebug() << "SINGLE REQUEST" << timeout;
}

void QGeoPositionInfoSourceXdg::locationUpdated([[maybe_unused]] const QDBusObjectPath &handle, const QVariantMap &location)
{
    qDebug() << "UPDATE" << handle << location;
    QGeoCoordinate coord(
        location.value("Latitude"_L1).toDouble(),
        location.value("Longitude"_L1).toDouble()
    );
    if (const auto alt = location.value("Altitude"_L1).toDouble(); !std::isnan(alt) && alt > std::numeric_limits<double>::lowest()) {
        coord.setAltitude(alt);
    }
    m_lastPos = {};
    m_lastPos.setCoordinate(coord);
    m_lastPos.setAttribute(QGeoPositionInfo::GroundSpeed, location.value("Speed"_L1).toDouble());
    m_lastPos.setAttribute(QGeoPositionInfo::Direction, location.value("Heading"_L1).toDouble());
    m_lastPos.setAttribute(QGeoPositionInfo::HorizontalAccuracy, location.value("Accuracy"_L1).toDouble());

    const auto ts = location.value("Timestamp"_L1).value<QDBusArgument>();
    quint64 secs, µsecs;
    ts.beginStructure();
    ts >> secs >> µsecs;
    ts.endStructure();
    m_lastPos.setTimestamp(QDateTime::fromMSecsSinceEpoch(secs * 1000 + µsecs / 1000, QTimeZone::UTC));

    Q_EMIT positionUpdated(m_lastPos);
}

#include "moc_qgeopositioninfosource_xdg.cpp"
