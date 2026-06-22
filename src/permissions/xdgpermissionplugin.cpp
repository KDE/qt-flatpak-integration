/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xdgpermissionplugin.h"

#include "locationinterface.h"
#include "requestinterface.h"
#include "sessioninterface.h"

#include <QDebug>
#include <QDBusServiceWatcher>

using namespace Qt::Literals;

static constexpr inline const auto PORTAL_SERVICE_NAME = "org.freedesktop.portal.Desktop"_L1;
static constexpr inline const auto PORTAL_PATH = "/org/freedesktop/portal/desktop"_L1;

XdgPermissionPlugin::XdgPermissionPlugin() = default;
XdgPermissionPlugin::~XdgPermissionPlugin() = default;

Qt::PermissionStatus XdgPermissionPlugin::checkPermission(const QPermission &permission)
{
    qDebug() << "checkPermission" << permission.type().name() << permission.value<QLocationPermission>().value().accuracy() << permission.value<QLocationPermission>().value().availability();
    return m_locationPermission;
}

void XdgPermissionPlugin::requestPermission(const QPermission &permission, const QPermissions::Private::PermissionCallback &callback)
{
    qDebug() << "XXXX requestPermission" << permission.type().name();

    auto interface = new OrgFreedesktopPortalLocationInterface(PORTAL_SERVICE_NAME, PORTAL_PATH, QDBusConnection::sessionBus(), this);
    if (!interface->isValid()) {
        // no portal
        m_locationPermission = Qt::PermissionStatus::Granted;
        callback(Qt::PermissionStatus::Granted);
    }

    QVariantMap args{
        { "session_handle_token"_L1, QUuid::createUuid().toString(QUuid::Id128) },
        { "distance-threshold"_L1, 0 },
        { "time-threshold"_L1, 0 },
        { "accuracy"_L1, /*preferredPositioningMethods() & QGeoPositionInfoSource::SatellitePositioningMethods ? XdgAccuracy::EXACT : XdgAccuracy::CITY*/ 5 },
    };
    qDebug() << "creating session" << args;
    const auto createSessionReply = interface->CreateSession(args);
    const auto createSessionReplyWatcher = new QDBusPendingCallWatcher(createSessionReply, interface);
    connect(createSessionReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this, interface, callback](auto watcher) {
        watcher->deleteLater();
        const QDBusPendingReply<QDBusObjectPath> createSessionReply = *watcher;
        const QDBusObjectPath sessionHandle = createSessionReply;
        qDebug() << "got portal session" << sessionHandle;
        auto session = new OrgFreedesktopPortalSessionInterface(PORTAL_SERVICE_NAME, sessionHandle.path(), QDBusConnection::sessionBus(), interface);

        // we have to predict the D-Bus path of the request so we can connect
        // before calling Start(), otherwise we risk missing the signal
        // see org.freedesktop.portal.Request.xml for an explanation of this race
        // and the recommended way of handling this
        const auto requestToken = QUuid::createUuid().toString(QUuid::Id128);
        const QString requestPath = PORTAL_PATH + "/request/"_L1
            + QDBusConnection::sessionBus().baseService().remove(':'_L1).replace('.'_L1, '_'_L1)
            + '/'_L1 + requestToken;
        qDebug() << "XXX watching predicted request path" << requestPath;
        auto request = new OrgFreedesktopPortalRequestInterface(PORTAL_SERVICE_NAME, requestPath, QDBusConnection::sessionBus(), session);
        connect(request, &OrgFreedesktopPortalRequestInterface::Response, this, [this, interface, callback](quint32 response, const QVariantMap &results) {
            m_locationPermission = response == 0 ? Qt::PermissionStatus::Granted : Qt::PermissionStatus::Denied;
            qDebug() << "XXX got permission reponse" << m_locationPermission;
            callback(m_locationPermission);
            interface->deleteLater();
        }, Qt::SingleShotConnection);

        QVariantMap startArgs{
            { "handle_token"_L1, requestToken }
        };
        // determineWindowId();
        // qDebug() << "starting portal request" << m_parentWindowId << startArgs;
        const auto requestReply = interface->Start(sessionHandle, /*m_parentWindowId*/ {}, startArgs);
        const auto requestReplyWatcher = new QDBusPendingCallWatcher(requestReply, interface);
        connect(requestReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this, interface](auto watcher) {
            watcher->deleteLater();
            const QDBusPendingReply<QDBusObjectPath> requestReply = *watcher;
            const QDBusObjectPath reqHandle = requestReply;
            qDebug() << "XXX portal request created:" << reqHandle;
            // if (reqHandle.path() != m_request->path()) {
            //     qCWarning(Log) << "predicted request path did not match!";
            //     m_request = std::make_unique<OrgFreedesktopPortalRequestInterface>(PORTAL_SERVICE_NAME, reqHandle.path(), QDBusConnection::sessionBus());
            //     connect(m_request.get(), &OrgFreedesktopPortalRequestInterface::Response, this, &QGeoPositionInfoSourceXdg::requestResponse);
            // }
        });
    });

#if 0
    if (QMessageBox::question(nullptr, u"Location Permission"_s, u"Ok?"_s) == QMessageBox::Yes) {
        m_locationPermission = Qt::PermissionStatus::Granted;
    } else {
        m_locationPermission = Qt::PermissionStatus::Denied;
    }
#endif
    callback(m_locationPermission);
};
