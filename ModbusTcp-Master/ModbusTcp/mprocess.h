#ifndef MPROCESS_H
#define MPROCESS_H
#include <QMainWindow>
#include <QTime>
#include <QDateTime>

#define ModbusTcpOB     0x00000  //事务标识符
#define ModbusTcpPR     0x00000  //协议标识符
#define ModbusTcpID     0x01     //模块ID
 //声明一个结构体类型
typedef struct {
    unsigned short addr;
    unsigned char fun;
    unsigned short data[128];
    unsigned short len;
}ModbusSendStruct;
//声明一个联合体类型
typedef union
{
    unsigned int IntData;
    unsigned short ShortData[2];
    float  FloatData;
}FLAOT_UNION;


typedef enum {
    TableContrrol = 0,
    TableRated,
    TableProtect,
    TableSystem,
    TableCorrect
}TableMenu;
typedef struct{
  unsigned short  bit0:1;
  unsigned short  bit1:1;
  unsigned short  bit2:1;
  unsigned short  bit3:1;
  unsigned short  bit4:1;
  unsigned short  bit5:1;
  unsigned short  bit6:1;
  unsigned short  bit7:1;
  unsigned short  bit8:1;
  unsigned short  bit9:1;
  unsigned short  bit10:1;
  unsigned short  bit11:1;
  unsigned short  bit12:1;
  unsigned short  bit13:1;
  unsigned short  bit14:1;
  unsigned short  bit15:1;

}DataBin;

typedef union{
    DataBin Bin;
    unsigned short All;
}UnionDataBin;

typedef union
{
    unsigned int IntData;
    unsigned char CharData[2];
}Int_UNION;


class mprocess
{
public:
    //系统的状态解析
    QString SystemStatusAnalysis(quint16 value);
    //系统的故障1解析
    QString SystemFaultAnalysis1(quint16 value,bool DateEnable);
    //系统的故障2解析
    QString SystemFaultAnalysis2(quint16 value,bool DateEnable);
    //系统的故障3解析
    QString SystemFaultAnalysis3(quint16 value,bool DateEnable);
    //模块的故障解析
    QString ModuleFaultAnalysis(unsigned int Value1,unsigned int Value2);

};

#endif // MPROCESS_H
