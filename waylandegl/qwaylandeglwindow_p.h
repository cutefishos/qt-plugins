// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QWAYLANDEGLWINDOW_H
#define QWAYLANDEGLWINDOW_H

#include <QtWaylandClient/private/qwaylandviewport_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include "qwaylandeglinclude_p.h"
#include "qwaylandeglclientbufferintegration_p.h"

#include <atomic>

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObject;

namespace CutefishEgl {

using namespace QtWaylandClient;

class QWaylandGLContext;

class QWaylandEglWindow : public QWaylandWindow
{
    Q_OBJECT
public:
    QWaylandEglWindow(QWindow *window, QWaylandDisplay *display);
    ~QWaylandEglWindow();
    WindowType windowType() const override;
    void ensureSize() override;

    void updateSurface(bool create);
    void resizeForFrame();
    void setViewportForFrame();
    void setGeometry(const QRect &rect) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    QRect contentsRect() const;

    EGLSurface eglSurface() const;
    GLuint contentFBO() const;
    GLuint contentTexture() const;
    bool needToUpdateContentFBO() const { return decoration() && (m_resize || !m_contentFBO); }

    QSurfaceFormat format() const override;

    void bindContentFBO();

    void invalidateSurface() override;

    QMutex* eglSurfaceLock();

private:
    QWaylandEglClientBufferIntegration *m_clientBufferIntegration = nullptr;
    struct wl_egl_window *m_waylandEglWindow = nullptr;

    // Locks any manipulation of the eglSurface size
    QMutex m_eglSurfaceLock;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    mutable bool m_resize = false;
    mutable QOpenGLFramebufferObject *m_contentFBO = nullptr;

    QSurfaceFormat m_format;
    // Size used in the last call to wl_egl_window_resize
    QSize m_requestedSize;

    // Size of the buffer used by QWaylandWindow
    // This is always written to from the main thread, potentially read from the rendering thread
    QReadWriteLock m_bufferSizeLock;
    QSize m_bufferSize;
    // The surface size m_bufferSize is for, and the attach offset still to go with it
    QSize m_bufferDestination;
    QPoint m_bufferOffset;

    // Viewport destination of the buffer being drawn, and whether its size is fixed yet;
    // both belong to the rendering thread
    QSize m_frameDestination;
    bool m_frameSizeFixed = false;

    // QWaylandWindow sets the viewport destination from the GUI thread as soon as the
    // geometry changes, which pairs it with a buffer the rendering thread drew at the old
    // size. The window takes the viewport over, and only the rendering thread sets it.
    QMutex m_viewportLock;
    QScopedPointer<QWaylandViewport> m_viewport;
    QSize m_viewportDestination;
    // Without its viewport, QWaylandWindow sets a rounded-up buffer scale on scale changes;
    // the buffers are drawn at the exact scale and the destination sizes them.
    qreal m_lastScale = 0;
    std::atomic<bool> m_resetBufferScale = false;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDEGLWINDOW_H
