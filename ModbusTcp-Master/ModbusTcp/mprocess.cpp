#include "mprocess.h"

//输入两个参数的故障代码，返回对应的故障信息字符串，
QString mprocess::ModuleFaultAnalysis(unsigned int Value1,unsigned int Value2)
{
    UnionDataBin mData;
    QString mstr;
    mData.All = Value1;

    if(mData.Bin.bit0 == 1)
    {
        mstr += "电感电流1软件过流\n";
    }
    if(mData.Bin.bit1 == 1)
    {
        mstr += "电感电流2软件过流\n";
    }
    if(mData.Bin.bit2 == 1)
    {
        mstr += "温度够热\n";
    }
    if(mData.Bin.bit3 == 1)
    {
        mstr += "正极绝缘检测异常\n";
    }
    if(mData.Bin.bit4 == 1)
    {
        mstr += "负极绝缘检测异常\n";
    }
    if(mData.Bin.bit5 == 1)
    {
        mstr += "输入端口电压过压\n";
    }
    if(mData.Bin.bit6 == 1)
    {
        mstr += "输入正半亩电压过压\n";
    }
    if(mData.Bin.bit7 == 1)
    {
        mstr += "输入负半亩电压过压\n";
    }
    if(mData.Bin.bit8 == 1)
    {
        mstr += "输入求和母线电压过压\n";
    }
    if(mData.Bin.bit9 == 1)
    {
        mstr += "输入正半母电压欠压\n";
    }
    if(mData.Bin.bit10 == 1)
    {
        mstr += "输入负半母电压欠压\n";
    }
    if(mData.Bin.bit11 == 1)
    {
        mstr += "输入求和母线电压欠压\n";
    }
    if(mData.Bin.bit12 == 1)
    {
        mstr += "输出端口电压过压\n";
    }
    if(mData.Bin.bit13 == 1)
    {
        mstr += "输出正半母线电压过压\n";
    }
    if(mData.Bin.bit14 == 1)
    {
        mstr += "输出负半母线电压过压\n";
    }
    if(mData.Bin.bit15 == 1)
    {
        mstr += "输出求和母线电压过压\n";
    }
    //提取第二个故障数值
    mData.All = Value2;
    if(mData.Bin.bit0 == 1)
    {
        mstr += "输出正半母线电压欠压\n";
    }
    if(mData.Bin.bit1 == 1)
    {
        mstr += "输出负半母线电压欠压\n";
    }
    if(mData.Bin.bit2 == 1)
    {
        mstr += "输出求和母线电压欠压\n";
    }
    if(mData.Bin.bit3 == 1)
    {
        mstr += "输入反接\n";
    }
    if(mData.Bin.bit4 == 1)
    {
        mstr += "输出反接\n";
    }
    if(mData.Bin.bit5 == 1)
    {
        mstr += "直流软启故障\n";
    }
    if(mData.Bin.bit6 == 1)
    {
        mstr += "直流软启故障\n";
    }
    if(mData.Bin.bit7 == 1)
    {
        mstr += "A继电器1故障\n";
    }
    if(mData.Bin.bit8 == 1)
    {
        mstr += "B继电器故障\n";
    }
    if(mData.Bin.bit9 == 1)
    {
        mstr += "母线电流过流\n";
    }
    if(mData.Bin.bit10 == 1)
    {
        mstr += "过热告警\n";
    }
    if(mData.Bin.bit11 == 1)
    {
        mstr += "两侧输入都欠压告警\n";
    }
    if(mData.Bin.bit12 == 1)
    {
        mstr += "风机2无反馈告警\n";
    }
    if(mData.Bin.bit13 == 1)
    {
        mstr += "告警3：预留\n";
    }
    if(mData.Bin.bit14 == 1)
    {
        mstr += "告警4：预留\n";
    }
    if(mData.Bin.bit15 == 1)
    {
        mstr += "急停\n";
    }
    return mstr;
}


