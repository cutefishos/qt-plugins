#ifndef WAYLANDEXPORT_H
#define WAYLANDEXPORT_H

#include <QString>

class QWindow;

// One xdg-foreign export of an application window. The portal passes the handle
// to the dialog process, which parents itself onto the window. Exporting must
// outlive the dialog: destroying the export invalidates the relationship.
class WaylandExport
{
public:
    WaylandExport() = default;
    ~WaylandExport();

    WaylandExport(const WaylandExport &) = delete;
    WaylandExport &operator=(const WaylandExport &) = delete;

    // Returns the portal's parent_window identifier, or an empty string when
    // the window cannot be exported.
    QString exportWindow(QWindow *window);

private:
    struct wl_display *m_display = nullptr;
    struct zxdg_exported_v2 *m_exported = nullptr;
};

#endif // WAYLANDEXPORT_H
