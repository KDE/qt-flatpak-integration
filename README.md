# Qt Flatpak Integration

## Content

### Qt Positioning plugin

A plugin for [Qt Positioning](https://doc.qt.io/qt-6/qtpositioning-index.html) using the
[XDG Desktop Portal Location API](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Location.html)
as backend.

This has the a higher priority than the Geoclue backend provided by Qt and will thus used as first/default choice.
While the Geoclue backend has access to more details, it does require sandbox holes, which can be avoided by using
the portal API.

### QPermission plugin

This provides the platform implementation for the [QPermission API](https://doc.qt.io/qt-6/permissions.html)
for (sandboxed) Linux applications.

Currently this is only implemented for the location permission. Possible future work is to add the
camera permission, once Qt Multimedia can actually use the XDG Desktop Portal Camera API. All other
permissions currently supported by Qt have no equivalent in the portal API (yet).

Despite being buildable for older Qt versions as well, this plugin will only have an effect with Qt 6.13.

## Packaging Notes

Do not package or deploy this.

This is only useful within the [Flatpak Runtime](https://invent.kde.org/packaging/flatpak-kde-runtime/).
While it can be installed for development and testing, it should not be deployed anywhere else but
inside the Flatpak Runtime.
