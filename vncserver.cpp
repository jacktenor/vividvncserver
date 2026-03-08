#include "vncserver.h"

#ifdef _WIN32
    // Winsock must be included before windows.h
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/keysym.h>
    #include <X11/extensions/XTest.h>
    #include <X11/extensions/XShm.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
#endif

#include <QDebug>
#include <QPixmap>
#include <QCryptographicHash>
#include <QByteArray>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <QGuiApplication>
#include <QClipboard>
#include <QSignalBlocker>

VncServer::VncServer(QObject* parent) : QObject(parent)
{
#ifdef _WIN32
    // XShm is Linux/X11-only
    m_useXShm = false;
#endif
    connect(&m_timer, &QTimer::timeout, this, &VncServer::captureAndUpdate);
}

VncServer::~VncServer()
{
    if (m_screen) {
        rfbScreenCleanup(m_screen);
        m_screen = nullptr;
    }
}

static constexpr int TILE = 32;  // 16 or 32 are good; 32 = fewer checks

static bool g_settingClipboardFromClient = false;

#ifndef _WIN32
static KeySym normalizeRfbKeysymToX11(KeySym ks)
{
    // Many clients send "Unicode" keysyms: 0x01000000 | codepoint
    // Convert them into X11 "Uxxxx" keysyms which X can map.
    if ((ks & 0xFF000000) == 0x01000000) {
        unsigned int cp = (unsigned int)(ks & 0x00FFFFFF);

        // ASCII fast path
        if (cp < 0x80) return (KeySym)cp;

        // Build "Uxxxx" string for XStringToKeysym
        char buf[16];
        if (cp <= 0xFFFF)
            std::snprintf(buf, sizeof(buf), "U%04X", cp);
        else
            std::snprintf(buf, sizeof(buf), "U%06X", cp);

        KeySym uks = XStringToKeysym(buf);
        if (uks != NoSymbol) return uks;
    }

    return ks;
}
#else
// On Windows we translate RFB keysyms to Win32 virtual keys.
static uint32_t normalizeRfbKeysymToX11(uint32_t ks) { return ks; }
#endif
static void onSetXCutText(char* text, int len, rfbClientPtr cl);


bool VncServer::start(int port, int fps, const QSize& sizeHint)
{
    QScreen* scr = QGuiApplication::primaryScreen();
    if (!scr) {
        qWarning() << "No primary screen.";
        return false;
    }

    QSize initialSize = sizeHint.isValid() ? sizeHint : scr->size();
    ensureFramebufferSize(initialSize);

    if (!initXShmCapture(m_fbSize.width(), m_fbSize.height())) {
        qWarning() << "XShm init failed, falling back to Qt grabWindow().";
        m_useXShm = false;
    }

    // 32bpp truecolor (BGRA/ARGB style). LibVNCServer treats it as raw bytes.
    int bitsPerSample = 8;
    int samplesPerPixel = 3;  // RGB (we will still store 32-bit; alpha ignored)
    int bytesPerPixel = 4;

    m_screen = rfbGetScreen(nullptr, nullptr,
                            m_fbSize.width(), m_fbSize.height(),
                            bitsPerSample, samplesPerPixel, bytesPerPixel);

    if (!m_screen) {
        qWarning() << "rfbGetScreen failed.";
        return false;
    }

    m_screen->frameBuffer = reinterpret_cast<char*>(m_fbImage.bits());
    m_screen->alwaysShared = TRUE;
    m_screen->ptrAddEvent = &VncServer::onPointerEvent;
    m_screen->kbdAddEvent = &VncServer::onKeyEvent;

    m_screen->setXCutText = onSetXCutText;
    m_screen->port = port;
    m_screen->ipv6port = port;


    // Force a known pixel format: 32bpp, 24-bit color, RGB in 0x00RRGGBB order
    m_screen->serverFormat.bitsPerPixel = 32;
    m_screen->serverFormat.depth        = 24;
    m_screen->serverFormat.trueColour   = TRUE;

    m_screen->serverFormat.redMax   = 255;
    m_screen->serverFormat.greenMax = 255;
    m_screen->serverFormat.blueMax  = 255;

    // Most Linux little-endian QImage::Format_ARGB32 memory corresponds to 0xAARRGGBB
    m_screen->serverFormat.redShift   = 16;
    m_screen->serverFormat.greenShift = 8;
    m_screen->serverFormat.blueShift  = 0;


    m_screen->deferUpdateTime = 10;  // milliseconds; reduces chatter, often smoother

    // Make our instance reachable from callbacks if you use any (optional)
    m_screen->screenData = this;

    // Strongly recommended: only accept local connections (SSH tunnel)
    if (m_localhostOnly) {
    m_screen->listenInterface = htonl(INADDR_LOOPBACK);   // 127.0.0.1
    }

    rfbInitServer(m_screen);
QClipboard* cb = QGuiApplication::clipboard();
if (cb) {
    QObject::connect(cb, &QClipboard::dataChanged, this, [this, cb]() {
        if (!m_screen) return;
        if (g_settingClipboardFromClient) return; // don't echo back what client just sent
        if (!cb) return;

        const QString text = cb->text();
        if (text.isEmpty()) return;

        QByteArray utf8 = text.toUtf8();

        // Broadcast to all connected clients
        rfbSendServerCutText(m_screen, utf8.data(), utf8.size());
    });
}
    // Kick off capture loop
    int intervalMs = (fps <= 0) ? 33 : qMax(1, 1000 / fps);
    m_timer.start(intervalMs);

    m_fpsTimer.start();
    qInfo() << "VNC server started on port" << port
            << "size" << m_fbSize << "fps target" << fps;

    return true;
}

