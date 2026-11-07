# SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

project(xdg-portal-location-integration VERSION 6.12.0)

find_package(ECM 6.25 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(FeatureSummary)
include(ECMSetupVersion)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

find_package(Qt6 REQUIRED COMPONENTS CorePrivate DBus Gui GuiPrivate Network Positioning)

find_file(XDG_LOCATION_XML NAME org.freedesktop.portal.Location.xml PATH_SUFFIXES share/dbus-1/interfaces REQUIRED)
find_file(XDG_REQUEST_XML NAME org.freedesktop.portal.Request.xml PATH_SUFFIXES share/dbus-1/interfaces REQUIRED)
find_file(XDG_SESSION_XML NAME org.freedesktop.portal.Session.xml PATH_SUFFIXES share/dbus-1/interfaces REQUIRED)

add_subdirectory(src)