//60WK  显示状态数据
QString mprocess:: SystemStatusAnalysis(quint16 value)
{
    QString str = "0";
    UnionDataBin mData;
    mData.All = value;

    if( mData.Bin.bit1 == 1)
    {
        str = "运行";
        return  str;
    }
    if( mData.Bin.bit0 == 1 &&  mData.Bin.bit7 == 1 )
    {
        str = "停机";
        return  str;
    }
    if( mData.Bin.bit7 == 1)
    {
        str = "上电";//"故障";
        return  str;
    }
    if( mData.Bin.bit7 == 1)
    {
        str = "Ⅱ级软起";
        return  str;
    }
    if( mData.Bin.bit1 == 1)
    {
        str = "Ⅰ级软起";
        return  str;
    }
    if( mData.Bin.bit4 == 1)
    {
        str = "告警";
        return  str;
    }
    if( mData.Bin.bit3 == 1)
    {
        str = "Ⅰ级故障";
        return  str;
    }
    if(mData.Bin.bit2 == 1)
    {
        str = "Ⅱ级故障";
        return  str;
    }
    else
    {
        str = "下电";
        return  str;
    }
}
//系统故障分析（60KW整机）
QString mprocess:: SystemFaultAnalysis1(quint16 value,bool DateEnable)
{
    QString str = "";
    QString DateStr;
    UnionDataBin mData;

    mData.All = value;
    QDateTime m_CurrentDateTime = QDateTime::currentDateTime();
    if(DateEnable == true)
        DateStr = m_CurrentDateTime.toString("yyyy-mm-dd HH:mm:ss：");
    else
        DateStr = "警告：";

    if(mData.Bin.bit0 == 1)
    {
        str += DateStr+ "输入主交接异常\n";
    }
    if(mData.Bin.bit1 == 1)
    {
        str += DateStr+ "输入软启接触异常\n";
    }
    if(mData.Bin.bit2 == 1)
    {
        str += DateStr+ "输入反接\n";
    }
    if(mData.Bin.bit3 == 1)
    {
        str += DateStr+ "输入软启超时\n";
    }
    if(mData.Bin.bit4 == 1)
    {
        str += DateStr+ "输入欠压\n";
    }
    if(mData.Bin.bit5 == 1)
    {
        str += DateStr+ "输入过压\n";
    }
    if(mData.Bin.bit6 == 1)
    {
        str += DateStr+ "输入过流\n";
    }
    if(mData.Bin.bit7 == 1)
    {
        str += DateStr+ "输入对地短路\n";
    }
    if(mData.Bin.bit8 == 1)
    {
        str += DateStr+ "输出接触器异常\n";
    }
    if(mData.Bin.bit9 == 1)
    {
        str += DateStr+ "输出反接\n";
    }
    if(mData.Bin.bit10 == 1)
    {
        str += DateStr+ "输出软启超时\n";
    }
    if(mData.Bin.bit11 == 1)
    {
        str += DateStr+ "输出过压\n";
    }
    if(mData.Bin.bit12 == 1)
    {
        str += DateStr+ "输出过流\n";
    }
    if(mData.Bin.bit13 == 1)
    {
        str += DateStr+ "输出过载\n";
    }
    if(mData.Bin.bit14 == 1)
    {
        str += DateStr+ "输出对地短路\n";
    }
    if(mData.Bin.bit15 == 1)
    {
        str += DateStr+ "急停\n";
    }
    return str;
}

//故障信息2（60KW整机）
QString mprocess:: SystemFaultAnalysis2(quint16 value,bool DateEnable)
{
    QString str = "";
    QString DateStr;
    UnionDataBin mData;

    mData.All = value;
    QDateTime m_CurrentDateTime = QDateTime::currentDateTime();
    if(DateEnable == true)
        DateStr = m_CurrentDateTime.toString("yyyy-mm-dd HH:mm:ss：");
    else
        DateStr = "警告：";
    if(mData.Bin.bit12 == 1)
    {
        str += DateStr+ "内部SCI通讯超时\n";
    }
    if(mData.Bin.bit13 == 1)
    {
        str += DateStr+ "外部RS485通讯超时\n";
    }
    if(mData.Bin.bit14 == 1)
    {

        str += DateStr+ "外部CAN通讯超时\n";
    }
    if(mData.Bin.bit15 == 1)
    {
        str += DateStr+ "并机光纤通讯超时\n";
    }

    return str;
}

//系统故障分析（60KW整机）
QString  mprocess::SystemFaultAnalysis3(quint16 value,bool DateEnable)
{
    QString str = "";
    QString DateStr;
    UnionDataBin mData;

    mData.All = value;
    QDateTime m_CurrentDateTime = QDateTime::currentDateTime();
    if(DateEnable == true)
        DateStr = m_CurrentDateTime.toString("yyyy-mm-dd HH:mm:ss：");
    else
        DateStr = "警告：";
    if(mData.Bin.bit0 == 1)
    {
        str += DateStr+ "系统限功率报警\n";
    }
    if(mData.Bin.bit1 == 1)
    {
        str += DateStr+ "系统限流报警\n";
    }
    if(mData.Bin.bit2 == 1)
    {
        str += DateStr+ "系统限压报警\n";
    }
    if(mData.Bin.bit3 == 1)
    {
        str += DateStr+ "系统降额告警\n";
    }
    if(mData.Bin.bit4 == 1)
    {
        str += DateStr+ "环境过热告警\n";
    }
    if(mData.Bin.bit5 == 1)
    {
        str += DateStr+ "电感过热告警\n";
    }
    if(mData.Bin.bit6 == 1)
    {
        str += DateStr+ "散热器过热告警\n";
    }
    if(mData.Bin.bit7 == 1)
    {
        str += DateStr+ "从机有切断告警\n";
    }
    if(mData.Bin.bit8 == 1)
    {
        str += DateStr+ "风机异常告警\n";
    }
    if(mData.Bin.bit9 == 1)
    {
        str += DateStr+ "3#从机故障\n";
    }
    if(mData.Bin.bit10 == 1)
    {
        str += DateStr+ "2#从机故障\n";
    }
    if(mData.Bin.bit11 == 1)
    {
        str += DateStr+ "1#从机故障\n";
    }
    if(mData.Bin.bit12 == 1)
    {
        str += DateStr+ "防雷保护告警\n";
    }
    if(mData.Bin.bit13 == 1)
    {
        str += DateStr+ "熔断器告警\n";
    }

    return str;
}