void VncServer::ensureFramebufferSize(const QSize& sz)
{
    if (sz == m_fbSize && !m_fbImage.isNull())
        return;

    m_fbSize = sz;

    // Store as 32-bit pixels for simplicity
    m_fbImage = QImage(m_fbSize, QImage::Format_ARGB32);
    m_fbImage.fill(Qt::black);
}

#ifndef _WIN32
bool VncServer::initXShmCapture(int w, int h)
{
    shutdownXShmCapture();

    m_capDpy = XOpenDisplay(nullptr);
    if (!m_capDpy) {
        qWarning() << "XOpenDisplay failed for XShm capture.";
        return false;
    }

    m_screenNum = DefaultScreen(m_capDpy);
    m_root = (unsigned long)RootWindow(m_capDpy, m_screenNum);

    if (!XShmQueryExtension(m_capDpy)) {
        qWarning() << "XShm extension not available.";
        shutdownXShmCapture();
        return false;
    }

    // Allocate real XShmSegmentInfo
    XShmSegmentInfo* info = new XShmSegmentInfo();
    std::memset(info, 0, sizeof(XShmSegmentInfo));
    m_shminfo = (void*)info;

    XImage* xi = XShmCreateImage(
        m_capDpy,
        DefaultVisual(m_capDpy, m_screenNum),
        DefaultDepth(m_capDpy, m_screenNum),
        ZPixmap,
        nullptr,
        info,
        w, h
    );

    if (!xi) {
        qWarning() << "XShmCreateImage failed.";
        shutdownXShmCapture();
        return false;
    }

    m_ximg = (void*)xi;

    const size_t imageSize =
        (size_t)xi->bytes_per_line * (size_t)xi->height;

    info->shmid = shmget(IPC_PRIVATE, imageSize, IPC_CREAT | 0600);
    if (info->shmid < 0) {
        qWarning() << "shmget failed.";
        shutdownXShmCapture();
        return false;
    }

    info->shmaddr = (char*)shmat(info->shmid, nullptr, 0);
    if (info->shmaddr == (char*)-1) {
        qWarning() << "shmat failed.";
        shutdownXShmCapture();
        return false;
    }

    xi->data = info->shmaddr;
    info->readOnly = False;

    if (!XShmAttach(m_capDpy, info)) {
        qWarning() << "XShmAttach failed.";
        shutdownXShmCapture();
        return false;
    }

    XSync(m_capDpy, False);
    m_shmReady = true;
    return true;
}
#else
bool VncServer::initXShmCapture(int, int)
{
    return false;
}

