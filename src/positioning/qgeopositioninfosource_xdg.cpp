/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qgeopositioninfosource_xdg.h"

#include "locationinterface.h"
#include "requestinterface.h"
#include "sessioninterface.h"

#include "logging.h"

#include <QDBusConnection>
#include <QGuiApplication>
#include <QUuid>

#include <private/qdesktopunixservices_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

using namespace Qt::Literals;

static constexpr inline const auto PORTAL_SERVICE_NAME = "org.freedesktop.portal.Desktop"_L1;
static constexpr inline const auto PORTAL_PATH = "/org/freedesktop/portal/desktop"_L1;

enum XdgAccuracy {
    NONE = 0,
    COUNTRY = 1,
    CITY = 2,
    NEIGHBORHOOD = 3,
    STREET = 4,
    EXACT = 5
};

QGeoPositionInfoSourceXdg::QGeoPositionInfoSourceXdg(const QVariantMap &parameters, QObject *parent)
    : QGeoPositionInfoSource(parent)
{
    m_parentWindowId = parameters.value("parentWindowId"_L1).toString();

    m_requestTimer.setSingleShot(true);
    connect(&m_requestTimer, &QTimer::timeout, this, &QGeoPositionInfoSourceXdg::stopUpdates);
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

void QGeoPositionInfoSourceXdg::setPreferredPositioningMethods(QGeoPositionInfoSource::PositioningMethods methods)
{
    const auto needsRestart = m_interface && preferredPositioningMethods() != methods;
    const auto remainingTime = m_requestTimer.remainingTime();
    qCDebug(Log) << "setPreferredPositioningMethods" << methods << preferredPositioningMethods() << needsRestart << remainingTime;
    QGeoPositionInfoSource::setPreferredPositioningMethods(methods);
    if (needsRestart) {
        stopUpdates();
        if (remainingTime > 0) {
            requestUpdate(remainingTime);
        } else {
            startUpdates();
        }
    }
}

int QGeoPositionInfoSourceXdg::minimumUpdateInterval() const
{
    return 1000;
}

void QGeoPositionInfoSourceXdg::setUpdateInterval(int msec)
{
    const auto needsRestart = m_interface && updateInterval() != msec;
    const auto remainingTime = m_requestTimer.remainingTime();
    qCDebug(Log) << "setUpdateInterval" << msec << updateInterval() << needsRestart << remainingTime;
    QGeoPositionInfoSource::setUpdateInterval(msec);
    if (needsRestart) {
        stopUpdates();
        if (remainingTime > 0) {
            requestUpdate(remainingTime);
        } else {
            startUpdates();
        }
    }
}

QGeoPositionInfoSource::Error QGeoPositionInfoSourceXdg::error() const
{
    return m_error;
}

void QGeoPositionInfoSourceXdg::startUpdates()
{
    qCInfo(Log) << "startUpdates()";
    m_requestTimer.stop();
    if (!m_interface) {
        connectToPortal();
    }
}

void QGeoPositionInfoSourceXdg::stopUpdates()
{
    qCInfo(Log) << "stopUpdates()";
    if (m_request) {
        m_request->Close();
    }
    if (m_session) {
        m_session->Close();
    }
    m_session.reset();
    m_interface.reset();
    m_requestTimer.stop();
}

void QGeoPositionInfoSourceXdg::requestUpdate(int timeout)
{
    qCInfo(Log) << "requestUpdate" << timeout;
    if (m_interface && !m_requestTimer.isActive()) {
        // already permanently started
        return;
    }

    m_requestTimer.stop();
    connectToPortal();
    m_requestTimer.setInterval(timeout);
    m_requestTimer.start();
}

bool QGeoPositionInfoSourceXdg::setBackendProperty(const QString &name, const QVariant &value)
{
    qCDebug(Log) << "setBackendProperty" << name << value;
    if (name == "parentWindowId"_L1) {
        m_parentWindowId = value.toString();
        return true;
    }

    return QGeoPositionInfoSource::setBackendProperty(name, value);
}

void QGeoPositionInfoSourceXdg::connectToPortal()
{
    setError(QGeoPositionInfoSource::NoError);

    m_interface = std::make_unique<OrgFreedesktopPortalLocationInterface>(PORTAL_SERVICE_NAME, PORTAL_PATH, QDBusConnection::sessionBus());
    connect(m_interface.get(), &OrgFreedesktopPortalLocationInterface::LocationUpdated, this, &QGeoPositionInfoSourceXdg::locationUpdated);
    QVariantMap args{
        { "session_handle_token"_L1, QUuid::createUuid().toString(QUuid::Id128) },
        { "distance-threshold"_L1, 0 },
        { "time-threshold"_L1, updateInterval() / 1000 },
        { "accuracy"_L1, preferredPositioningMethods() & QGeoPositionInfoSource::SatellitePositioningMethods ? XdgAccuracy::EXACT : XdgAccuracy::CITY },
    };
    qCDebug(Log) << "creating session" << args;
    const auto createSessionReply = m_interface->CreateSession(args);
    const auto createSessionReplyWatcher = new QDBusPendingCallWatcher(createSessionReply, m_interface.get());
    connect(createSessionReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this](auto watcher) {
        watcher->deleteLater();
        const QDBusPendingReply<QDBusObjectPath> createSessionReply = *watcher;
        const QDBusObjectPath sessionHandle = createSessionReply;
        qCDebug(Log) << "got portal session" << sessionHandle;
        m_session = std::make_unique<OrgFreedesktopPortalSessionInterface>(PORTAL_SERVICE_NAME, sessionHandle.path(), QDBusConnection::sessionBus());
        connect(m_session.get(), &OrgFreedesktopPortalSessionInterface::Closed, this, [this](const auto &args) {
            qCInfo(Log) << "portal session closed" << args;
            setError(QGeoPositionInfoSource::ClosedError);
            m_session.reset();
            m_request.reset();
            m_interface.reset();
        });

        // we have to predict the D-Bus path of the request so we can connect
        // before calling Start(), otherwise we risk missing the signal
        // see org.freedesktop.portal.Request.xml for an explanation of this race
        // and the recommended way of handling this
        const auto requestToken = QUuid::createUuid().toString(QUuid::Id128);
        const QString requestPath = PORTAL_PATH + "/request/"_L1
            + QDBusConnection::sessionBus().baseService().remove(':'_L1).replace('.'_L1, '_'_L1)
            + '/'_L1 + requestToken;
        qCDebug(Log) << "watching predicted request path" << requestPath;
        m_request = std::make_unique<OrgFreedesktopPortalRequestInterface>(PORTAL_SERVICE_NAME, requestPath, QDBusConnection::sessionBus());
        connect(m_request.get(), &OrgFreedesktopPortalRequestInterface::Response, this, &QGeoPositionInfoSourceXdg::requestResponse);

        QVariantMap startArgs{
            { "handle_token"_L1, requestToken }
        };
        determineWindowId();
        qCDebug(Log) << "starting portal request" << m_parentWindowId << startArgs;
        const auto requestReply = m_interface->Start(sessionHandle, m_parentWindowId, startArgs);
        const auto requestReplyWatcher = new QDBusPendingCallWatcher(requestReply, m_interface.get());
        connect(requestReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this](auto watcher) {
            watcher->deleteLater();
            const QDBusPendingReply<QDBusObjectPath> requestReply = *watcher;
            const QDBusObjectPath reqHandle = requestReply;
            qCDebug(Log) << "portal request created:" << reqHandle;
            if (reqHandle.path() != m_request->path()) {
                qCWarning(Log) << "predicted request path did not match!";
                m_request = std::make_unique<OrgFreedesktopPortalRequestInterface>(PORTAL_SERVICE_NAME, reqHandle.path(), QDBusConnection::sessionBus());
                connect(m_request.get(), &OrgFreedesktopPortalRequestInterface::Response, this, &QGeoPositionInfoSourceXdg::requestResponse);
            }
        });
    });
}

