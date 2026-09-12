#include "waylandexport.h"

#include <QWindow>

#ifdef CUTEFISH_PLATFORMTHEME_WAYLAND

#include <QGuiApplication>
#include <qguiapplication_platform.h>
#include <qpa/qplatformwindow_p.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <xdg-foreign-unstable-v2-client-protocol.h>

#include <cstring>

namespace {

struct ExportState {
    zxdg_exporter_v2 *exporter = nullptr;
    QString handle;
};

void registryGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface,
                    uint32_t)
{
    auto *state = static_cast<ExportState *>(data);
    if (std::strcmp(interface, zxdg_exporter_v2_interface.name) == 0) {
        state->exporter = static_cast<zxdg_exporter_v2 *>(
                wl_registry_bind(registry, name, &zxdg_exporter_v2_interface, 1));
    }
}

} // namespace

QString WaylandExport::exportWindow(QWindow *window)
{
    if (m_exported)
        return QString();
    if (!window)
        return QString();

    auto *application = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    auto *native = window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
    if (!application || !native || !native->surface())
        return QString();

    // A queue of our own, so the roundtrips below never dispatch Qt's events.
    m_display = application->display();
    wl_event_queue *queue = wl_display_create_queue(m_display);
    auto *wrapper = static_cast<wl_display *>(wl_proxy_create_wrapper(m_display));
    wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(wrapper), queue);
    wl_registry *registry = wl_display_get_registry(wrapper);
    wl_proxy_wrapper_destroy(wrapper);

    ExportState state;
    static const wl_registry_listener registryListener = {registryGlobal,
                                                          [](void *, wl_registry *, uint32_t) { }};
    wl_registry_add_listener(registry, &registryListener, &state);
    wl_display_roundtrip_queue(m_display, queue);

    QString handle;
    if (state.exporter) {
        m_exported = zxdg_exporter_v2_export_toplevel(state.exporter, native->surface());
        static const zxdg_exported_v2_listener exportedListener = {
                [](void *data, zxdg_exported_v2 *, const char *handle) {
                    static_cast<ExportState *>(data)->handle = QString::fromUtf8(handle);
                }};
        zxdg_exported_v2_add_listener(m_exported, &exportedListener, &state);
        wl_display_roundtrip_queue(m_display, queue);
        handle = state.handle;
        // Off the queue that is about to go, onto the display's default one.
        wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(m_exported), nullptr);
        zxdg_exporter_v2_destroy(state.exporter);
    }

    wl_registry_destroy(registry);
    wl_event_queue_destroy(queue);
    if (handle.isEmpty()) {
        if (m_exported) {
            zxdg_exported_v2_destroy(m_exported);
            m_exported = nullptr;
        }
        return QString();
    }
    return QStringLiteral("wayland:") + handle;
}

WaylandExport::~WaylandExport()
{
    if (!m_exported)
        return;
    zxdg_exported_v2_destroy(m_exported);
    wl_display_flush(m_display);
}

#else

QString WaylandExport::exportWindow(QWindow *)
{
    return QString();
}

WaylandExport::~WaylandExport() = default;

#endif