#endif


#ifndef _WIN32
void VncServer::shutdownXShmCapture()
{
    XImage* xi = (XImage*)m_ximg;
    XShmSegmentInfo* info = (XShmSegmentInfo*)m_shminfo;

    if (m_capDpy && m_shmReady && info) {
        XShmDetach(m_capDpy, info);
        XSync(m_capDpy, False);
    }

    m_shmReady = false;

    if (xi) {
        XDestroyImage(xi);
        m_ximg = nullptr;
        xi = nullptr;
    }

    if (info) {
        if (info->shmaddr && info->shmaddr != (char*)-1) {
            shmdt(info->shmaddr);
            info->shmaddr = nullptr;
        }
        if (info->shmid > 0) {
            shmctl(info->shmid, IPC_RMID, nullptr);
            info->shmid = 0;
        }

        delete info;
        m_shminfo = nullptr;
        info = nullptr;
    }

    if (m_capDpy) {
        XCloseDisplay(m_capDpy);
        m_capDpy = nullptr;
    }
}
#else
void VncServer::shutdownXShmCapture()
{
}

#endif


#ifndef _WIN32
bool VncServer::grabXShmFrameToARGB32(QImage& out)
{
    XImage* xi = (XImage*)m_ximg;

    if (!m_capDpy || !m_shmReady || !xi) return false;

    Window rootWin = (Window)m_root;

    if (!XShmGetImage(m_capDpy, rootWin, xi, 0, 0, AllPlanes)) {
        return false;
    }

    const int w = xi->width;
    const int h = xi->height;

    if (out.size() != QSize(w, h) || out.format() != QImage::Format_ARGB32) {
        out = QImage(w, h, QImage::Format_ARGB32);
    }

    const quint32 rmask = (quint32)xi->red_mask;
    const quint32 gmask = (quint32)xi->green_mask;
    const quint32 bmask = (quint32)xi->blue_mask;

    auto shiftForMask = [](quint32 mask) -> int {
        int s = 0;
        while (mask && (mask & 1u) == 0u) { mask >>= 1u; s++; }
        return s;
    };

    const int rshift = shiftForMask(rmask);
    const int gshift = shiftForMask(gmask);
    const int bshift = shiftForMask(bmask);

    const uchar* srcBase = (const uchar*)xi->data;
    const int srcStride = xi->bytes_per_line;

    for (int y = 0; y < h; ++y) {
        const quint32* src = (const quint32*)(srcBase + y * srcStride);
        quint32* dst = (quint32*)out.scanLine(y);

        for (int x = 0; x < w; ++x) {
            const quint32 p = src[x];
            const quint32 r = (p & rmask) >> rshift;
            const quint32 g = (p & gmask) >> gshift;
            const quint32 b = (p & bmask) >> bshift;
            dst[x] = 0xFF000000u | (r << 16) | (g << 8) | (b);
        }
    }

    return true;
}
#else
bool VncServer::grabXShmFrameToARGB32(QImage&)
{
    return false;
}

#endif



