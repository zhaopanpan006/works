#include "form.h"
#include "ui_form.h"
#include <QTextCursor>

#include "mainwindow.h"
#include "ui_mainwindow.h"

Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);
}

Form::~Form()
{
    delete ui;
}



void Form::on_Btn_TableSend_clicked()
{

    this->close();
}
