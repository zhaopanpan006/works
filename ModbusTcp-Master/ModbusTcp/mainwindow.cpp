#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "form.h"
#include "ui_form.h"

/*
 * ===运行数据=(仅支持03码)==========================
 * 0X0000~0X0008:状态信息9个寄存器
 * 0X0009~0X0017:故障信息15个寄存器
 * 0X0018~0X008A:数据信息115个寄存器float
 * ===操作命令=(仅支持06码)==========================
 * 0X0900~0X0904:操作指令5个寄存器
 * ===用户设置参数=(支持03、06、10码)=======================
 * 0X0A00~0X0A03:通用设置参数4个寄存器
 * 0X0A04~0X0A20:运行设置参数29个寄存器
 * 0X0A22~0X0A46:网络参数设置37个寄存器
 * ===厂家设置参数=(支持03、06、10码)=======================
 * 0X0FA0~0X0FC6:机型参数39个寄存器
 * 0X0FC8~0X121E:校准参数599个寄存器
 * 0X1220~0X127E:保护参数95个寄存器
 * 0X1280~0X1308:控制参数137个寄存器
 * 0X130A~0X1326:保护时间29个寄存器
 * 0X1327~0X1345:系统配置参数
 */

//申请200个结构体的内存栈，用于网口的数据收发，数据发送压入栈出栈，先进后出，防止定时发送和随时发送的接收不同步导致数据处理异常，
ModbusSendStruct m_SendStruct[200];//数据太小会在快速发送时导致数组溢出，系统崩溃

//故障显示的缓存寄存器
quint16  SystemFaulCurrentBuff[5] ;
quint16  SystemFaulHistroyBuff[5] ;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("双向DCDC控制软件");
    //m_toolbarline->setText("双向DCDC控制软件 ");
   // ui->toolBar_3->addWidget(m_toolbarline);

    //初始化定时器
    m_Timer = new QTimer(this);
    m_Timer->start(200);
    connect(m_Timer, &QTimer::timeout, this,&MainWindow::m_TimerEvent);

    //创建子窗口 用于网口发送的时候数据输入
    m_TableSendView = new Form();

    //子窗口button触发函数m_SendForm;
    connect(m_TableSendView->ui->Btn_TableSend,&QPushButton::clicked,this,&MainWindow::m_SendForm);

    //m_TcpChile声明
    m_TcpChile=new QTcpSocket(this);
    //提取服务器连接信号，提示服务器连接成功
    connect(m_TcpChile,&QTcpSocket::connected,[=](){
        ui->data_group->setPlainText("服务器连接成功！");        
       // ui->act_tcpswitch->setIcon(QIcon("://images/switch_on.png"));
        ui->btn_TcpComConnect->setText("closs");
        ui->btn_TcpComConnect->setStyleSheet("background-color: rgb(82, 255, 19)");
        ui->act_comsta->setIcon(QIcon("://images/netok.png"));
        m_toolbarline->setText("["+ui->Edit_TcpIP->text()+ "]:");
        ui->toolBar->addWidget(m_toolbarline);
        m_toolbarline1->setText(ui->Edit_TcpPort->text());
        ui->toolBar->addWidget(m_toolbarline1);

        TcpConnectStatusFlag = true;
    });
    //提取服务器断开信号，显示断开状态
    connect(m_TcpChile,&QTcpSocket::disconnected,[=](){
        ui->data_group->appendPlainText("服务器已断开！");

        //ui->act_tcpswitch->setIcon(QIcon("://images/switch_off.png"));
        ui->btn_TcpComConnect->setText("open");
        ui->btn_TcpComConnect->setStyleSheet("background-color: rgb(255, 255, 255)");
        ui->act_comsta->setIcon(QIcon("://images/netfault.png"));
        TcpConnectStatusFlag = false;
    });
    //提取接收数据，并处理
    connect(m_TcpChile,&QTcpSocket::readyRead,[=](){
        //获取通信套接字的内容
        QByteArray BA =m_TcpChile->readAll();
        ModbusReturnDataProcess(BA);
    });


    //初始化 状态故障解析类
    DataProcess = new mprocess;

    //设置Table的列间距
    ui->Tab_OperationParamater->setColumnWidth(0,250);
    ui->Tab_OperationParamater->setColumnWidth(1,200);

    ui->Tab_ConttrolParamater->setColumnWidth(0,250);
    ui->Tab_ConttrolParamater->setColumnWidth(1,170);
    ui->Tab_RatedlParamater->setColumnWidth(0,250);
    ui->Tab_RatedlParamater->setColumnWidth(1,170);
    ui->Tab_ProtectParamater->setColumnWidth(0,250);
    ui->Tab_ProtectParamater->setColumnWidth(1,170);
    ui->Tab_SystemParamater->setColumnWidth(0,250);
    ui->Tab_SystemParamater->setColumnWidth(1,170);



    //Table赋值,第一列赋寄存器地址的初值
    for(int value =0;value<5000;value++)
    {
        QString str = "0x"+QString("%1").arg(value,4,16,QChar('0')).toUpper();
        ui->Tab_ModbusRegister->setItem(value,0,new QTableWidgetItem(str));
    }
    //运行参数TABLE1初始化
    Tab_OperationParamaterCh1ItemIint();
}