void VncServer::captureAndUpdate()
{
    if (!m_screen) return;

    // 1) Capture frame (prefer XShm)
    QImage img;
    bool ok = false;

    if (m_useXShm) {
        ok = grabXShmFrameToARGB32(img);
    }

    if (!ok) {
        QScreen* scr = QGuiApplication::primaryScreen();
        if (!scr) return;

        QPixmap pm = scr->grabWindow(0);
        if (pm.isNull()) return;

        img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    }

    if (img.isNull()) return;

    // 2) If resolution changed, resize framebuffer and (if using XShm) re-init capture
    if (img.size() != m_fbSize) {
        ensureFramebufferSize(img.size());

        m_screen->width  = m_fbSize.width();
        m_screen->height = m_fbSize.height();
        m_screen->frameBuffer = reinterpret_cast<char*>(m_fbImage.bits());

        rfbNewFramebuffer(m_screen, m_screen->frameBuffer,
                          m_screen->width, m_screen->height, 8, 3, 4);

        // If XShm is enabled, rebuild the shared-memory capture to match the new size.
        if (m_useXShm) {
            shutdownXShmCapture();
            if (!initXShmCapture(m_fbSize.width(), m_fbSize.height())) {
                qWarning() << "XShm re-init failed after resize. Falling back to Qt grabWindow().";
                m_useXShm = false;
            }
        }
    }

    const int w = m_fbSize.width();
    const int h = m_fbSize.height();

    const uchar* srcBits = img.constBits();
    uchar* dstBits = m_fbImage.bits();

    const int srcStride = img.bytesPerLine();
    const int dstStride = m_fbImage.bytesPerLine();

    // 3) Tile diff + copy
    // If many tiles change (video), per-tile memcmp/marking becomes overhead.
    // We'll count changed tiles and switch to a single full-frame update.
    const int tilesX = (w + TILE - 1) / TILE;
    const int tilesY = (h + TILE - 1) / TILE;
    const int totalTiles = tilesX * tilesY;

    int changedTiles = 0;

    // Store changed rects count only; we will mark per tile unless motion is high.
    for (int y = 0; y < h; y += TILE) {
        const int th = std::min(TILE, h - y);

        for (int x = 0; x < w; x += TILE) {
            const int tw = std::min(TILE, w - x);

            bool tileChanged = false;

            // Compare tile row-by-row
            for (int row = 0; row < th; ++row) {
                const uchar* s = srcBits + (y + row) * srcStride + x * 4;
                const uchar* d = dstBits + (y + row) * dstStride + x * 4;

                if (std::memcmp(s, d, (size_t)tw * 4) != 0) {
                    tileChanged = true;
                    break;
                }
            }

            if (!tileChanged) continue;

            changedTiles++;

            // High-motion heuristic: if a lot is changing, stop doing per-tile work.
            // We'll break out and do one full-frame copy+mark.
            if (changedTiles > (totalTiles / 3)) { // ~33% tiles changed => video/scrolling
                // Full frame copy
                const size_t bytesToCopy = (size_t)h * (size_t)dstStride;
                // If strides match, memcpy is fastest; otherwise copy row-by-row.
                if (srcStride == dstStride) {
                    std::memcpy(dstBits, srcBits, bytesToCopy);
                } else {
                    for (int row = 0; row < h; ++row) {
                        std::memcpy(dstBits + row * dstStride,
                                    srcBits + row * srcStride,
                                    (size_t)w * 4);
                    }
                }

                // Mark whole screen once (reduces overhead)
                rfbMarkRectAsModified(m_screen, 0, 0, w, h);

                // Process events and return
                rfbProcessEvents(m_screen, 0);
                return;
            }

            // Copy only this tile
            for (int row = 0; row < th; ++row) {
                const uchar* s = srcBits + (y + row) * srcStride + x * 4;
                uchar* d = dstBits + (y + row) * dstStride + x * 4;
                std::memcpy(d, s, (size_t)tw * 4);
            }

            // Mark only changed tile
            rfbMarkRectAsModified(m_screen, x, y, x + tw, y + th);
        }
    }

    // 4) Always process network/input events
    rfbProcessEvents(m_screen, 0);
}

void VncServer::onPointerEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
    if (!cl || !cl->screen) return;
    VncServer* self = static_cast<VncServer*>(cl->screen->screenData);
    if (!self) return;

    self->injectPointer(buttonMask, x, y);
}

void VncServer::onKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    qDebug() << "KEY EVENT: down=" << (down ? "true" : "false")
    << "keysym=0x" << Qt::hex << (unsigned long)key;

    if (!cl || !cl->screen) return;
    VncServer* self = static_cast<VncServer*>(cl->screen->screenData);
    if (!self) return;

    qDebug() << "onKeyEvent raw down value=" << (int)down;
    self->injectKey(down != 0, (unsigned long)key);
}

