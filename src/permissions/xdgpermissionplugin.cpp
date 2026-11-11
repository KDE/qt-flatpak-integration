/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xdgpermissionplugin.h"
#include "logging.h"

#include "locationinterface.h"
#include "requestinterface.h"
#include "sessioninterface.h"

#include <QDebug>
#include <QDBusServiceWatcher>
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

XdgPermissionPlugin::XdgPermissionPlugin() = default;
XdgPermissionPlugin::~XdgPermissionPlugin() = default;

Qt::PermissionStatus XdgPermissionPlugin::checkPermission(const QPermission &permission)
{
    if (permission.type().id() == qMetaTypeId<QLocationPermission>()) {
        const auto accuracy = permission.value<QLocationPermission>().value().accuracy();
        return m_locationPermission.value(accuracy, Qt::PermissionStatus::Undetermined);
    }

    qCDebug(Log) << "checkPermission" << permission.type().name();
    return Qt::PermissionStatus::Undetermined;
}

struct LocationPermissionRequest
{
    QLocationPermission::Accuracy accuracy;
    QPermissions::Private::PermissionCallback callback;
    OrgFreedesktopPortalLocationInterface *portalIface = nullptr;
};

void XdgPermissionPlugin::requestPermission(const QPermission &permission, const QPermissions::Private::PermissionCallback &callback)
{
    qCInfo(Log) << "requestPermission" << permission.type().name();
    if (permission.type().id() == qMetaTypeId<QLocationPermission>()) {
        LocationPermissionRequest req;
        req.accuracy = permission.value<QLocationPermission>().value().accuracy();
        req.callback = callback;

        req.portalIface = new OrgFreedesktopPortalLocationInterface(PORTAL_SERVICE_NAME, PORTAL_PATH, QDBusConnection::sessionBus(), this);
        if (!req.portalIface->isValid()) {
            // no portal
            m_locationPermission[req.accuracy] = Qt::PermissionStatus::Granted;
            callback(Qt::PermissionStatus::Granted);
        }

        QVariantMap args{
            { "session_handle_token"_L1, QUuid::createUuid().toString(QUuid::Id128) },
            { "distance-threshold"_L1, 0 },
            { "time-threshold"_L1, 0 },
            { "accuracy"_L1, req.accuracy == QLocationPermission::Precise ? XdgAccuracy::EXACT : XdgAccuracy::CITY },
        };
        qCDebug(Log) << "creating session" << args;
        const auto createSessionReply = req.portalIface->CreateSession(args);
        const auto createSessionReplyWatcher = new QDBusPendingCallWatcher(createSessionReply, req.portalIface);
        connect(createSessionReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this, req](auto watcher) {
            watcher->deleteLater();
            const QDBusPendingReply<QDBusObjectPath> createSessionReply = *watcher;
            const QDBusObjectPath sessionHandle = createSessionReply;
            qCDebug(Log) << "got portal session" << sessionHandle;
            auto session = new OrgFreedesktopPortalSessionInterface(PORTAL_SERVICE_NAME, sessionHandle.path(), QDBusConnection::sessionBus(), req.portalIface);
            connect(session, &OrgFreedesktopPortalSessionInterface::Closed, this, [req](const auto &args) {
                qCInfo(Log) << "portal session closed" << args;
                req.callback(Qt::PermissionStatus::Undetermined);
                req.portalIface->deleteLater();
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
            auto requestIface = new OrgFreedesktopPortalRequestInterface(PORTAL_SERVICE_NAME, requestPath, QDBusConnection::sessionBus(), session);
            connect(requestIface, &OrgFreedesktopPortalRequestInterface::Response, this, [this, req](quint32 response, const QVariantMap &results) {
                locationRequestResponse(req, response, results);
            });

            QVariantMap startArgs{
                { "handle_token"_L1, requestToken }
            };
            const auto winId = parentWindowId();
            qCDebug(Log) << "starting portal request" << winId << startArgs;
            const auto requestReply = req.portalIface->Start(sessionHandle, winId, startArgs);
            const auto requestReplyWatcher = new QDBusPendingCallWatcher(requestReply, req.portalIface);
            connect(requestReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this, req, requestIface](auto watcher) {
                watcher->deleteLater();
                const QDBusPendingReply<QDBusObjectPath> requestReply = *watcher;
                const QDBusObjectPath reqHandle = requestReply;
                qCDebug(Log) << "portal request created:" << reqHandle;
                if (reqHandle.path() != requestIface->path()) {
                    qCWarning(Log) << "predicted request path did not match!";
                    auto requestIface = new OrgFreedesktopPortalRequestInterface(PORTAL_SERVICE_NAME, reqHandle.path(), QDBusConnection::sessionBus(), req.portalIface);
                    connect(requestIface, &OrgFreedesktopPortalRequestInterface::Response, this, [this, req](quint32 response, const QVariantMap &results) {
                        locationRequestResponse(req, response, results);
                    });
                }
            });
        });
    }

    callback(Qt::PermissionStatus::Undetermined);
};

void XdgPermissionPlugin::locationRequestResponse(const LocationPermissionRequest &req, quint32 response, const QVariantMap &results)
{
    switch (response) {
        case 0:
            m_locationPermission[req.accuracy] = Qt::PermissionStatus::Granted;
            break;
        case 1:
            m_locationPermission[req.accuracy] = Qt::PermissionStatus::Denied;
            break;
        default:
            qCWarning(Log) << "got unhandled permission response code:" << response;
    };
    qCInfo(Log) << "got permission reponse" << m_locationPermission[req.accuracy] << results;
    req.callback(m_locationPermission[req.accuracy]);
    req.portalIface->deleteLater();
}

QString XdgPermissionPlugin::parentWindowId()
{
    const auto unixServices = dynamic_cast<QDesktopUnixServices *>(QGuiApplicationPrivate::platformIntegration()->services());
    return QGuiApplication::focusWindow() && unixServices
        ? unixServices->portalWindowIdentifier(QGuiApplication::focusWindow())
        : QString();
}