MainWindow::~MainWindow()
{
    delete ui;
}
//--------------------------------------------------------------------------------------------------------------------------
//定时器触发函数--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------
void MainWindow::m_TimerEvent()
{
    //计数器累计显示
    QString str = QString::number(m_TimerCnt++);
    ui->Edit_TimerCounter->setText(str);

    //获取IP PORT
    if(m_TimerCnt == 5 )//窗口打开的第五的时钟周期时，连接网络端口
    {
        m_TcpChile->connectToHost(ui->Edit_TcpIP->text(),ui->Edit_TcpPort->text().toUInt());
        //设置超时500
        if (m_TcpChile->waitForConnected(500))
        {
            TcpConnectStatusFlag = true;
        }
        else
        {
            QMessageBox::information(this,"提示","端口：未连接");
        }
    }


    //随机发送处理区：将要发送的数据存入栈m_SendStruct，先进后出发送
    if(ModbusTcpReadySendCnt > 0 ) //待发送数量 大于0表示未发送完成，先完成发送任务
    {
        if(m_SendStruct[ModbusTcpReadySendCnt].fun == 0x06)
        {
            WriteSingleRegister(m_SendStruct[ModbusTcpReadySendCnt].addr, m_SendStruct[ModbusTcpReadySendCnt].data[0]);
        }
        else if(m_SendStruct[ModbusTcpReadySendCnt].fun == 0x10)
        {
            WriteMultipleRegister(m_SendStruct[ModbusTcpReadySendCnt].addr,m_SendStruct[ModbusTcpReadySendCnt].data,m_SendStruct[ModbusTcpReadySendCnt].len);
        }
        ModbusTcpReadySendCnt--;
    }
    else//定时任务处理区：发送数据请求完毕后开始轮询发读取指令
    {
        //定时读取所有寄存器数据，并在table上显示寄存器数据
        ModbusReadAllRegister();
    }
    //在窗体上显示，故障信息，参数和数据信息
    SystemDataShow();

    //初始化参数，获取运行状态。
    if(SystemInitCnt >0)
    {
        SystemInitCnt--;
        if(SystemInitCnt <5)
        {
            //显示运行模式
            OutParameterMode = VirtualRegister[0x0a00];
            ui->Com_RunMode->setCurrentIndex(OutParameterMode-1);
        }
    }
     //Com_RunSetRefreshFlag
    if(Com_RunSetRefreshFlag>0)
    {
        Com_RunSetRefreshFlag--;
        if(Com_RunSetRefreshFlag >= 10 && Com_RunSetRefreshFlag <= 15)
        {
            //刷新运行参数
            Com_RunSetDataShow(OutParameterMode);
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------
//TCP连接服务器TBN---------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------


void MainWindow::on_btn_TcpComConnect_clicked()
{
    if(TcpConnectStatusFlag == false)
    {   //获取IP PORT
        m_TcpChile->connectToHost(ui->Edit_TcpIP->text(),ui->Edit_TcpPort->text().toUInt());
        //设置超时500
        if (m_TcpChile->waitForConnected(500))
        {
            TcpConnectStatusFlag = true;
            SystemInitCnt = 15;
        }
        else
        {
            QMessageBox::information(this,"提示","端口：未连接");
        }
    }
    else
    {
        m_TcpChile->disconnectFromHost();
    }
}

void MainWindow::on_act_tcpswitch_triggered()
{

}

//字节数组转16进制字符串---------------------------------------------------------------------------------------------------
void MainWindow::ByteToHexString(QString &str, QByteArray &ba)
{
    QString strs = ba.toHex().toUpper();//直接转换中间没有空格
    //qDebug() << ba.toHex() << endl;

    for(int i=0;i<strs.length();i+=2)
    {
        QString str_1 = strs.mid (i,2);
        str += str_1;
        str += " ";
    }
}
//延时函数 ms---------------------------------------------------------------------------------------------------
void MainWindow::myMsleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100); //非阻塞式
}
//--------------------------------------------------------------------------------------------------------------------------
//Modbus30\06\10码函数-------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------
void MainWindow::ReadHeldingRegister(quint16 REG_addr,quint16 REG_len)
{
    quint8 SendBuff[20];
    quint16 data_cnt = 0;

    SendBuff[data_cnt++] = (quint8)(ModbusTcpOB>>8);
    SendBuff[data_cnt++]= (quint8)(ModbusTcpOB&0XFF);//事务标识符
    SendBuff[data_cnt++] = (quint8)(ModbusTcpPR>>8);
    SendBuff[data_cnt++] = (quint8)(ModbusTcpPR&0XFF);//协议标识符
    SendBuff[data_cnt++] = 0x00;
    SendBuff[data_cnt++] = 0x06;//后面字节长度
    SendBuff[data_cnt++] = ModbusTcpID;//模块ID
    SendBuff[data_cnt++] = 0x03;//功能码
    SendBuff[data_cnt++] = (quint8)(REG_addr>>8);
    SendBuff[data_cnt++] = (quint8)(REG_addr&0XFF); //读取寄存器地址
    SendBuff[data_cnt++] = (quint8)(REG_len>>8);
    SendBuff[data_cnt++] = (quint8)(REG_len&0XFF);//读取寄存器长度

    QByteArray ba((char*)SendBuff,data_cnt);
    memset(SendBuff,0,data_cnt);
    if( TcpConnectStatusFlag == true)
    {
        m_TcpChile->write(ba);
        QString str ;
        ByteToHexString(str,ba);
        ui->data_group->appendPlainText("发送:"+str);
     }
    else
    {
        //ui->data_group->append("tcp can't open!");
        return;
    }
}

void MainWindow::WriteSingleRegister(quint16 REG_addr,quint16 REG_value)
{
    quint8 SendBuff[20];
    quint16 data_cnt = 0;
    SendBuff[data_cnt++] = (quint8)(ModbusTcpOB>>8);
    SendBuff[data_cnt++] = (quint8)(ModbusTcpOB&0XFF);//事务标识符
    SendBuff[data_cnt++] = (quint8)(ModbusTcpPR>>8);
    SendBuff[data_cnt++] = (quint8)(ModbusTcpPR&0XFF);//协议标识符
    SendBuff[data_cnt++] = 0x00;
    SendBuff[data_cnt++] = 0x06;//后面字节长度
    SendBuff[data_cnt++] = ModbusTcpID;//模块ID
    SendBuff[data_cnt++] = 0x06;//功能码
    SendBuff[data_cnt++] = (quint8)(REG_addr>>8);
    SendBuff[data_cnt++] = (quint8)(REG_addr&0XFF); //写入寄存器地址
    SendBuff[data_cnt++] = (quint8)(REG_value>>8);
    SendBuff[data_cnt++] = (quint8)(REG_value&0XFF);//写入寄存器数据

    QByteArray ba((char*)SendBuff,data_cnt);
    memset(SendBuff,0,data_cnt);
    if( TcpConnectStatusFlag == true)
    {//写三次保证数据可以被写入
        //myMsleep(10);
        m_TcpChile->write(ba);
        QString str ;
        ByteToHexString(str,ba);
        ui->data_group->appendPlainText("发送:"+str);
     }
    else
    {
        //ui->data_group->append("tcp can't open!");
        return;
    }

}

void MainWindow::WriteMultipleRegister(quint16 REG_addr,quint16* REG_value,quint16 REG_len)
{
    quint8 SendBuff[50];
    quint16 data_len = REG_len*2+7;
    quint16 data_cnt = 0;
    SendBuff[data_cnt++] = (quint8)(ModbusTcpOB>>8);
    SendBuff[data_cnt++] = (quint8)(ModbusTcpOB&0XFF);//事务标识符
    SendBuff[data_cnt++] = (quint8)(ModbusTcpPR>>8);
    SendBuff[data_cnt++] = (quint8)(ModbusTcpPR&0XFF);//协议标识符
    SendBuff[data_cnt++] = (quint8)(data_len>>8);;
    SendBuff[data_cnt++] = (quint8)(data_len&0XFF);//后面字节长度
    SendBuff[data_cnt++] = ModbusTcpID;//模块ID
    SendBuff[data_cnt++] = 0x10;//功能码
    SendBuff[data_cnt++] = (quint8)(REG_addr>>8);
    SendBuff[data_cnt++] = (quint8)(REG_addr&0XFF); //写入寄存器地址
    SendBuff[data_cnt++] = (quint8)(REG_len>>8);
    SendBuff[data_cnt++] = (quint8)(REG_len&0XFF); //写入数量
    SendBuff[data_cnt++] = (quint8)(REG_len*2);//写入字节数
    for(quint16 len =0;len<REG_len;len++)
    {
        SendBuff[data_cnt++] = (quint8)((*(REG_value+len))>>8);
        SendBuff[data_cnt++] = (quint8)((*(REG_value+len))&0XFF);//写入寄存器数据
    }

    QByteArray ba((char*)SendBuff,data_cnt);
    memset(SendBuff,0,data_cnt);
    if( TcpConnectStatusFlag == true)
    {
       // myMsleep(10);
        m_TcpChile->write(ba);
       // myMsleep(10);
        QString str ;
        ByteToHexString(str,ba);
        ui->data_group->appendPlainText("发送:"+str);
     }
    else
    {
        //ui->data_group->append("tcp can't open!");
        return;
    }
}
//06码写入单个寄存器
void MainWindow::m_ModbusSendStack(quint16 addr,quint16 data)
{
    ModbusTcpReadySendCnt++;
    m_SendStruct[ModbusTcpReadySendCnt].addr    = addr;
    m_SendStruct[ModbusTcpReadySendCnt].fun     = 0x06;
    m_SendStruct[ModbusTcpReadySendCnt].data[0] = data;
}
//10码写入float的寄存器
void MainWindow::m_ModbusSendStack(quint16 addr,float fdata)
{
    FLAOT_UNION UnionData;
    UnionData.FloatData = fdata;

    ModbusTcpReadySendCnt++;
    m_SendStruct[ModbusTcpReadySendCnt].addr    = addr;
    m_SendStruct[ModbusTcpReadySendCnt].fun     = 0x10;
    m_SendStruct[ModbusTcpReadySendCnt].data[0] = UnionData.ShortData[1];
    m_SendStruct[ModbusTcpReadySendCnt].data[1] = UnionData.ShortData[0];
    m_SendStruct[ModbusTcpReadySendCnt].len     = 2;
}
//-------Modbus06码写入Float，占用两个字节--------------------------------------------------------------------------------------
void MainWindow::WriteSingleRegisterFloat(quint16 REG_addr,float REG_value)
{
    FLAOT_UNION UnionData;
    UnionData.FloatData =REG_value;
    unsigned short Temp = 0;
    //浮点型联合体大小端转换
    Temp = UnionData.ShortData[0];
    UnionData.ShortData[0] = UnionData.ShortData[1];
    UnionData.ShortData[1] = Temp;
    WriteMultipleRegister(REG_addr,UnionData.ShortData,2);

}
//--------------------------------------------------------------------------------------------------------------------------
//定时器轮询读取寄存器的数据
void MainWindow::ModbusReadAllRegister()
{
     FLAOT_UNION UnionData;
    //定时刷新Modbus数据
    switch(TimeRoundStatus)
    {
        //实时读取部分
        case 0:
        {   //0x0000~0x0017 24个：运行参数->状态信息+故障信息（uint16）仅支持03码
            VirtualRegisterAddr = 0X0000;
            VirtualRegisterReadLength = 24;
            VirtualRegisterReadFlag = true;
            ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
            break;
        }
        case 1:
        {   //0x0018~0x008A 116个：运行参数->数据信息（float）仅支持03码
            VirtualRegisterAddr = 0x0018;
            VirtualRegisterReadLength = 116;
            VirtualRegisterReadFlag = true;
            ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
            break;
        }
        case 2:
        {
//                // 仅在使用的时候读取实际值
//                //0x0A00~0x0A46 71个：用户参数（uint16）
//                VirtualRegisterAddr = 0X0a00;
//                VirtualRegisterReadLength = 71;
//                VirtualRegisterReadFlag = true;
//                ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
//                break;
            //0x0A00~0x0A20 22个：用户参数（uint16+float）//暂时不处理“网络参数设置”
            VirtualRegisterAddr = 0X0a00;
            VirtualRegisterReadLength = 34;
            VirtualRegisterReadFlag = true;
            ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
            break;
        }
        case 3:
        {
            if(ui->tabWidget_2->currentIndex() == 1)//tabWidget_2中选择第二个widget
            {
                TableWidgetDataRefresh();
            }
            break;
        }
        default:;
    }
    if(TimeRoundStatus >= 4)
        TimeRoundStatus = 0;
    else
         TimeRoundStatus++;

    //显示所有寄存器数据（HEX）
    for(int value =0;value<5000;value++)
    {
        QString str = "0x"+QString("%1").arg(VirtualRegister[value],4,16,QChar('0')).toUpper();
        ui->Tab_ModbusRegister->setItem(value,2,new QTableWidgetItem(str));
        ui->Tab_ModbusRegister->item(value,2)->setBackground(QColor(255,247,212));
    }

    //0x0018 0x008A 12个字 运行参数（float）
    for(int value =0;value<12;value++)
    {
        UnionData.ShortData[1] = VirtualRegister[0x0018+value++];
        UnionData.ShortData[0] = VirtualRegister[0x0018+value];
        //ui->Tab_ConttrolParamater->setItem(value/2,1,new QTableWidgetItem(QString::number(UnionData.FloatData ,'f',4)));
        ui->Tab_OperationParamater->setItem(value/2,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    }
//    UnionData.ShortData[1] = VirtualRegister[0x001E];
//    UnionData.ShortData[0] = VirtualRegister[0x001F];
//    ui->Tab_OperationParamater->setItem(0,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
//    UnionData.ShortData[1] = VirtualRegister[0x0020];
//    UnionData.ShortData[0] = VirtualRegister[0x0021];
//    ui->Tab_OperationParamater->setItem(0,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
//    UnionData.ShortData[1] = VirtualRegister[0x0022];
//    UnionData.ShortData[0] = VirtualRegister[0x0023];
//    ui->Tab_OperationParamater->setItem(0,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

//    UnionData.ShortData[1] = VirtualRegister[0x0018];
//    UnionData.ShortData[0] = VirtualRegister[0x0019];
//    ui->Tab_OperationParamater->setItem(0,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
//    UnionData.ShortData[1] = VirtualRegister[0x001A];
//    UnionData.ShortData[0] = VirtualRegister[0x001B];
//    ui->Tab_OperationParamater->setItem(0,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
//    UnionData.ShortData[1] = VirtualRegister[0x001C];
//    UnionData.ShortData[0] = VirtualRegister[0x001D];
//    ui->Tab_OperationParamater->setItem(0,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));


    //0X1280 0x1308 138个字 控制参数（float）
    for(int value =0;value<138;value++)
    {
        FLAOT_UNION UnionData;
        UnionData.ShortData[1] = VirtualRegister[0x1280+value++];
        UnionData.ShortData[0] = VirtualRegister[0x1280+value];
        //ui->Tab_ConttrolParamater->setItem(value/2,1,new QTableWidgetItem(QString::number(UnionData.FloatData ,'f',4)));
        ui->Tab_ConttrolParamater->setItem(value/2,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    }

    //0x0FA0~0xFC6 40个字:机型参数（float）
    for(int value =0;value<40;value++)
    {
        FLAOT_UNION UnionData;
        UnionData.ShortData[1] = VirtualRegister[0x0fa0+value++];
        UnionData.ShortData[0] = VirtualRegister[0x0fa0+value];
        ui->Tab_RatedlParamater->setItem(value/2,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    }
     //0X1220 0x127e 96个 保护参数（float）
    for(int value =0;value<96;value++)
    {//0x1278 uint32
        FLAOT_UNION UnionData;
        UnionData.ShortData[1] = VirtualRegister[0X1220+value++];
        UnionData.ShortData[0] = VirtualRegister[0X1220+value];
        if(0X1220+value == 0x1277||0X1220+value == 0x1279 )
            ui->Tab_ProtectParamater->setItem(value/2,1,new QTableWidgetItem(QString::number(UnionData.IntData )));
        else
            ui->Tab_ProtectParamater->setItem(value/2,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    }
     //0X130a  0x1326 29个 保护时间（uint16）
    for(int value =0;value<29;value++)
    {
        ui->Tab_ProtectTimers->setItem(value,1,new QTableWidgetItem(QString::number(VirtualRegister[0X130a+value])));
    }
    //0x1327~0x1345 31: 系统配置参数（uint16）
    for(int value =0;value<31;value++)
    {
        ui->Tab_SystemParamater->setItem(value,1,new QTableWidgetItem(QString::number(VirtualRegister[0x1327+value])));
    }

    //TableWidget 更新完成，
    TableBackgroundColorIint();    
    Tab_OperationParamaterCh1SetItemIint();//显示寄存器数据，改变颜色
    Tab_OperationParamaterCh2SetItemIint();
    Tab_OperationParamaterCh3SetItemIint();
}
//table内的数据分类更新，选中的模块实时更新，不选中的不更新
void MainWindow::TableWidgetDataRefresh()
{

    switch (ui->tabWidget->currentIndex()) //Table1SelectFlag模块选中的数值
    {
        case TableContrrol:
        {
            if(m_SendStepFlag == 0)//分段执行发送指令
            {
                m_SendStepFlag++;
                //0X1280 0x12e2 100个 控制参数（float）
                VirtualRegisterAddr = 0X1280;
                VirtualRegisterReadLength = 100;
                VirtualRegisterReadFlag = true;
                ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
            }
            else
               if(m_SendStepFlag == 1)
               {
                   m_SendStepFlag = 0;
                   //0X12e4 0x1308  38个 控制参数（float）
                   VirtualRegisterAddr = 0X12e4;
                   VirtualRegisterReadLength = 38;
                   VirtualRegisterReadFlag = true;
                   ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
               }
            break;
        }
        case TableRated:
        {
            //0x0FA0~0xFC6 40个:机型参数（float）
             VirtualRegisterAddr = 0X0fa0;
             VirtualRegisterReadLength = 40;
             VirtualRegisterReadFlag = true;
             ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
             break;
        }
        case TableProtect:
        {
            if(m_SendStepFlag == 0)//分段执行发送指令
            {
                 m_SendStepFlag++;
                //0X1220 0x127e 96个 保护参数（float）
                VirtualRegisterAddr = 0X1220;
                VirtualRegisterReadLength = 96;
                VirtualRegisterReadFlag = true;
                ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
            }
            else
                if(m_SendStepFlag == 1)
                {
                    m_SendStepFlag = 0;
                    //0X130a  0x1326 29个 保护时间（uint16）
                    VirtualRegisterAddr = 0X130a;
                    VirtualRegisterReadLength = 29;
                    VirtualRegisterReadFlag = true;
                    ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);

                }
            break;
        }
        case TableSystem:
        {
            //0x1327~0x1345 31: 系统配置参数
            VirtualRegisterAddr = 0x1327;
            VirtualRegisterReadLength = 31;
            VirtualRegisterReadFlag = true;
            ReadHeldingRegister(VirtualRegisterAddr,VirtualRegisterReadLength);
            break;
        }
        case TableCorrect:
        {
            break;
        }
        default:break;
    }
}
//--------------------------------------------------------------------------------------------------------------------------
//Modbus返回数据分析--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------
void MainWindow::ModbusReturnDataProcess(QByteArray BA)
{
    QString Str;
    qint16 LEN = 0;
    quint8 ReturnByteCnt = 0;
    Int_UNION CharToInt;

    ByteToHexString(Str,BA);
    //在显示编辑区域显示
    ui->data_group->appendPlainText("接收："+Str);//不用settext 这样会覆盖之前的消息

    LEN =  BA.size();
    ReturnByteCnt = BA.at(8);//获取Modbus返回数据的字节数

    ui->data_group->appendPlainText(QString::number(LEN));

    if((BA.size() >= 12) && (VirtualRegisterReadFlag == true)) //接收数据长度大于12  && 发送标志判断
    {
        VirtualRegisterReadFlag = false;
        if(BA.at(6)== ModbusTcpID && (BA.at(7) == 3 )) // ID号&& 03码判断
        {
           if(ReturnByteCnt == VirtualRegisterReadLength*2) //存储长度是否位当前发送的长度，对应发送的地址
           {
//               qDebug()<<VirtualRegisterAddr;
//               qDebug()<<VirtualRegisterReadLength;
//               qDebug()<<BA;
               for(qint16 i = 0;i<VirtualRegisterReadLength;i++)//循环提取数据缓存到虚拟存储中VirtualRegister[].
               {
                   CharToInt.CharData[1] =BA.at(9+i*2);
                   CharToInt.CharData[0] =BA.at(10+i*2);
                   VirtualRegister[VirtualRegisterAddr+i]  = CharToInt.IntData;
               }
           }
        }
    }
    else
        if((BA.size() == 9))
        {
            if(ui->mDebug->isChecked() == true)
                QMessageBox::information(this,"提示","发送不合法");
        }
}


//故障信息记录，通过对比历史和当前的故障信息，判断当前那个报警信息被立起，即报警上升延，显示当前报警。
void MainWindow::SystemDataShow()
{
     QString str;
     quint16 ProcessDefault =0;

    //运行数据中的状态信息显示
    UnionDataBin Value ;
    Value.All = VirtualRegister[0];
    if(Value.Bin.bit0 == 1) //bit0 待机状态
    {
        ui->OpenDeviceButton->setIcon(QIcon("://images/open.png"));
        ui->OpenDeviceButton->setIconSize(QSize(25,25));
    }
    else
    {
        ui->OpenDeviceButton->setIcon(QIcon("://images/close.png"));
        ui->OpenDeviceButton->setIconSize(QSize(25,25));
    }

    if(Value.Bin.bit5 == 1) //运行
    {
        ui->RunEButton->setIcon(QIcon("://images/run.ico"));
        ui->RunEButton->setIconSize(QSize(25,25));
    }
    else
    {
        ui->RunEButton->setIcon(QIcon("://images/stopB.png"));
        ui->RunEButton->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit6 == 1) //告警
    {
        ui->AlmEButton->setIcon(QIcon("://images/alm.ico"));
        ui->AlmEButton->setIconSize(QSize(25,25));
    }
    else
    {
        ui->AlmEButton->setIcon(QIcon("://images/noalm.png"));
        ui->AlmEButton->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit7 == 1) //故障
    {
        ui->ErrEButton->setIcon(QIcon("://images/err.ico"));
        ui->ErrEButton->setIconSize(QSize(25,25));
    }
    else
    {
        ui->ErrEButton->setIcon(QIcon("://images/noerr.png"));
        ui->ErrEButton->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit15 == 1) //急停
    {
        ui->EmeStopButton->setIcon(QIcon("://images/jstop.png"));
        ui->EmeStopButton->setIconSize(QSize(25,25));
    }
    else
    {
        ui->EmeStopButton->setIcon(QIcon("://images/nojstop.png"));
        ui->EmeStopButton->setIconSize(QSize(25,25));
    }



    Value.All = VirtualRegister[4]; //20kw模块1运行状态
    if(Value.Bin.bit0 == 1) //bit0 待机状态
    {
        ui->Open_CloseButton_1->setIcon(QIcon("://images/open.png"));
        ui->Open_CloseButton_1->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Open_CloseButton_1->setIcon(QIcon("://images/close.png"));
        ui->Open_CloseButton_1->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit1 == 1) //bit1 运行状态
    {
        ui->Run_StopButton_1->setIcon(QIcon("://images/run.ico"));
        ui->Run_StopButton_1->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Run_StopButton_1->setIcon(QIcon("://images/stopB.png"));
        ui->Run_StopButton_1->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit2 == 1) //告警
    {
        ui->Err1Button_1->setIcon(QIcon("://images/alm.ico"));
        ui->Err1Button_1->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Err1Button_1->setIcon(QIcon("://images/noalm.png"));
        ui->Err1Button_1->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit3 == 1) //故障
    {
        ui->Err2Button_1->setIcon(QIcon("://images/err.ico"));
        ui->Err2Button_1->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Err2Button_1->setIcon(QIcon("://images/noerr.png"));
        ui->Err2Button_1->setIconSize(QSize(25,25));
    }

    Value.All = VirtualRegister[5]; //25kw模块2运行状态
    if(Value.Bin.bit0 == 1) //bit0 待机状态
    {
        ui->Open_CloseButton_2->setIcon(QIcon("://images/open.png"));
        ui->Open_CloseButton_2->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Open_CloseButton_2->setIcon(QIcon("://images/close.png"));
        ui->Open_CloseButton_2->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit1 == 1) //bit1 运行状态
    {
        ui->Run_StopButton_2->setIcon(QIcon("://images/run.ico"));
        ui->Run_StopButton_2->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Run_StopButton_2->setIcon(QIcon("://images/stopB.png"));
        ui->Run_StopButton_2->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit2 == 1) //告警
    {
        ui->Err1Button_2->setIcon(QIcon("://images/alm.ico"));
        ui->Err1Button_2->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Err1Button_2->setIcon(QIcon("://images/noalm.png"));
        ui->Err1Button_2->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit3 == 1) //故障
    {
        ui->Err2Button_2->setIcon(QIcon("://images/err.ico"));
        ui->Err2Button_2->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Err2Button_2->setIcon(QIcon("://images/noerr.png"));
        ui->Err2Button_2->setIconSize(QSize(25,25));
    }

    Value.All = VirtualRegister[6]; //20kw模块3运行状态
    if(Value.Bin.bit0 == 1) //bit0 待机状态
    {
        ui->Open_CloseButton_3->setIcon(QIcon("://images/open.png"));
        ui->Open_CloseButton_3->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Open_CloseButton_3->setIcon(QIcon("://images/close.png"));
        ui->Open_CloseButton_3->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit1 == 1) //bit1 运行状态
    {
        ui->Run_StopButton_3->setIcon(QIcon("://images/run.ico"));
        ui->Run_StopButton_3->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Run_StopButton_3->setIcon(QIcon("://images/stopB.png"));
        ui->Run_StopButton_3->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit2 == 1) //告警
    {
        ui->Err1Button_3->setIcon(QIcon("://images/alm.ico"));
        ui->Err1Button_3->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Err1Button_3->setIcon(QIcon("://images/noalm.png"));
        ui->Err1Button_3->setIconSize(QSize(25,25));
    }
    if(Value.Bin.bit3 == 1) //故障
    {
        ui->Err2Button_3->setIcon(QIcon("://images/err.ico"));
        ui->Err2Button_3->setIconSize(QSize(25,25));
    }
    else
    {
        ui->Err2Button_3->setIcon(QIcon("://images/noerr.png"));
        ui->Err2Button_3->setIconSize(QSize(25,25));
    }


    switch (VirtualRegister[0x0a00]) {
    case 1:
        ui->atc_mode->setText("恒压");
        break;
    case 2:
        ui->atc_mode->setText("恒流");
        break;
    case 3:
        ui->atc_mode->setText("恒功率");
        break;
    case 4:
        ui->atc_mode->setText("MPPT");
        break;
    case 5:
        ui->atc_mode->setText("恒阻");
        break;
    default:
        ui->atc_mode->setText("   ");
        break;
    }

//显示故障信息#60KW------------------------------------------------------------------------------------------------------
     //获取故障寄存器数据
     SystemFaulCurrentBuff[0] = VirtualRegister[9];
     SystemFaulCurrentBuff[1] = VirtualRegister[10];
     SystemFaulCurrentBuff[2] = VirtualRegister[11];
     //故障1   记录当前故障数据
     ProcessDefault = SystemFaulCurrentBuff[0] ^ SystemFaulHistroyBuff[0] & SystemFaulCurrentBuff[0];
     if(ProcessDefault > 0)//异或处理 状态存在变化则执行以下操作
     {
         str = DataProcess->SystemFaultAnalysis1(ProcessDefault,true);
         ui->EditText_FaultGroup->appendPlainText(str);
     }
     //故障2  记录当前故障数据
     ProcessDefault = SystemFaulCurrentBuff[1] ^ SystemFaulHistroyBuff[1] & SystemFaulCurrentBuff[1];
     if(ProcessDefault > 0)//异或处理 状态存在变化则执行以下操作
     {
         str = DataProcess->SystemFaultAnalysis2(ProcessDefault,true);
         ui->EditText_FaultGroup->appendPlainText(str);
     }
     //故障3  记录当前故障数据
     ProcessDefault = SystemFaulCurrentBuff[2] ^ SystemFaulHistroyBuff[2] & SystemFaulCurrentBuff[2];
     if(ProcessDefault > 0)//异或处理 状态存在变化则执行以下操作
     {
         str = DataProcess->SystemFaultAnalysis3(ProcessDefault,true);
         ui->EditText_FaultGroup->appendPlainText(str);
     }
     //缓存当前故障数据
     SystemFaulHistroyBuff[0] = SystemFaulCurrentBuff[0];
     SystemFaulHistroyBuff[1] = SystemFaulCurrentBuff[1];
     SystemFaulHistroyBuff[2] = SystemFaulCurrentBuff[2];

     //整机故障显示（未完成）
     //模块故障显示
     ModuleFaultShow();
}


//--------------------------------------------------------------------------------------------------------------------------
//-------开关机ACTION---------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------
//往0X0900寄存器里面写入指令
void MainWindow::on_atc_takeon_triggered()
{
    quint16 data = 0x00F1;
    m_ModbusSendStack(0x0900,data);
}

void MainWindow::on_atc_takeoff_triggered()
{
    quint16 data = 0x00F2;
    m_ModbusSendStack(0x0900,data);
}

void MainWindow::on_atc_rst_triggered()
{
    quint16 data = 0x00F5;
    m_ModbusSendStack(0x0900,data);
}

void MainWindow::on_atc_run_triggered()
{
    quint16 data = 0x00F3;
    m_ModbusSendStack(0x0900,data);
}

void MainWindow::on_atc_stop_triggered()
{
    quint16 data = 0x00F4;
    m_ModbusSendStack(0x0900,data);
}
//--------------------------------------------------------------------------------------------------------------------------
//参数设置BTN---------------------------------------------------------------------------------------------------------
//-模式选择-------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_Com_RunMode_currentIndexChanged(int index)
{
    //全局变量赋值 赋值运行模式
    OutParameterMode = index+1;
    //根据运行模式，显示运行设置参数
    Com_RunSetDataShow(OutParameterMode);
}
//显示数据刷新
void MainWindow::Com_RunSetDataShow(int value)
{
    FLAOT_UNION UnionData;
    switch (value) {
    case 1:{
        ui->label_OutName1->setText("电压给定");
        ui->label_OutName2->setText("电压步长");
        ui->label_OutName3->setText("内阻设置");
        ui->label_OutUnit1->setText("V");
        ui->label_OutUnit2->setText("V/ms");
        ui->label_OutUnit3->setText("mΩ");
        ui->label_OutName3->setVisible(true);
        ui->label_OutUnit3->setVisible(true);
        ui->Edit_OutData3->setVisible(true);

        UnionData.ShortData[1] = VirtualRegister[0x0A04];
        UnionData.ShortData[0] = VirtualRegister[0x0A05];
        ui->Edit_OutData1->setText(QString::number(UnionData.FloatData));
        UnionData.ShortData[1] = VirtualRegister[0x0A0A];
        UnionData.ShortData[0] = VirtualRegister[0x0A0B];
        ui->Edit_OutData2->setText(QString::number(UnionData.FloatData));
        UnionData.ShortData[1] = VirtualRegister[0x0A12];
        UnionData.ShortData[0] = VirtualRegister[0x0A13];
        ui->Edit_OutData3->setText(QString::number(UnionData.FloatData));
        break;
    }
    case 2:{
        ui->label_OutName1->setText("电流给定");
        ui->label_OutName2->setText("电流步长");
        ui->label_OutUnit1->setText("A");
        ui->label_OutUnit2->setText("A/ms");
        ui->label_OutName3->setVisible(false);
        ui->label_OutUnit3->setVisible(false);
        ui->Edit_OutData3->setVisible(false);
        UnionData.ShortData[1] = VirtualRegister[0x0A06];
        UnionData.ShortData[0] = VirtualRegister[0x0A07];
        ui->Edit_OutData1->setText(QString::number(UnionData.FloatData));
        UnionData.ShortData[1] = VirtualRegister[0x0A0C];
        UnionData.ShortData[0] = VirtualRegister[0x0A0D];
        ui->Edit_OutData2->setText(QString::number(UnionData.FloatData));
        break;
    }
    case 3:{
        ui->label_OutName1->setText("功率给定");
        ui->label_OutName2->setText("功率步长");
        ui->label_OutUnit1->setText("W");
        ui->label_OutUnit2->setText("W/ms");
        ui->label_OutName3->setVisible(false);
        ui->label_OutUnit3->setVisible(false);
        ui->Edit_OutData3->setVisible(false);
        UnionData.ShortData[1] = VirtualRegister[0x0A08];
        UnionData.ShortData[0] = VirtualRegister[0x0A09];
        ui->Edit_OutData1->setText(QString::number(UnionData.FloatData));
        UnionData.ShortData[1] = VirtualRegister[0x0A0E];
        UnionData.ShortData[0] = VirtualRegister[0x0A0F];
        ui->Edit_OutData2->setText(QString::number(UnionData.FloatData));
        break;
    }
    case 4:{
        ui->label_OutName1->setText("MPPT");
        ui->label_OutName2->setText("MPPT");
        ui->label_OutUnit1->setText("V");
        ui->label_OutUnit2->setText("V/ms");
        ui->label_OutName3->setVisible(false);
        ui->label_OutUnit3->setVisible(false);
        ui->Edit_OutData3->setVisible(false);
        ui->Edit_OutData1->setText("0");
        ui->Edit_OutData2->setText("0");
        break;
    }
    case 5:{//未使用
        ui->label_OutName1->setText("阻抗给定");
        ui->label_OutName2->setText("阻抗步长");
        ui->label_OutUnit1->setText("Ω");
        ui->label_OutUnit2->setText("Ω/ms");
        ui->label_OutName3->setVisible(false);
        ui->label_OutUnit3->setVisible(false);
        ui->Edit_OutData3->setVisible(false);
        UnionData.ShortData[1] = VirtualRegister[0x0A10];
        UnionData.ShortData[0] = VirtualRegister[0x0A11];
        ui->Edit_OutData1->setText(QString::number(UnionData.FloatData));
        UnionData.ShortData[1] = VirtualRegister[0x0A14];
        UnionData.ShortData[0] = VirtualRegister[0x0A15];
        ui->Edit_OutData2->setText(QString::number(UnionData.FloatData));
        break;
    }
    default: break;
    }
}
void MainWindow::OutParameterSet()
{
    float ParamaterData1 = 0;
    float ParamaterData2 = 0;
    float ParamaterData3 = 0;
    //切换运行模式
    m_ModbusSendStack(0x0A00,(quint16)(OutParameterMode));
    //根据运行模式选择 参数对应的地址发送
    switch (OutParameterMode) {
    case 1:{
        ParamaterData1 = ui->Edit_OutData1->text().toFloat();//电压设置
        ParamaterData2 = ui->Edit_OutData2->text().toFloat();//电压调节步长
        ParamaterData3 = ui->Edit_OutData3->text().toFloat();//内阻设置
        m_ModbusSendStack(0x0A04,ParamaterData1);
        m_ModbusSendStack(0x0A0A,ParamaterData2);
        m_ModbusSendStack(0x0A12,ParamaterData3);
        break;
    }
    case 2:{
        ParamaterData1 = ui->Edit_OutData1->text().toFloat();//电流设置
        ParamaterData2 = ui->Edit_OutData2->text().toFloat();//电流调节步长
        m_ModbusSendStack(0x0A06,ParamaterData1);
        m_ModbusSendStack(0x0A0C,ParamaterData2);
        break;
    }
    case 3:{
        ParamaterData1 = ui->Edit_OutData1->text().toFloat();//功率设置
        ParamaterData2 = ui->Edit_OutData2->text().toFloat();//功率调节步长
        m_ModbusSendStack(0x0A08,ParamaterData1);
        m_ModbusSendStack(0x0A0E,ParamaterData2);
        break;
    }
    case 4:{//未使用
        QMessageBox::information(this,"提示","预留功能");
        break;
    }
    case 5:{
        ParamaterData1 = ui->Edit_OutData1->text().toFloat();//阻抗设置
        ParamaterData2 = ui->Edit_OutData2->text().toFloat();//阻抗调节步长
        m_ModbusSendStack(0x0A10,ParamaterData1);
        m_ModbusSendStack(0x0A14,ParamaterData2);
        break;
    }
    default:
        break;
    }
    //定时器刷新运行参数设置，执行三次，
    Com_RunSetRefreshFlag = 25;
}
//发送输出参数，获取写入数据，选择输出指令地址
void MainWindow::on_Btn_OutParameterSet_clicked()
{    
   OutParameterSet();
}

//输出设置 回车操作
void MainWindow::on_Edit_OutData1_returnPressed()
{
    OutParameterSet();
}

void MainWindow::on_Edit_OutData2_returnPressed()
{
    OutParameterSet();
}

void MainWindow::on_Edit_OutData3_returnPressed()
{
    OutParameterSet();
}

//Table双击触发：根据每个table触发发送子窗体，给子窗体赋地址和初始数据，
void MainWindow::on_Tab_ConttrolParamater_cellDoubleClicked(int row, int column)
{
    //显示子窗体，给子窗体控件赋值
    if(column == 1)
    {
        TableSetFloatIntFlag = 1;
        m_TableSendView->show();
        m_TableSendView->ui->Edit_TableSendAddr->setText(QString::number(0x1280+row*2));

        QString str =  ui->Tab_ConttrolParamater->item(row,column)->text();

        m_TableSendView->ui->Edit_TableSendData->setText(str);
    }
}
void MainWindow::on_Tab_RatedlParamater_cellDoubleClicked(int row, int column)
{
    if(column == 1)
    {
        TableSetFloatIntFlag = 1;
        m_TableSendView->show();
        m_TableSendView->ui->Edit_TableSendAddr->setText(QString::number(0x0FA0+row*2));
        m_TableSendView->ui->Edit_TableSendData->setText("0");
    }
}

void MainWindow::on_Tab_ProtectParamater_cellDoubleClicked(int row, int column)
{
    if(column == 1)
    {
        TableSetFloatIntFlag = 1;
        m_TableSendView->show();
        m_TableSendView->ui->Edit_TableSendAddr->setText(QString::number(0x1220+row*2));
        m_TableSendView->ui->Edit_TableSendData->setText("0");
    }

}

void MainWindow::on_Tab_ProtectTimers_cellDoubleClicked(int row, int column)
{
    if(column == 1)
    {
        TableSetFloatIntFlag = 0;
        m_TableSendView->show();
        m_TableSendView->ui->Edit_TableSendAddr->setText(QString::number(0x130a+row));
        m_TableSendView->ui->Edit_TableSendData->setText("0");
    }
}
void MainWindow::on_Tab_SystemParamater_cellDoubleClicked(int row, int column)
{
    if(column == 1)
    {
        TableSetFloatIntFlag = 0;
        m_TableSendView->show();
        m_TableSendView->ui->Edit_TableSendAddr->setText(QString::number(0x1327+row));
        m_TableSendView->ui->Edit_TableSendData->setText("0");
    }
}
//子窗口触发发送函数，判断发送位置位 float OR uint，选择发送函数
void MainWindow::m_SendForm()
{
    if(TableSetFloatIntFlag == 1) //发送float类型参数
        m_ModbusSendStack(m_TableSendView->ui->Edit_TableSendAddr->text().toUInt(),(float)m_TableSendView->ui->Edit_TableSendData->text().toFloat());
    else //发送int型类型参数
        m_ModbusSendStack(m_TableSendView->ui->Edit_TableSendAddr->text().toUInt(),(quint16)m_TableSendView->ui->Edit_TableSendData->text().toUInt());
}
//窗口退出事件
void MainWindow::closeEvent(QCloseEvent *e)
{
    //关闭窗口后关闭TCP，防止程序异常退出
    m_TcpChile->disconnectFromHost();
    //执行退出信号
    e->accept();
}

//函数必须设置在 Item 设置完成后 负责会报错
void MainWindow::TableBackgroundColorIint()
{
    for(quint8 val =0;val < 69;val++)
    {
        if(val < 5)
        {
            ui->Tab_ConttrolParamater->item(val,0)->setBackground(QColor(202,250,255));
            ui->Tab_ConttrolParamater->item(val,1)->setBackground(QColor(202,250,255));
        }

        else if(val < 25)
        {
            ui->Tab_ConttrolParamater->item(val,0)->setBackground(QColor(255,206,196));
            ui->Tab_ConttrolParamater->item(val,1)->setBackground(QColor(255,206,196));
        }
        else  if(val < 45)
        {
            ui->Tab_ConttrolParamater->item(val,0)->setBackground(QColor(193,255,193));
            ui->Tab_ConttrolParamater->item(val,1)->setBackground(QColor(193,255,193));
        }
        else if(val < 65)
        {
            ui->Tab_ConttrolParamater->item(val,0)->setBackground(QColor(255,196,237));
            ui->Tab_ConttrolParamater->item(val,1)->setBackground(QColor(255,196,237));
        }
        else
        {
            ui->Tab_ConttrolParamater->item(val,0)->setBackground(QColor(255,247,212));
            ui->Tab_ConttrolParamater->item(val,1)->setBackground(QColor(255,247,212));
        }
    }

    for(quint8 val =0;val < 6;val++)
    {
        ui->Tab_OperationParamater->item(val,0)->setBackground(QColor(202,250,255));
        ui->Tab_OperationParamater->item(val,1)->setBackground(QColor(202,250,255));
    }
}

//table列表初始化：初始化命名、间距 颜色
void MainWindow::Tab_OperationParamaterCh1ItemIint()
{
     quint16 i =0;
    //设置Table的列间距
    ui->Tab_OperationParamaterCh1->setColumnWidth(0,200);
    ui->Tab_OperationParamaterCh1->setColumnWidth(1,100);
    //设置table初始值
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("A端口电压"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("A母线电压"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("A电流"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("A功率"));

    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("电感电流1"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("电感电流2"));

    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("B端口电压"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("B电压"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("B电流"));    
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("B功率"));

    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("A半母线电压1"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("A半母线电压2"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("B半母线电压1"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("B半母线电压2"));

//    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("模块A端电容电压差"));

//    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("模块B端电容电压差"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("散热器温度"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("运行状态"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("故障状态1"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("故障状态2"));
    ui->Tab_OperationParamaterCh1->setItem(i++,0,new QTableWidgetItem("故障状态3"));
    i=0;
    for(quint8 val =0;val < 19;val++)
    {
        ui->Tab_OperationParamaterCh1->item(val,0)->setBackground(QColor(255,206,196));
    }

    //设置Table的列间距
    ui->Tab_OperationParamaterCh2->setColumnWidth(0,200);
    ui->Tab_OperationParamaterCh2->setColumnWidth(1,100);
    //设置table初始值
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("A端口电压"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("A母线电压"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("A电流"));    
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("A功率"));

    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("电感电流1"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("电感电流2"));

    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("B端口电压"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("B电压"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("B端电流"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("B功率"));

    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("A半母线电压1"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("A半母线电压2"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("B半母线电压1"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("B半母线电压2"));

//    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("模块A端电容电压差"));
//    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("模块B端电容电压差"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("散热器温度"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("运行状态"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("故障状态1"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("故障状态2"));
    ui->Tab_OperationParamaterCh2->setItem(i++,0,new QTableWidgetItem("故障状态3"));
    i=0;
    for(quint8 val =0;val < 19;val++)
    {
        ui->Tab_OperationParamaterCh2->item(val,0)->setBackground(QColor(193,255,193));
    }

    //设置Table的列间距
    ui->Tab_OperationParamaterCh3->setColumnWidth(0,200);
    ui->Tab_OperationParamaterCh3->setColumnWidth(1,100);
    //设置table初始值
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("A端口电压"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("A母线电压"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("A电流"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("A功率"));

    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("电感电流1"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("电感电流2"));

    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("B端口电压"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("B电压"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("B电流"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("B功率"));

    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("A半母线电压1"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("A半母线电压2"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("B半母线电压1"));
    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("B半母线电压2"));

//    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("模块A端电容电压差"));
//    ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("模块B端电容电压差"));
     ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("散热器温度"));
     ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("运行状态"));
     ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("故障状态1"));
     ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("故障状态2"));
     ui->Tab_OperationParamaterCh3->setItem(i++,0,new QTableWidgetItem("故障状态3"));
    i=0;
    for(quint8 val =0;val < 19;val++)
    {
        ui->Tab_OperationParamaterCh3->item(val,0)->setBackground(QColor(255,196,237));
    }
}
void MainWindow::Tab_OperationParamaterCh1SetItemIint()
{
    //Tab_OperationParamaterCh1 Item赋值
    FLAOT_UNION UnionData;
    quint16 value =0;
    UnionData.ShortData[1] = VirtualRegister[0x0024];
    UnionData.ShortData[0] = VirtualRegister[0x0024+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0026];
    UnionData.ShortData[0] = VirtualRegister[0x0026+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0028];
    UnionData.ShortData[0] = VirtualRegister[0x0028+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));   
    UnionData.ShortData[1] = VirtualRegister[0x0034];
    UnionData.ShortData[0] = VirtualRegister[0x0034+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    //电感电流1、2
    UnionData.ShortData[1] = VirtualRegister[0x002a];
    UnionData.ShortData[0] = VirtualRegister[0x002a+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x002c];
    UnionData.ShortData[0] = VirtualRegister[0x002c+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    UnionData.ShortData[1] = VirtualRegister[0x002e];
    UnionData.ShortData[0] = VirtualRegister[0x002f];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0030];
    UnionData.ShortData[0] = VirtualRegister[0x0030+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0032];
    UnionData.ShortData[0] = VirtualRegister[0x0032+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));    

    UnionData.ShortData[1] = VirtualRegister[0x0036];
    UnionData.ShortData[0] = VirtualRegister[0x0036+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    UnionData.ShortData[1] = VirtualRegister[0x0060];
    UnionData.ShortData[0] = VirtualRegister[0x0060+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0062];
    UnionData.ShortData[0] = VirtualRegister[0x0062+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0064];
    UnionData.ShortData[0] = VirtualRegister[0x0064+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData) ));
    UnionData.ShortData[1] = VirtualRegister[0x0066];
    UnionData.ShortData[0] = VirtualRegister[0x0066+1];
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

//    UnionData.ShortData[1] = VirtualRegister[0x0078];
//    UnionData.ShortData[0] = VirtualRegister[0x0078+1];
//    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
//    UnionData.ShortData[1] = VirtualRegister[0x007a];
//    UnionData.ShortData[0] = VirtualRegister[0x007a+1];
//    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    //散热器温度
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0015] )));
    //运行状态
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0004] )));
    //故障信息1
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x000c] )));
    //故障信息2
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x000d] )));
    //故障信息3
    ui->Tab_OperationParamaterCh1->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x000e] )));


    //只有table Item有初始值的情况下才能使用
    for(quint8 val =0;val < 19;val++)
    {
        ui->Tab_OperationParamaterCh1->item(val,1)->setBackground(QColor(255,206,196));
    }
}