void QGeoPositionInfoSourceXdg::requestResponse(quint32 response, const QVariantMap &results)
{
    if (!m_request) {
        // no request in progress
        return;
    }

    qCDebug(Log) << "request response" << response << results;
    if (response != 0) {
        setError(response == 1 ? QGeoPositionInfoSource::AccessError : QGeoPositionInfoSource::ClosedError);
        m_session.reset();
        m_interface.release()->deleteLater(); // we are potentially still in the watcher signal processing
    }

    m_request.release()->deleteLater();
}

void QGeoPositionInfoSourceXdg::setError(QGeoPositionInfoSource::Error error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    Q_EMIT errorOccurred(error);
}

void QGeoPositionInfoSourceXdg::locationUpdated([[maybe_unused]] const QDBusObjectPath &handle, const QVariantMap &location)
{
    qCDebug(Log) << "got position update" << handle << location;
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
    if (const auto heading = location.value("Heading"_L1).toDouble(); heading >= 0.0) {
        // Geoclue sends -1.0 for unknown heading
        m_lastPos.setAttribute(QGeoPositionInfo::Direction, heading);
    }
    m_lastPos.setAttribute(QGeoPositionInfo::HorizontalAccuracy, location.value("Accuracy"_L1).toDouble());

    const auto ts = location.value("Timestamp"_L1).value<QDBusArgument>();
    quint64 secs, µsecs;
    ts.beginStructure();
    ts >> secs >> µsecs;
    ts.endStructure();
    m_lastPos.setTimestamp(QDateTime::fromMSecsSinceEpoch((secs * 1000) + (µsecs / 1000), QTimeZone::UTC));

    Q_EMIT positionUpdated(m_lastPos);
}

void QGeoPositionInfoSourceXdg::determineWindowId()
{
    // try to guess the window id if not provided
    if (m_parentWindowId.isEmpty()) {
        const auto services = QGuiApplicationPrivate::platformIntegration()->services();
        if (const auto unixServices = dynamic_cast<QDesktopUnixServices*>(services)) {
            for (const auto window : QGuiApplication::topLevelWindows()) {
                if (m_parentWindowId = unixServices->portalWindowIdentifier(window); !m_parentWindowId.isEmpty()) {
                    break;
                }
            }
        }
    }
}

#include "moc_qgeopositioninfosource_xdg.cpp"
