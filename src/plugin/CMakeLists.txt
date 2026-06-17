# SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(libqtposition_xdg MODULE)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_LOCATION_XML} locationinterface)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_REQUEST_XML} requestinterface)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_SESSION_XML} sessioninterface)
target_sources(libqtposition_xdg PRIVATE
    qgeopositioninfosource_xdg.cpp
    qgeopositioninfosourcefactory_xdg.cpp
    ${xdg_iface_srcs}
)
target_link_libraries(libqtposition_xdg PRIVATE Qt::DBus Qt::Positioning)

install(TARGETS libqtposition_xdg DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/position/)