void MainWindow::Tab_OperationParamaterCh2SetItemIint()
{
    //Tab_OperationParamaterCh1 Item赋值
    FLAOT_UNION UnionData;
    quint16 value =0;
    UnionData.ShortData[1] = VirtualRegister[0x0038];
    UnionData.ShortData[0] = VirtualRegister[0x0039];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x003a];
    UnionData.ShortData[0] = VirtualRegister[0x003b];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x003c];
    UnionData.ShortData[0] = VirtualRegister[0x003d];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    //A功率
    UnionData.ShortData[1] = VirtualRegister[0x0048];
    UnionData.ShortData[0] = VirtualRegister[0x0049];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    //电感电流
    UnionData.ShortData[1] = VirtualRegister[0x003e];
    UnionData.ShortData[0] = VirtualRegister[0x003f];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0040];
    UnionData.ShortData[0] = VirtualRegister[0x0041];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));


    UnionData.ShortData[1] = VirtualRegister[0x0042];
    UnionData.ShortData[0] = VirtualRegister[0x0043];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0044];
    UnionData.ShortData[0] = VirtualRegister[0x0045];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0046];
    UnionData.ShortData[0] = VirtualRegister[0x0049];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
   //B功率
    UnionData.ShortData[1] = VirtualRegister[0x004a];
    UnionData.ShortData[0] = VirtualRegister[0x004b];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    UnionData.ShortData[1] = VirtualRegister[0x0068];
    UnionData.ShortData[0] = VirtualRegister[0x0069];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x006a];
    UnionData.ShortData[0] = VirtualRegister[0x006b];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x006c];
    UnionData.ShortData[0] = VirtualRegister[0x006d];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData) ));
    UnionData.ShortData[1] = VirtualRegister[0x006e];
    UnionData.ShortData[0] = VirtualRegister[0x006f];
    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

