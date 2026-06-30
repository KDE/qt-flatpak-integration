/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef QGEOPOSITIONINFOSOURCE_XDG_H
#define QGEOPOSITIONINFOSOURCE_XDG_H

#include <QGeoPositionInfoSource>
#include <QTimer>

class OrgFreedesktopPortalLocationInterface;
class OrgFreedesktopPortalRequestInterface;
class OrgFreedesktopPortalSessionInterface;

QT_BEGIN_NAMESPACE

class QDBusObjectPath;

class QGeoPositionInfoSourceXdg : public QGeoPositionInfoSource
{
    Q_OBJECT
public:
    explicit QGeoPositionInfoSourceXdg(const QVariantMap &parameters, QObject *parent = nullptr);
    ~QGeoPositionInfoSourceXdg();

    [[nodiscard]] QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const override;
    [[nodiscard]] PositioningMethods supportedPositioningMethods() const override;
    void setPreferredPositioningMethods(QGeoPositionInfoSource::PositioningMethods methods) override;
    [[nodiscard]] int minimumUpdateInterval() const override;
    void setUpdateInterval(int msec) override;
    [[nodiscard]] Error error() const override;
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout) override;
    bool setBackendProperty(const QString &name, const QVariant &value) override;

private Q_SLOTS:
    void requestResponse(quint32 response, const QVariantMap &results);

private:
    void connectToPortal();
    void setError(QGeoPositionInfoSource::Error error);
    void locationUpdated(const QDBusObjectPath &handle, const QVariantMap &location);

    void determineWindowId();
    QString m_parentWindowId;

    std::unique_ptr<OrgFreedesktopPortalLocationInterface> m_interface;
    std::unique_ptr<OrgFreedesktopPortalRequestInterface> m_request;
    std::unique_ptr<OrgFreedesktopPortalSessionInterface> m_session;

    QGeoPositionInfo m_lastPos;
    QGeoPositionInfoSource::Error m_error = NoError;

    QTimer m_requestTimer;
};

QT_END_NAMESPACE

#endif
