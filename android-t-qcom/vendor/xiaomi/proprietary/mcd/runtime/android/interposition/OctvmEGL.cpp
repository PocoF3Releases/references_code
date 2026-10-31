#include <EGL/egl.h>
#include <EGL/egl_display.h>
#include <gui/Surface.h>
#include <utils/Timers.h>
#include <utils/Log.h>

static int64_t beginFrameTime = 0;
static int64_t endFrameTime = 0;

#ifdef USE_OPENGL_RENDERER

EGLAPI void EGLAPIENTRY eglBeginFrame(EGLDisplay dpy, EGLSurface surface);

void octvmBeginFrame(EGLDisplay dpy, EGLSurface surface)
{
    begineFrameTime = nanoseconds_to_milliseconds(systemTime());
    eglBeginFrame(dpy, surface);
}
#endif

EGLBoolean octvmSwapBuffers(EGLDisplay dpy, EGLSurface draw)
{
    EGLBoolean retVal = eglSwapBuffers(dpy, draw);

    endFrameTime = nanoseconds_to_milliseconds(systemTime());
    int64_t frameDelay = endFrameTime - beginFrameTime;
    if (beginFrameTime != 0) {
        if (frameDelay > 100) {
            LOG_EVENT_LONG(1990001, frameDelay);
        } else if (frameDelay > 17) {
            LOG_EVENT_LONG(1990000, frameDelay);
        }
        beginFrameTime = 0;
    }

    return retVal;
}
