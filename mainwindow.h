#pragma once

#include <QMainWindow>
#include "vncserver.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

void setListenLocalhostOnly(bool v);
void setRequirePassword(bool v);
void setPassword(const QString& p);
bool startServer(int port, int fps);

private slots:
    void on_chkRequirePassword_toggled(bool checked);
    void on_btnStart_clicked();
    void on_btnStop_clicked();

private:
    void setUiRunning(bool running);
    void setStatus(const QString& s);

private:
    Ui::MainWindow *ui;
    VncServer m_server;
};
