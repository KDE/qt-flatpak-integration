# SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(libqtposition_xdg MODULE)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_LOCATION_XML} locationinterface)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_REQUEST_XML} requestinterface)
qt6_add_dbus_interface(xdg_iface_srcs ${XDG_SESSION_XML} sessioninterface)
ecm_qt_declare_logging_category(libqtposition_xdg
    HEADER logging.h
    IDENTIFIER Log
    CATEGORY_NAME org.kde.qtpositioning.xdg
    DESCRIPTION "Qt Positioning XDG plugin"
    EXPORT XDGPLUGIN
)
target_sources(libqtposition_xdg PRIVATE
    qgeopositioninfosource_xdg.cpp
    qgeopositioninfosourcefactory_xdg.cpp
    ${xdg_iface_srcs}
)
target_link_libraries(libqtposition_xdg PRIVATE Qt::DBus Qt::Positioning Qt::Gui Qt::GuiPrivate)

install(TARGETS libqtposition_xdg DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/position/)
ecm_qt_install_logging_categories(
    EXPORT XDGPLUGIN
    FILE org_kde_qtpositioning_xdg.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
