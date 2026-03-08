/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpinBox *spinPort;
    QLabel *label_2;
    QSpinBox *spinFps;
    QSpacerItem *horizontalSpacer_5;
    QCheckBox *chkLocalhostOnly;
    QCheckBox *chkRequirePassword;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *editPassword;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnStart;
    QPushButton *btnStop;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_3;
    QLabel *lblStatus;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_3;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(500, 200);
        MainWindow->setMinimumSize(QSize(500, 200));
        MainWindow->setMaximumSize(QSize(600, 200));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(25, 20, 25, 10);
        label = new QLabel(centralwidget);
        label->setObjectName("label");

        horizontalLayout->addWidget(label, 0, Qt::AlignmentFlag::AlignRight);

        spinPort = new QSpinBox(centralwidget);
        spinPort->setObjectName("spinPort");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(spinPort->sizePolicy().hasHeightForWidth());
        spinPort->setSizePolicy(sizePolicy);
        spinPort->setMaximumSize(QSize(70, 16777215));
        spinPort->setMaximum(65535);
        spinPort->setValue(5903);

        horizontalLayout->addWidget(spinPort);

        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");

        horizontalLayout->addWidget(label_2, 0, Qt::AlignmentFlag::AlignRight);

        spinFps = new QSpinBox(centralwidget);
        spinFps->setObjectName("spinFps");
        spinFps->setValue(60);

        horizontalLayout->addWidget(spinFps);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_5);

        chkLocalhostOnly = new QCheckBox(centralwidget);
        chkLocalhostOnly->setObjectName("chkLocalhostOnly");
        chkLocalhostOnly->setEnabled(true);
        chkLocalhostOnly->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
        chkLocalhostOnly->setStyleSheet(QString::fromUtf8("border-left-color: rgb(115, 115, 115);"));

        horizontalLayout->addWidget(chkLocalhostOnly);

        chkRequirePassword = new QCheckBox(centralwidget);
        chkRequirePassword->setObjectName("chkRequirePassword");
        chkRequirePassword->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
        chkRequirePassword->setStyleSheet(QString::fromUtf8("border-left-color: rgb(115, 115, 115);"));

        horizontalLayout->addWidget(chkRequirePassword);


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 1);


        verticalLayout->addLayout(gridLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(10);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setSizeConstraint(QLayout::SizeConstraint::SetNoConstraint);
        horizontalLayout_2->setContentsMargins(25, 10, 0, 10);
        editPassword = new QLineEdit(centralwidget);
        editPassword->setObjectName("editPassword");
        sizePolicy.setHeightForWidth(editPassword->sizePolicy().hasHeightForWidth());
        editPassword->setSizePolicy(sizePolicy);
        editPassword->setMinimumSize(QSize(250, 25));
        editPassword->setMaximumSize(QSize(22250, 16777215));
        editPassword->setEchoMode(QLineEdit::EchoMode::Password);

        horizontalLayout_2->addWidget(editPassword);

        horizontalSpacer = new QSpacerItem(50, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        btnStart = new QPushButton(centralwidget);
        btnStart->setObjectName("btnStart");
        btnStart->setMaximumSize(QSize(125, 16777215));

        horizontalLayout_2->addWidget(btnStart);

        btnStop = new QPushButton(centralwidget);
        btnStop->setObjectName("btnStop");
        btnStop->setMaximumSize(QSize(125, 16777215));

        horizontalLayout_2->addWidget(btnStop);

        horizontalSpacer_2 = new QSpacerItem(20, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(10, 5, 10, 5);
        horizontalSpacer_3 = new QSpacerItem(10, 10, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);

        lblStatus = new QLabel(centralwidget);
        lblStatus->setObjectName("lblStatus");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lblStatus->sizePolicy().hasHeightForWidth());
        lblStatus->setSizePolicy(sizePolicy1);
        lblStatus->setMaximumSize(QSize(16777215, 15));
        lblStatus->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);

        horizontalLayout_3->addWidget(lblStatus);

        horizontalSpacer_4 = new QSpacerItem(15, 10, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_4);


        verticalLayout->addLayout(horizontalLayout_3);

        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setMinimumSize(QSize(0, 15));
        label_3->setMaximumSize(QSize(16777215, 15));
        QFont font;
        font.setPointSize(9);
        label_3->setFont(font);
        label_3->setTextFormat(Qt::TextFormat::RichText);
        label_3->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_3->setOpenExternalLinks(true);

        verticalLayout->addWidget(label_3, 0, Qt::AlignmentFlag::AlignHCenter);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "VividVNC Server", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Port:", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "F.P.S", nullptr));
        chkLocalhostOnly->setText(QCoreApplication::translate("MainWindow", "Local Host Only?", nullptr));
        chkRequirePassword->setText(QCoreApplication::translate("MainWindow", "Require Password?", nullptr));
        editPassword->setText(QString());
        editPassword->setPlaceholderText(QCoreApplication::translate("MainWindow", "Password?", nullptr));
        btnStart->setText(QCoreApplication::translate("MainWindow", "Start Server", nullptr));
        btnStop->setText(QCoreApplication::translate("MainWindow", "Stop Server", nullptr));
        lblStatus->setText(QCoreApplication::translate("MainWindow", "Status:", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "<a href=\"https://beeralator.com\">Beeralator.com</a>", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
