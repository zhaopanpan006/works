/********************************************************************************
** Form generated from reading UI file 'form.ui'
**
** Created by: Qt User Interface Compiler version 5.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FORM_H
#define UI_FORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QGridLayout *gridLayout;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *label_3;
    QLineEdit *Edit_TableSendAddr;
    QLabel *label;
    QLineEdit *Edit_TableSendData;
    QSpacerItem *horizontalSpacer;
    QPushButton *Btn_TableSend;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QStringLiteral("Form"));
        Form->resize(240, 240);
        Form->setMinimumSize(QSize(220, 220));
        Form->setMaximumSize(QSize(500, 500));
        QFont font;
        font.setFamily(QStringLiteral("Arial"));
        font.setPointSize(14);
        Form->setFont(font);
        gridLayout = new QGridLayout(Form);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        groupBox_2 = new QGroupBox(Form);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setMinimumSize(QSize(200, 200));
        groupBox_2->setMaximumSize(QSize(200, 200));
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout_2->addWidget(label_3, 0, 0, 1, 1);

        Edit_TableSendAddr = new QLineEdit(groupBox_2);
        Edit_TableSendAddr->setObjectName(QStringLiteral("Edit_TableSendAddr"));
        Edit_TableSendAddr->setReadOnly(true);

        gridLayout_2->addWidget(Edit_TableSendAddr, 0, 1, 1, 1);

        label = new QLabel(groupBox_2);
        label->setObjectName(QStringLiteral("label"));

        gridLayout_2->addWidget(label, 1, 0, 1, 1);

        Edit_TableSendData = new QLineEdit(groupBox_2);
        Edit_TableSendData->setObjectName(QStringLiteral("Edit_TableSendData"));

        gridLayout_2->addWidget(Edit_TableSendData, 1, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(54, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer, 2, 0, 1, 1);

        Btn_TableSend = new QPushButton(groupBox_2);
        Btn_TableSend->setObjectName(QStringLiteral("Btn_TableSend"));

        gridLayout_2->addWidget(Btn_TableSend, 2, 1, 1, 1);


        gridLayout->addWidget(groupBox_2, 0, 0, 1, 1);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QApplication::translate("Form", "\345\217\202\346\225\260\350\256\276\347\275\256", Q_NULLPTR));
        groupBox_2->setTitle(QString());
        label_3->setText(QApplication::translate("Form", "\345\234\260\345\235\200\357\274\232", Q_NULLPTR));
        label->setText(QApplication::translate("Form", "\346\225\260\346\215\256\357\274\232", Q_NULLPTR));
        Edit_TableSendData->setText(QApplication::translate("Form", "0", Q_NULLPTR));
        Btn_TableSend->setText(QApplication::translate("Form", "\345\217\221\351\200\201", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FORM_H
