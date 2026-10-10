// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandeglwindow_p.h"

#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandsurface_p.h>
#include "qwaylandglcontext_p.h"

#include <QtGui/private/qeglconvenience_p.h>

#include <QDebug>
#include <QtGui/QWindow>
#include <qpa/qwindowsysteminterface.h>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QScopeGuard>

QT_BEGIN_NAMESPACE

namespace CutefishEgl {

using namespace QtWaylandClient;

QWaylandEglWindow::QWaylandEglWindow(QWindow *window, QWaylandDisplay *display)
    : QWaylandWindow(window, display)
    , m_clientBufferIntegration(static_cast<QWaylandEglClientBufferIntegration *>(mDisplay->clientBufferIntegration()))
    , m_format(window->requestedFormat())
{
    connect(display, &QWaylandDisplay::connected, this, [this] {
        m_clientBufferIntegration = static_cast<QWaylandEglClientBufferIntegration *>(
                mDisplay->clientBufferIntegration());
    });
    ensureSize();
}

QWaylandEglWindow::~QWaylandEglWindow()
{
    if (m_eglSurface) {
        eglDestroySurface(m_clientBufferIntegration->eglDisplay(), m_eglSurface);
        m_eglSurface = 0;
    }

    if (m_waylandEglWindow)
        wl_egl_window_destroy(m_waylandEglWindow);

    delete m_contentFBO;
}

QWaylandWindow::WindowType QWaylandEglWindow::windowType() const
{
    return QWaylandWindow::Egl;
}

void QWaylandEglWindow::ensureSize()
{
    // this is always called on the main thread
    QRect rect = geometry();
    QMargins margins = clientSideMargins();
    QSize destination = rect.size() + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    QSize sizeWithMargins = destination * scale();

    if (m_lastScale != scale()) {
        m_lastScale = scale();
        // QWaylandWindow::setScale() has just sent set_buffer_scale(ceil(scale)); take it
        // back before an out-of-band commit can apply it. The render thread repeats it.
        {
            QReadLocker surfaceLock(&mSurfaceLock);
            if (mSurface && mSurface->version() >= 3)
                mSurface->set_buffer_scale(1);
        }
        m_resetBufferScale = true;
    }

    // The rendering thread resizes the wl_egl_window when it starts its next frame: resized
    // from here in the middle of one, Mesa swaps the buffer being drawn for a new one.
    QWriteLocker lock(&m_bufferSizeLock);
    m_bufferSize = sizeWithMargins;
    m_bufferDestination = destination;
    m_bufferOffset += mOffset;
    mOffset = QPoint();
}

void QWaylandEglWindow::setGeometry(const QRect &rect)
{
    QWaylandWindow::setGeometry(rect);
    // If the surface was invalidated through invalidateSurface() and
    // we're now getting a resize we don't want to create it again.
    // Just resize the wl_egl_window, the EGLSurface will be created
    // the next time makeCurrent is called.
    ensureSize();
}

void QWaylandEglWindow::updateSurface(bool create)
{
    // eglSurfaceLock should be locked before calling this method

    QSize sizeWithMargins;
    QSize destination;
    {
        QReadLocker lock(&m_bufferSizeLock);
        sizeWithMargins = m_bufferSize;
        destination = m_bufferDestination;
    }

    // wl_egl_windows must have both width and height > 0
    // mesa's egl returns NULL if we try to create a, invalid wl_egl_window, however not all EGL
    // implementations may do that, so check the size ourself. Besides, we must deal with resizing
    // a valid window to 0x0, which would make it invalid. Hence, destroy it.
    if (sizeWithMargins.isEmpty()) {
        if (m_eglSurface) {
            eglDestroySurface(m_clientBufferIntegration->eglDisplay(), m_eglSurface);
            m_eglSurface = 0;
        }
        if (m_waylandEglWindow) {
            wl_egl_window_destroy(m_waylandEglWindow);
            m_waylandEglWindow = 0;
        }
        QWriteLocker lock(&m_bufferSizeLock);
        m_bufferOffset = QPoint();
    } else {
        if (m_waylandEglWindow) {
            int current_width = 0;
            int current_height = 0;
            static bool disableResizeCheck = qgetenv("QT_WAYLAND_DISABLE_RESIZECHECK").toInt();

            if (!disableResizeCheck) {
                wl_egl_window_get_attached_size(m_waylandEglWindow, &current_width, &current_height);
            }
            if (disableResizeCheck || (current_width != sizeWithMargins.width() || current_height != sizeWithMargins.height()) || m_requestedSize != sizeWithMargins) {
                QPoint offset;
                {
                    QWriteLocker lock(&m_bufferSizeLock);
                    offset = std::exchange(m_bufferOffset, QPoint());
                }
                wl_egl_window_resize(m_waylandEglWindow, sizeWithMargins.width(), sizeWithMargins.height(), offset.x(), offset.y());
                m_requestedSize = sizeWithMargins;

                m_resize = true;
            }

            // A scale and size change together can keep the buffer size: the destination
            // still has to follow.
            m_frameDestination = destination;
        } else if (create) {
            // reset() write-locks mSurfaceLock before it takes eglSurfaceLock, which the caller
            // holds: blocking here deadlocks, so skip this frame while the surface is torn down.
            if (!mSurfaceLock.tryLockForRead())
                return;

            auto unlock = qScopeGuard([this] { mSurfaceLock.unlock(); });
            if (!mSurface)
                return;

            wl_egl_window *eglWindow = wl_egl_window_create(mSurface->object(), sizeWithMargins.width(), sizeWithMargins.height());
            if (Q_UNLIKELY(!eglWindow)) {
                qCWarning(lcQpaWayland, "Could not create wl_egl_window with size %dx%d\n", sizeWithMargins.width(), sizeWithMargins.height());
                return;
            }

            QSurfaceFormat fmt = window()->requestedFormat();
            // Alpha for a client decoration, unless asked for none: Qt marks such a window
            // opaque anyway. The context decides alike, as the two must share a config.
            if (mDisplay->supportsWindowDecoration() && fmt.alphaBufferSize() != 0)
                fmt.setAlphaBufferSize(8);
            EGLConfig eglConfig = q_configFromGLFormat(m_clientBufferIntegration->eglDisplay(), fmt);
            m_format = q_glFormatFromConfig(m_clientBufferIntegration->eglDisplay(), eglConfig, fmt);

            EGLSurface eglSurface = eglCreateWindowSurface(m_clientBufferIntegration->eglDisplay(), eglConfig, (EGLNativeWindowType) eglWindow, 0);
            if (Q_UNLIKELY(eglSurface == EGL_NO_SURFACE)) {
                qCWarning(lcQpaWayland, "Could not create EGL surface (EGL error 0x%x)\n", eglGetError());
                wl_egl_window_destroy(eglWindow);
                return;
            }

            m_waylandEglWindow = eglWindow;
            m_eglSurface = eglSurface;
            m_requestedSize = sizeWithMargins;
            m_frameDestination = destination;
        }
    }
}

/*!
 * Called by the rendering thread from makeCurrent(), with eglSurfaceLock locked. Only the
 * first call of a frame resizes: a later one would come after the buffer is being drawn.
 */
void QWaylandEglWindow::resizeForFrame()
{
    if (m_frameSizeFixed)
        return;

    m_frameSizeFixed = true;
    updateSurface(false);
}

/*!
 * Called by the rendering thread right before eglSwapBuffers() commits the frame, so the
 * destination and the buffer it is for always go out together.
 */
void QWaylandEglWindow::setViewportForFrame()
{
    m_frameSizeFixed = false;

    QReadLocker surfaceLock(&mSurfaceLock);
    QMutexLocker lock(&m_viewportLock);
    if (!mSurface || !m_viewport)
        return;

    if (m_resetBufferScale.exchange(false) && mSurface->version() >= 3)
        mSurface->set_buffer_scale(1);

    if (m_frameDestination.isEmpty() || m_frameDestination == m_viewportDestination)
        return;

    m_viewport->set_destination(m_frameDestination.width(), m_frameDestination.height());
    m_viewportDestination = m_frameDestination;
}

void QWaylandEglWindow::setWindowFlags(Qt::WindowFlags flags)
{
    // initWindow() calls this right after creating the viewport.
    if (mViewport) {
        QMutexLocker lock(&m_viewportLock);
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        m_viewport.swap(mViewport);
QT_WARNING_POP
        m_viewportDestination = QSize();
    }

    QWaylandWindow::setWindowFlags(flags);
}

QRect QWaylandEglWindow::contentsRect() const
{
    QRect r = geometry();
    QMargins m = clientSideMargins();
    return QRect(m.left(), m.bottom(), r.width(), r.height());
}

QSurfaceFormat QWaylandEglWindow::format() const
{
    return m_format;
}

void QWaylandEglWindow::invalidateSurface()
{
    // QWaylandWindow::reset() calls this before it destroys the wl_surface.
    {
        QMutexLocker lock(&m_viewportLock);
        m_viewport.reset();
        m_viewportDestination = QSize();
    }

    QMutexLocker lock (&m_eglSurfaceLock);

    if (m_eglSurface) {
        eglDestroySurface(m_clientBufferIntegration->eglDisplay(), m_eglSurface);
        m_eglSurface = 0;
    }
    if (m_waylandEglWindow) {
        wl_egl_window_destroy(m_waylandEglWindow);
        m_waylandEglWindow = nullptr;
    }
    delete m_contentFBO;
    m_contentFBO = nullptr;
}

EGLSurface QWaylandEglWindow::eglSurface() const
{
    return m_eglSurface;
}

QMutex* QWaylandEglWindow::eglSurfaceLock()
{
    return &m_eglSurfaceLock;
}

GLuint QWaylandEglWindow::contentFBO() const
{
    if (!decoration())
        return 0;

    if (m_resize || !m_contentFBO) {
        QOpenGLFramebufferObject *old = m_contentFBO;
        QSize fboSize = geometry().size() * scale();
        m_contentFBO = new QOpenGLFramebufferObject(fboSize.width(), fboSize.height(), QOpenGLFramebufferObject::CombinedDepthStencil);

        delete old;
        m_resize = false;
    }

    return m_contentFBO->handle();
}

GLuint QWaylandEglWindow::contentTexture() const
{
    return m_contentFBO->texture();
}

void QWaylandEglWindow::bindContentFBO()
{
    if (decoration()) {
        contentFBO();
        m_contentFBO->bind();
    }
}

}

QT_END_NAMESPACE

#include "moc_qwaylandeglwindow_p.cpp"
