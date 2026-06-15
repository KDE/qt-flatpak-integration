# SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

set (RELEASE_SERVICE_VERSION_MAJOR "26")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(xdg-portal-location-integration VERSION ${RELEASE_SERVICE_VERSION})

find_package(ECM 6.25 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(FeatureSummary)
include(ECMSetupVersion)
include(KDEInstallDirs)
include(ECMFindQmlModule)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQmlModule)
include(ECMGenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMAddTests)

ecm_setup_version(PROJECT
    VARIABLE_PREFIX KNMEASERVER
    VERSION_HEADER knmeaserver_version.h
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KNMEAServerConfigVersion.cmake"
    SOVERSION 1
)

find_package(Qt6 REQUIRED COMPONENTS DBus Location Network Positioning Quick Test)
find_package(KF6 REQUIRED COMPONENTS DNSSD Kirigami)

find_file(XDG_LOCATION_XML NAME org.freedesktop.portal.Location.xml PATH_SUFFIXES share/dbus-1/interfaces REQUIRED)
find_file(XDG_SESSION_XML NAME org.freedesktop.portal.Session.xml PATH_SUFFIXES share/dbus-1/interfaces REQUIRED)

add_subdirectory(src)
add_subdirectory(autotests)
