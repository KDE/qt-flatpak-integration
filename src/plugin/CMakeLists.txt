# SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(libqtposition_xdg MODULE)
target_sources(libqtposition_xdg PRIVATE
    qgeopositioninfosource_xdg.cpp
    qgeopositioninfosourcefactory_xdg.cpp
)
target_link_libraries(libqtposition_xdg PRIVATE Qt::DBus Qt::Positioning)

install(TARGETS libqtposition_xdg DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/position/)