//    UnionData.ShortData[1] = VirtualRegister[0x007c];
//    UnionData.ShortData[0] = VirtualRegister[0x007d];
//    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
//    UnionData.ShortData[1] = VirtualRegister[0x007e];
//    UnionData.ShortData[0] = VirtualRegister[0x007f];
//    ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
     ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0016] )));
     //运行状态
     ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0005] )));
     //故障信息1
     ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x000f] )));
     //故障信息2
     ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0010] )));
     //故障信息3
     ui->Tab_OperationParamaterCh2->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0011] )));


    //只有table Item有初始值的情况下才能使用
    for(quint8 val =0;val < 19;val++)
    {
        ui->Tab_OperationParamaterCh2->item(val,1)->setBackground(QColor(193,255,193));
    }
}

void MainWindow::Tab_OperationParamaterCh3SetItemIint()
{
    //Tab_OperationParamaterCh1 Item赋值
    FLAOT_UNION UnionData;
    quint16 value =0;
    UnionData.ShortData[1] = VirtualRegister[0x004c];
    UnionData.ShortData[0] = VirtualRegister[0x004d];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x004e];
    UnionData.ShortData[0] = VirtualRegister[0x004f];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0050];
    UnionData.ShortData[0] = VirtualRegister[0x0051];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    //A功率
    UnionData.ShortData[1] = VirtualRegister[0x005c];
    UnionData.ShortData[0] = VirtualRegister[0x005d];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    //电感电流
    UnionData.ShortData[1] = VirtualRegister[0x0052];
    UnionData.ShortData[0] = VirtualRegister[0x0053];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0054];
    UnionData.ShortData[0] = VirtualRegister[0x0055];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    UnionData.ShortData[1] = VirtualRegister[0x0056];
    UnionData.ShortData[0] = VirtualRegister[0x0057];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0058];
    UnionData.ShortData[0] = VirtualRegister[0x0059];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x005a];
    UnionData.ShortData[0] = VirtualRegister[0x005b];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));

    //B功率
    UnionData.ShortData[1] = VirtualRegister[0x005e];
    UnionData.ShortData[0] = VirtualRegister[0x005f];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));


    UnionData.ShortData[1] = VirtualRegister[0x0070];
    UnionData.ShortData[0] = VirtualRegister[0x0071];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0072];
    UnionData.ShortData[0] = VirtualRegister[0x0073];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0074];
    UnionData.ShortData[0] = VirtualRegister[0x0075];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    UnionData.ShortData[1] = VirtualRegister[0x0076];
    UnionData.ShortData[0] = VirtualRegister[0x0077];
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData) ));

