/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <private/qpermissions_p.h>

#include <QMap>

class XdgPermissionPlugin : public QPermissionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.QPermissionPluginInterface.6.5" FILE "plugin.json")

public:
    explicit XdgPermissionPlugin();
    ~XdgPermissionPlugin();

    Qt::PermissionStatus checkPermission(const QPermission &permission) override;
    void requestPermission(const QPermission &permission, const QPermissions::Private::PermissionCallback &callback) override;

private:
    [[nodiscard]] QString parentWindowId() const;

    QMap<QLocationPermission::Accuracy, Qt::PermissionStatus> m_locationPermission;
};
