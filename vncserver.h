#pragma once

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QString>
#include <QPoint>
#include <QSize>

extern "C" {
#include <rfb/rfb.h>
}

// Forward declaration only for X11 Display (safe)
struct _XDisplay;
typedef struct _XDisplay Display;

class VncServer : public QObject
{
    Q_OBJECT

public:
    explicit VncServer(QObject* parent = nullptr);
    ~VncServer();

    bool start(int port, int fps, const QSize& sizeHint = QSize());

    void setPassword(const QString& password);
    void setRequirePassword(bool on);
    void setListenLocalhostOnly(bool on);
    void stop();

private:
    void captureAndUpdate();
    void ensureFramebufferSize(const QSize& sz);

    // ========================
    // libvncserver core
    // ========================
    rfbScreenInfoPtr m_screen = nullptr;

    // ========================
    // Framebuffer / capture loop
    // ========================
    QTimer m_timer;
    QSize m_fbSize;
    QImage m_fbImage;
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;

    // ========================
    // Security settings
    // ========================
    QString m_password;
    bool m_requirePassword = false;
    bool m_localhostOnly = true;

    // ========================
    // Input injection
    // ========================
    int m_lastButtons = 0;
    QPoint m_lastPos = QPoint(-1, -1);

#ifndef _WIN32
    // X11 display handle (used for input injection on Linux/X11)
    Display* m_xdpy = nullptr;
#endif

    // Input injection (implemented for both platforms)
    void ensureX11();
    void closeX11();
    void injectPointer(int buttonMask, int x, int y);
    void injectKey(bool down, unsigned long keysym);


    static void onPointerEvent(int buttonMask, int x, int y, rfbClientPtr cl);
    static void onKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl);


    // ========================
    // Optional: XShm high-performance capture (Linux/X11 only)
    // ========================
    bool m_useXShm = true;

#ifndef _WIN32
    Display* m_capDpy = nullptr;
    unsigned long m_root = 0;
    int m_screenNum = 0;

    void* m_ximg = nullptr;     // actually XImage*
    void* m_shminfo = nullptr;  // actually XShmSegmentInfo*
    bool m_shmReady = false;
#endif

    bool initXShmCapture(int w, int h);
    void shutdownXShmCapture();
    bool grabXShmFrameToARGB32(QImage& out);

};
