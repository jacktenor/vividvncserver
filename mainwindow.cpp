#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <rfb/rfb.h>
#include <rfb/rfbproto.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Defaults
    ui->chkLocalhostOnly->setChecked(true);
    ui->chkRequirePassword->setChecked(false);
    ui->editPassword->setEnabled(false);

    MainWindow::setMinimumWidth(525);
    MainWindow::setMaximumWidth(525);

    setUiRunning(false);
    setStatus("Ready.");    
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setStatus(const QString& s)
{
    if (ui->lblStatus) ui->lblStatus->setText(s);
}

void MainWindow::setUiRunning(bool running)
{

   setStyleSheet(R"(
    QCheckBox::indicator {
        width: 14px;
        height: 14px;
        border: 1px solid palette(mid);
        border-radius: 3px;
        background: transparent;
    }
    QCheckBox::indicator:checked {
        border: 1px solid palette(mid);
        background: transparent;
        image: url(:/icons/checkmark.png);
    }
)");

    ui->btnStart->setEnabled(!running);
    ui->btnStop->setEnabled(running);

    ui->spinPort->setEnabled(!running);
    ui->spinFps->setEnabled(!running);
    ui->chkLocalhostOnly->setEnabled(!running);
    ui->chkRequirePassword->setEnabled(!running);
    ui->editPassword->setEnabled(!running && ui->chkRequirePassword->isChecked());
}

void MainWindow::on_chkRequirePassword_toggled(bool checked)
{
    ui->editPassword->setEnabled(checked && ui->btnStart->isEnabled());
}

void MainWindow::on_btnStart_clicked()
{
    // Apply UI settings
    m_server.setListenLocalhostOnly(ui->chkLocalhostOnly->isChecked());
    m_server.setRequirePassword(ui->chkRequirePassword->isChecked());
    m_server.setPassword(ui->editPassword->text());

    const int port = ui->spinPort->value();
    const int fps  = ui->spinFps->value();

    setUiRunning(true);

    if (!m_server.start(port, fps)) {
        setStatus("Failed to start server. Check console output.");
        setUiRunning(false);
        return;
    }

    if (ui->chkLocalhostOnly->isChecked()) {
        setStatus(QString("Running on 127.0.0.1:%1 (use SSH tunnel).").arg(port));
    } else {
        setStatus(QString("Running on LAN port %1 (WARNING: exposed).").arg(port));
    }
}

void MainWindow::on_btnStop_clicked()
{
    m_server.stop();
    setStatus("Stopped.");
    setUiRunning(false);
}

void MainWindow::setListenLocalhostOnly(bool v)
{
    m_server.setListenLocalhostOnly(v);
}

void MainWindow::setRequirePassword(bool v)
{
    m_server.setRequirePassword(v);
}

void MainWindow::setPassword(const QString& p)
{
    m_server.setPassword(p);
}

bool MainWindow::startServer(int port, int fps)
{
    return m_server.start(port, fps);
}
