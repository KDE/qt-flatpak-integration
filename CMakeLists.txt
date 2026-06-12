# SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

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

find_package(Qt6 REQUIRED COMPONENTS Network)
find_package(KF6 REQUIRED COMPONENTS DNSSD)

add_subdirectory(src)