// LibVNCServer calls this when a client sends clipboard text.
// Signature matches: setXCutText(char* str, int len, rfbClientPtr cl)
static void onSetXCutText(char* text, int len, rfbClientPtr cl)
{
    if (!cl || !cl->screen) return;
    if (!text || len <= 0) return;

    // Your server object is stored here:
    VncServer* self = static_cast<VncServer*>(cl->screen->screenData);
    if (!self) return;

    QClipboard* cb = QGuiApplication::clipboard();
    if (!cb) return;

    QString s = QString::fromUtf8(text, len);
    if (s.contains(QChar::ReplacementCharacter)) {
        s = QString::fromLatin1(text, len);
    }

    // Prevent echo loop back to client(s)
    g_settingClipboardFromClient = true;
    const QSignalBlocker blocker(cb);
    cb->setText(s);
    g_settingClipboardFromClient = false;
}

void VncServer::setListenLocalhostOnly(bool on)
{
    m_localhostOnly = on;
}

void VncServer::stop()
{
    m_timer.stop();

    if (m_screen) {
        // Stop accepting clients and clean up
        rfbShutdownServer(m_screen, TRUE);
        rfbScreenCleanup(m_screen);
        m_screen = nullptr;
    }
     closeX11();

    shutdownXShmCapture();

}

void VncServer::setRequirePassword(bool on)
{
    m_requirePassword = on;
}

void VncServer::setPassword(const QString& password)
{
    m_password = password;
}

#ifndef _WIN32
void VncServer::ensureX11()
{
    if (m_xdpy) return;
    qDebug() << "DISPLAY env:" << qgetenv("DISPLAY");
    qDebug() << "WAYLAND_DISPLAY env:" << qgetenv("WAYLAND_DISPLAY");
    m_xdpy = XOpenDisplay(nullptr);
    qDebug() << "XOpenDisplay result:" << (m_xdpy ? "OK" : "FAILED");

    int event_base, error_base, major, minor;
    if (!XTestQueryExtension(m_xdpy, &event_base, &error_base, &major, &minor)) {
    qDebug() << "XTEST EXTENSION NOT AVAILABLE";
} else {
    qDebug() << "XTEST available, version" << major << "." << minor;
    // This is the key line - allow ourselves to inject events
    XTestGrabControl(m_xdpy, True);
    }
}

#else
void VncServer::ensureX11()
{
}

#endif


#ifndef _WIN32
void VncServer::closeX11()
{
    if (m_xdpy) {
        XCloseDisplay(m_xdpy);
        m_xdpy = nullptr;
    }
}
#else
void VncServer::closeX11()
{
}

#endif


#ifndef _WIN32
void VncServer::injectPointer(int buttonMask, int x, int y)
{
    ensureX11();
    if (!m_xdpy) return;

    // Move mouse (always)
    XTestFakeMotionEvent(m_xdpy, -1, x, y, CurrentTime);

    auto setButton = [&](int vncBit, int xButton)
    {
        bool nowDown  = (buttonMask & vncBit) != 0;
        bool wasDown  = (m_lastButtons & vncBit) != 0;

        if (nowDown != wasDown) {
            XTestFakeButtonEvent(m_xdpy, xButton, nowDown ? True : False, CurrentTime);
        }
    };

    // VNC mask: 1=left,2=middle,4=right
    setButton(1, 1);
    setButton(2, 2);
    setButton(4, 3);

    m_lastButtons = buttonMask;

    XFlush(m_xdpy);
}
#else
void VncServer::injectPointer(int buttonMask, int x, int y)
{
    // Map VNC coordinates to absolute mouse coordinates (0..65535)
    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);
    if (sw <= 1 || sh <= 1) return;

    const LONG absX = (LONG)((double)x * 65535.0 / (double)(sw - 1));
    const LONG absY = (LONG)((double)y * 65535.0 / (double)(sh - 1));

    INPUT inMove{};
    inMove.type = INPUT_MOUSE;
    inMove.mi.dx = absX;
    inMove.mi.dy = absY;
    inMove.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    // Determine button transitions
    auto sendBtn = [&](int vncBit, DWORD downFlag, DWORD upFlag)
    {
        const bool nowDown = (buttonMask & vncBit) != 0;
        const bool wasDown = (m_lastButtons & vncBit) != 0;
        if (nowDown == wasDown) return;

        INPUT inBtn{};
        inBtn.type = INPUT_MOUSE;
        inBtn.mi.dwFlags = nowDown ? downFlag : upFlag;
        SendInput(1, &inBtn, sizeof(INPUT));
    };

    SendInput(1, &inMove, sizeof(INPUT));

    // VNC mask: 1=left,2=middle,4=right
    sendBtn(1, MOUSEEVENTF_LEFTDOWN,   MOUSEEVENTF_LEFTUP);
    sendBtn(2, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP);
    sendBtn(4, MOUSEEVENTF_RIGHTDOWN,  MOUSEEVENTF_RIGHTUP);

    m_lastButtons = buttonMask;
}