//    UnionData.ShortData[1] = VirtualRegister[0x0080];
//    UnionData.ShortData[0] = VirtualRegister[0x0081];
//    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
//    UnionData.ShortData[1] = VirtualRegister[0x0082];
//    UnionData.ShortData[0] = VirtualRegister[0x0083];
//    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(UnionData.FloatData )));
    //散热器温度
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0017] )));
    //运行状态
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0006] )));
    //故障信息1
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0012] )));
    //故障信息2
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0013] )));
    //故障信息3
    ui->Tab_OperationParamaterCh3->setItem(value++,1,new QTableWidgetItem(QString::number(VirtualRegister[0x0014] )));

    //只有table Item有初始值的情况下才能使用
    for(quint8 val =0;val < 19;val++)
    {
        ui->Tab_OperationParamaterCh3->item(val,1)->setBackground(QColor(255,196,237));
    }
}

//20KW的故障信息显示
void MainWindow::ModuleFaultShow()
{
    UnionDataBin Value1, Value2;
    //提取20KW故障信息-----------------------------------------------------------------------------------------------------------------
    switch (ModuleFaultFlag) {
    case 0:{
        Value1.All = VirtualRegister[0x000c];
        Value2.All = VirtualRegister[0x000d];
        break;
    }
    case 1:{
        Value1.All = VirtualRegister[0x000F];
        Value2.All = VirtualRegister[0x0010];
        break;
    }
    case 2:{
        Value1.All = VirtualRegister[0x0012];
        Value2.All = VirtualRegister[0x0013];
        break;
    }
    default:
        break;
    }

    //故障信息1---------------------------------------------------------------------------------------------
    if(Value1.Bin.bit0 == 1)
    {
        ui->Check_Fault1_0->setChecked(true);
        ui->Check_Fault1_0->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_0->setChecked(false);
        ui->Check_Fault1_0->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit1 == 1)
    {
        ui->Check_Fault1_1->setChecked(true);
        ui->Check_Fault1_1->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_1->setChecked(false);
        ui->Check_Fault1_1->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit2 == 1)
    {
        ui->Check_Fault1_2->setChecked(true);
        ui->Check_Fault1_2->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_2->setChecked(false);
        ui->Check_Fault1_2->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit3 == 1)
    {
        ui->Check_Fault1_3->setChecked(true);
        ui->Check_Fault1_3->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_3->setChecked(false);
        ui->Check_Fault1_3->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit4 == 1)
    {
        ui->Check_Fault1_4->setChecked(true);
        ui->Check_Fault1_4->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_4->setChecked(false);
        ui->Check_Fault1_4->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit5 == 1)
    {
        ui->Check_Fault1_5->setChecked(true);
        ui->Check_Fault1_5->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_5->setChecked(false);
        ui->Check_Fault1_5->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit6 == 1)
    {
        ui->Check_Fault1_6->setChecked(true);
        ui->Check_Fault1_6->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_6->setChecked(false);
        ui->Check_Fault1_6->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit7 == 1)
    {
        ui->Check_Fault1_7->setChecked(true);
        ui->Check_Fault1_7->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_7->setChecked(false);
        ui->Check_Fault1_7->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit8 == 1)
    {
        ui->Check_Fault1_8->setChecked(true);
        ui->Check_Fault1_8->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_8->setChecked(false);
        ui->Check_Fault1_8->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit9 == 1)
    {
        ui->Check_Fault1_9->setChecked(true);
        ui->Check_Fault1_9->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_9->setChecked(false);
        ui->Check_Fault1_9->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit10 == 1)
    {
        ui->Check_Fault1_10->setChecked(true);
        ui->Check_Fault1_10->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_10->setChecked(false);
        ui->Check_Fault1_10->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit11 == 1)
    {
        ui->Check_Fault1_11->setChecked(true);
        ui->Check_Fault1_11->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_11->setChecked(false);
        ui->Check_Fault1_11->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit12 == 1)
    {
        ui->Check_Fault1_12->setChecked(true);
        ui->Check_Fault1_12->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_12->setChecked(false);
        ui->Check_Fault1_12->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit13 == 1)
    {
        ui->Check_Fault1_13->setChecked(true);
        ui->Check_Fault1_13->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_13->setChecked(false);
        ui->Check_Fault1_13->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit14 == 1)
    {
        ui->Check_Fault1_14->setChecked(true);
        ui->Check_Fault1_14->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_14->setChecked(false);
        ui->Check_Fault1_14->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value1.Bin.bit15 == 1)
    {
        ui->Check_Fault1_15->setChecked(true);
        ui->Check_Fault1_15->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault1_15->setChecked(false);
        ui->Check_Fault1_15->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
   //故障信息2---------------------------------------------------------------------------------------------
    if(Value2.Bin.bit0 == 1)
    {
        ui->Check_Fault2_0->setChecked(true);
        ui->Check_Fault2_0->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_0->setChecked(false);
        ui->Check_Fault2_0->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit1 == 1)
    {
        ui->Check_Fault2_1->setChecked(true);
        ui->Check_Fault2_1->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_1->setChecked(false);
        ui->Check_Fault2_1->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit2 == 1)
    {
        ui->Check_Fault2_2->setChecked(true);
        ui->Check_Fault2_2->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_2->setChecked(false);
        ui->Check_Fault2_2->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit3 == 1)
    {
        ui->Check_Fault2_3->setChecked(true);
        ui->Check_Fault2_3->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_3->setChecked(false);
        ui->Check_Fault2_3->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit4 == 1)
    {
        ui->Check_Fault2_4->setChecked(true);
        ui->Check_Fault2_4->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_4->setChecked(false);
        ui->Check_Fault2_4->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit5 == 1)
    {
        ui->Check_Fault2_5->setChecked(true);
        ui->Check_Fault2_5->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_5->setChecked(false);
        ui->Check_Fault2_5->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit6 == 1)
    {
        ui->Check_Fault2_6->setChecked(true);
        ui->Check_Fault2_6->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_6->setChecked(false);
        ui->Check_Fault2_6->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit7 == 1)
    {
        ui->Check_Fault2_7->setChecked(true);
        ui->Check_Fault2_7->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_7->setChecked(false);
        ui->Check_Fault2_7->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit8 == 1)
    {
        ui->Check_Fault2_8->setChecked(true);
        ui->Check_Fault2_8->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_8->setChecked(false);
        ui->Check_Fault2_8->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit9 == 1)
    {
        ui->Check_Fault2_9->setChecked(true);
        ui->Check_Fault2_9->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_9->setChecked(false);
        ui->Check_Fault2_9->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit10 == 1)
    {
        ui->Check_Fault2_10->setChecked(true);
        ui->Check_Fault2_10->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_10->setChecked(false);
        ui->Check_Fault2_10->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit11 == 1)
    {
        ui->Check_Fault2_11->setChecked(true);
        ui->Check_Fault2_11->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_11->setChecked(false);
        ui->Check_Fault2_11->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit12 == 1)
    {
        ui->Check_Fault2_12->setChecked(true);
        ui->Check_Fault2_12->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_12->setChecked(false);
        ui->Check_Fault2_12->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit13 == 1)
    {
        ui->Check_Fault2_13->setChecked(true);
        ui->Check_Fault2_13->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_13->setChecked(false);
        ui->Check_Fault2_13->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit14 == 1)
    {
        ui->Check_Fault2_14->setChecked(true);
        ui->Check_Fault2_14->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_14->setChecked(false);
        ui->Check_Fault2_14->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
    if(Value2.Bin.bit15 == 1)
    {
        ui->Check_Fault2_15->setChecked(true);
        ui->Check_Fault2_15->setStyleSheet("color: rgb(255, 0, 0);""font-size:20px");
    }
    else
    {
        ui->Check_Fault2_15->setChecked(false);
        ui->Check_Fault2_15->setStyleSheet("color: rgb(0, 0, 0);""font-size:20px");
    }
}


void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    ModuleFaultFlag = index;
}


void MainWindow::on_btn_SystemFaultClear_clicked()
{
      ui->EditText_FaultGroup->clear();
}


