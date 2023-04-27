#ifndef FORM_H
#define FORM_H

#include <QWidget>

namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = nullptr);
    ~Form();


private slots:


//private:
    void on_Btn_TableSend_clicked();

public:
    Ui::Form *ui;
public:

};

#endif // FORM_H
