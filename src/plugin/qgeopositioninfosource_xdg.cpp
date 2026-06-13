/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qgeopositioninfosource_xdg.h"

QGeoPositionInfoSourceXdg::QGeoPositionInfoSourceXdg(const QVariantMap &parameters, QObject *parent)
    : QGeoPositionInfoSource(parent)
{
}

QGeoPositionInfoSourceXdg::~QGeoPositionInfoSourceXdg() = default;

QGeoPositionInfo QGeoPositionInfoSourceXdg::lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const
{
    return {};
}

QGeoPositionInfoSource::PositioningMethods QGeoPositionInfoSourceXdg::supportedPositioningMethods() const
{
    return {};
}

int QGeoPositionInfoSourceXdg::minimumUpdateInterval() const
{
    return {};
}

QGeoPositionInfoSource::Error QGeoPositionInfoSourceXdg::error() const
{
    return {};
}

void QGeoPositionInfoSourceXdg::startUpdates()
{
}

void QGeoPositionInfoSourceXdg::stopUpdates()
{
}

void QGeoPositionInfoSourceXdg::requestUpdate(int timeout)
{
}

#include "moc_qgeopositioninfosource_xdg.cpp"
