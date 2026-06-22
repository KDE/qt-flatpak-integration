# SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(xdgpermissionplugin MODULE)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_LOCATION_XML} locationinterface)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_REQUEST_XML} requestinterface)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_SESSION_XML} sessioninterface)
target_sources(xdgpermissionplugin PRIVATE
    xdgpermissionplugin.cpp
    ${xdg_iface_srcs}
)
target_link_libraries(xdgpermissionplugin PRIVATE Qt::CorePrivate Qt::DBus Qt::GuiPrivate)

install(TARGETS xdgpermissionplugin DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/permissions/)
