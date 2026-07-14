#include "../include/yds_sdl_window.h"

#include <algorithm>
#include <cmath>

ysSdlWindow::ysSdlWindow() : ysWindow(Platform::Sdl) {
    /* void */
}

ysSdlWindow::~ysSdlWindow() {
    /* void */
}

ysError ysSdlWindow::InitializeWindow(ysWindow *parent, std::string title, WindowStyle style, int x, int y, int width, int height, ysMonitor *monitor) {
    YDS_ERROR_DECLARE("InitializeWindow");

    if (!CheckCompatibility(parent)) return YDS_ERROR_RETURN(ysError::IncompatiblePlatforms);

    YDS_NESTED_ERROR_CALL(ysWindow::InitializeWindow(parent, title, style, x, y, width, height, monitor));
    auto *parentWindow = static_cast<ysSdlWindow *>(parent);

    Uint32 flags = 0;

    if (m_windowState == WindowState::Hidden)
        flags |= SDL_WINDOW_HIDDEN;

    switch (style) {
    case WindowStyle::Windowed:
        flags |= SDL_WINDOW_RESIZABLE;
        break;
    case WindowStyle::Fullscreen:
        flags |= SDL_WINDOW_FULLSCREEN;
        break;
    case WindowStyle::Popup:
        flags |= SDL_WINDOW_BORDERLESS;
        break;
    case WindowStyle::Unknown:
        // do nothing
        break;
    }
    // Fit a window requested larger than the display into the usable area (menu bar and
    // Dock excluded), preserving aspect ratio, and centre it. Without this a fixed
    // 1920x1080 window opens partly or wholly off-screen on smaller Retina displays
    // (e.g. a 14" MacBook Pro at 1512x982 points) — the "window goes off the edge" bug.
    if (style == WindowStyle::Windowed) {
        SDL_Rect usable;
        if (SDL_GetDisplayUsableBounds(0, &usable) == 0 && usable.w > 0 && usable.h > 0) {
            const float scale = std::min(1.0f,
                std::min((float)usable.w / (float)width, (float)usable.h / (float)height));
            width  = (int)std::round(width  * scale);
            height = (int)std::round(height * scale);
            x = usable.x + (usable.w - width) / 2;
            y = usable.y + (usable.h - height) / 2;
        }
    }

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    // TODO: choose between VULKAN and OPENGL here
    m_window = SDL_CreateWindow(title.c_str(), x, y, width, height, SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);

    // Report the *actual* Metal drawable size (in pixels) as the screen resolution so the
    // application's projection and UI layout match what Metal renders. On a Retina display
    // the drawable is 2x the window's point size; leaving m_width/m_height at the requested
    // (point) size is what makes the engine view render oversized.
    if (m_renderer != nullptr) {
        int drawableWidth = width;
        int drawableHeight = height;
        if (SDL_GetRendererOutputSize(m_renderer, &drawableWidth, &drawableHeight) == 0
                && drawableWidth > 0 && drawableHeight > 0) {
            m_width = drawableWidth;
            m_height = drawableHeight;
        }
    }

    printf("[redline] window placed=(%d,%d) size(pts)=%dx%d drawable(px)=%dx%d\n",
           x, y, width, height, m_width, m_height);
    fflush(stdout);

    return YDS_ERROR_RETURN(ysError::None);
}

void ysSdlWindow::SetState(WindowState state) {
    if (state == m_windowState)
        return;

    ysWindow::SetState(state);

    switch (state) {
    case WindowState::Visible:
        SDL_ShowWindow(m_window);
        break;
    case WindowState::Hidden:
        SDL_HideWindow(m_window);
        break;
    case WindowState::Maximized:
        SDL_MaximizeWindow(m_window);
        break;
    case WindowState::Minimized:
        SDL_MinimizeWindow(m_window);
        break;
    case WindowState::Closed:
        SDL_DestroyWindow(m_window);
        break;
    case WindowState::Unknown:
        // do nothing
        break;
    }

    m_appliedState = state;
}

void ysSdlWindow::SetTitle(std::string title) {
    SDL_SetWindowTitle(m_window, title.c_str());
}

bool ysSdlWindow::SetWindowStyle(WindowStyle style) {
    // Actually toggle SDL fullscreen (the base only records the enum), so the F key works.
    if (m_window != nullptr) {
        if (style == WindowStyle::Fullscreen) {
            SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else if (style == WindowStyle::Windowed) {
            SDL_SetWindowFullscreen(m_window, 0);
        }
    }
    return ysWindow::SetWindowStyle(style);
}

int ysSdlWindow::GetScreenWidth() const {
    if (m_renderer != nullptr) {
        int w = 0, h = 0;
        if (SDL_GetRendererOutputSize(m_renderer, &w, &h) == 0 && w > 0) return w;
    }
    return m_width;
}

int ysSdlWindow::GetScreenHeight() const {
    if (m_renderer != nullptr) {
        int w = 0, h = 0;
        if (SDL_GetRendererOutputSize(m_renderer, &w, &h) == 0 && h > 0) return h;
    }
    return m_height;
}
