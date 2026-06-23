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

void XdgPermissionPlugin::requestPermission(const QPermission &permission, const QPermissions::Private::PermissionCallback &callback)
{
    qCInfo(Log) << "requestPermission" << permission.type().name();
    if (permission.type().id() == qMetaTypeId<QLocationPermission>()) {
        const auto accuracy = permission.value<QLocationPermission>().value().accuracy();

        auto interface = new OrgFreedesktopPortalLocationInterface(PORTAL_SERVICE_NAME, PORTAL_PATH, QDBusConnection::sessionBus(), this);
        if (!interface->isValid()) {
            // no portal
            m_locationPermission[accuracy] = Qt::PermissionStatus::Granted;
            callback(Qt::PermissionStatus::Granted);
        }

        QVariantMap args{
            { "session_handle_token"_L1, QUuid::createUuid().toString(QUuid::Id128) },
            { "distance-threshold"_L1, 0 },
            { "time-threshold"_L1, 0 },
            { "accuracy"_L1, accuracy == QLocationPermission::Precise ? XdgAccuracy::EXACT : XdgAccuracy::CITY },
        };
        qCDebug(Log) << "creating session" << args;
        const auto createSessionReply = interface->CreateSession(args);
        const auto createSessionReplyWatcher = new QDBusPendingCallWatcher(createSessionReply, interface);
        connect(createSessionReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this, accuracy, interface, callback](auto watcher) {
            watcher->deleteLater();
            const QDBusPendingReply<QDBusObjectPath> createSessionReply = *watcher;
            const QDBusObjectPath sessionHandle = createSessionReply;
            qCDebug(Log) << "got portal session" << sessionHandle;
            auto session = new OrgFreedesktopPortalSessionInterface(PORTAL_SERVICE_NAME, sessionHandle.path(), QDBusConnection::sessionBus(), interface);
            connect(session, &OrgFreedesktopPortalSessionInterface::Closed, this, [this](const auto &args) {
                qCInfo(Log) << "portal session closed" << args;
                // TODO what do we do here?
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
            auto request = new OrgFreedesktopPortalRequestInterface(PORTAL_SERVICE_NAME, requestPath, QDBusConnection::sessionBus(), session);
            connect(request, &OrgFreedesktopPortalRequestInterface::Response, this, [this, accuracy, interface, callback](quint32 response, const QVariantMap &results) {
                m_locationPermission[accuracy] = response == 0 ? Qt::PermissionStatus::Granted : Qt::PermissionStatus::Denied;
                qCInfo(Log) << "got permission reponse" << m_locationPermission[accuracy] << results;
                callback(m_locationPermission[accuracy]);
                interface->deleteLater();
            });

            QVariantMap startArgs{
                { "handle_token"_L1, requestToken }
            };
            const auto winId = parentWindowId();
            qCDebug(Log) << "starting portal request" << winId << startArgs;
            const auto requestReply = interface->Start(sessionHandle, winId, startArgs);
            const auto requestReplyWatcher = new QDBusPendingCallWatcher(requestReply, interface);
            connect(requestReplyWatcher, &QDBusPendingCallWatcher::finished, this, [this, interface, request](auto watcher) {
                watcher->deleteLater();
                const QDBusPendingReply<QDBusObjectPath> requestReply = *watcher;
                const QDBusObjectPath reqHandle = requestReply;
                qCDebug(Log) << "portal request created:" << reqHandle;
                if (reqHandle.path() != request->path()) {
                    qCWarning(Log) << "predicted request path did not match!";
                    // TODO handle this
                }
            });
        });
    }

    callback(Qt::PermissionStatus::Undetermined);
};

QString XdgPermissionPlugin::parentWindowId() const
{
    const auto services = QGuiApplicationPrivate::platformIntegration()->services();
    if (const auto unixServices = dynamic_cast<QDesktopUnixServices*>(services)) {
        for (const auto window : QGuiApplication::topLevelWindows()) {
            if (auto winId = unixServices->portalWindowIdentifier(window); !winId.isEmpty()) {
                return winId;
            }
        }
    }
    return {};
}
