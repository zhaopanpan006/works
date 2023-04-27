#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QMessageBox>
#include <QTableWidget>
#include <QCloseEvent>
#include <QTime>
#include <QDateTime>
#include <QColor>
#include <QImage>

#include "form.h"
#include "ui_form.h"
#include "mprocess.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    uint16_t ParameterData0 =0;
    uint16_t ParameterData1 =0;

private slots:

    void m_TimerEvent();

    void on_atc_takeon_triggered();

    void on_atc_takeoff_triggered();

    void on_atc_rst_triggered();

    void on_atc_run_triggered();

    void on_atc_stop_triggered();

    void on_act_tcpswitch_triggered();

    void on_Com_RunMode_currentIndexChanged(int index);

    void on_Btn_OutParameterSet_clicked();

    void on_Tab_ConttrolParamater_cellDoubleClicked(int row, int column);

    void on_Tab_RatedlParamater_cellDoubleClicked(int row, int column);

    void on_Tab_ProtectParamater_cellDoubleClicked(int row, int column);

    void on_Tab_ProtectTimers_cellDoubleClicked(int row, int column);

    void on_Tab_SystemParamater_cellDoubleClicked(int row, int column);



//private:


    void on_comboBox_currentIndexChanged(int index);

    void on_Edit_OutData1_returnPressed();

    void on_Edit_OutData2_returnPressed();

    void on_Edit_OutData3_returnPressed();

    void on_btn_SystemFaultClear_clicked();

    void on_btn_TcpComConnect_clicked();

public:
    Ui::MainWindow *ui;


    QTcpSocket* m_TcpChile = nullptr;

    QTimer* m_Timer;

    Form *m_TableSendView;

    QLabel *m_toolbarline = new QLabel();
    QLabel *m_toolbarline1 = new QLabel();

    //定义状态 、故障信息的解析类
    mprocess* DataProcess = NULL;


    quint16 m_TimerCnt = 0;
    quint16 TimeRoundStatus = 0;
    quint16 VirtualRegister[5000]={0};
    quint16 VirtualRegisterAddr = 0;
    quint16 VirtualRegisterReadLength = 0;
    quint16 OutParameterMode = 0;
    quint16 ModbusTcpReadySendCnt = 0;

    quint16 TableSetFloatIntFlag = 0;
    quint16 m_SendStepFlag = 0;
    quint8 ModuleFaultFlag = 0;

    //运行参数刷新标志
    quint16 Com_RunSetRefreshFlag = 25;
    quint16 SystemInitCnt = 20;



    void m_SendForm();

    bool VirtualRegisterReadFlag = false;
    bool TcpConnectStatusFlag = false;

    void ByteToHexString(QString &str, QByteArray &ba);    
    void myMsleep(int msec);

    void ReadHeldingRegister(quint16 REG_addr,quint16 REG_len);
    void WriteSingleRegister(quint16 REG_addr,quint16 REG_value);
    void WriteSingleRegisterFloat(quint16 REG_addr,float REG_value);
    void WriteMultipleRegister(quint16 REG_addr,quint16* REG_value,quint16 REG_len);

    void m_ModbusSendStack(quint16 addr,quint16 data);
    void m_ModbusSendStack(quint16 addr,float fdata);

    void ModbusReadAllRegister();
    void ModbusReturnDataProcess(QByteArray BA);

    void TableWidgetDataRefresh();

    void SystemDataShow();
    void OutParameterSet();


    void Com_RunSetDataShow(int value);

    void TableBackgroundColorIint();
    void Tab_OperationParamaterCh1ItemIint();
    void Tab_OperationParamaterCh1SetItemIint();
    void Tab_OperationParamaterCh2SetItemIint();
    void Tab_OperationParamaterCh3SetItemIint();
    void ModuleFaultShow();




    void closeEvent(QCloseEvent *);

    unsigned int BitAnalysis(unsigned int  data,unsigned int bit);

};
#endif // MAINWINDOW_H