#endif


#ifndef _WIN32
void VncServer::injectKey(bool down, unsigned long keysym)
{
    ensureX11();
    if (!m_xdpy) return;

    KeySym ks = normalizeRfbKeysymToX11((KeySym)keysym);

    bool needShift = (ks >= 'A' && ks <= 'Z');

    KeyCode kc = XKeysymToKeycode(m_xdpy, ks);
    if (kc == 0 && ks >= 'A' && ks <= 'Z') {
        kc = XKeysymToKeycode(m_xdpy, ks + 32);
        needShift = true;
    }
    if (kc == 0) return;

    if (needShift) {
        KeyCode shiftKc = XKeysymToKeycode(m_xdpy, XK_Shift_L);
        if (shiftKc) XTestFakeKeyEvent(m_xdpy, shiftKc, down ? True : False, CurrentTime);
    }

    XTestFakeKeyEvent(m_xdpy, kc, down ? True : False, CurrentTime);
    XFlush(m_xdpy);
}
#else
static WORD rfbKeysymToVk(uint32_t ks, bool* needShiftOut)
{
    if (needShiftOut) *needShiftOut = false;

    if (ks >= 0x20 && ks <= 0x7E) {
        const char ch = (char)ks;
        SHORT vk = VkKeyScanA(ch);
        if (vk != -1) {
            if (needShiftOut) *needShiftOut = ((vk >> 8) & 1) != 0;
            return (WORD)(vk & 0xFF);
        }
    }

    switch (ks) {
    case 0xFF0D: return VK_RETURN;
    case 0xFF08: return VK_BACK;
    case 0xFF09: return VK_TAB;
    case 0xFF1B: return VK_ESCAPE;
    case 0xFFFF: return VK_DELETE;
    case 0xFF50: return VK_HOME;
    case 0xFF57: return VK_END;
    case 0xFF55: return VK_PRIOR;
    case 0xFF56: return VK_NEXT;
    case 0xFF51: return VK_LEFT;
    case 0xFF52: return VK_UP;
    case 0xFF53: return VK_RIGHT;
    case 0xFF54: return VK_DOWN;
    case 0xFFE1: return VK_LSHIFT;
    case 0xFFE2: return VK_RSHIFT;
    case 0xFFE3: return VK_LCONTROL;
    case 0xFFE4: return VK_RCONTROL;
    case 0xFFE9: return VK_LMENU;
    case 0xFFEA: return VK_RMENU;
    default:     return 0;
    }
}

void VncServer::injectKey(bool down, unsigned long keysym)
{
    const uint32_t ks = (uint32_t)keysym;

    bool needShift = false;
    const WORD vk = rfbKeysymToVk(ks, &needShift);
    if (vk == 0) return;

    auto sendKey = [&](WORD vkey, bool isDown) {
        INPUT in{};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = vkey;
        in.ki.dwFlags = isDown ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &in, sizeof(INPUT));
    };

    if (needShift && down)  sendKey(VK_SHIFT, true);
    sendKey(vk, down);
    if (needShift && !down) sendKey(VK_SHIFT, false);
}
#endif
