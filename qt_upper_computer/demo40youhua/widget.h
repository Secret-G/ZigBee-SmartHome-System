#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
// 必须包含这三个头文件
#include <QtCharts/QChart>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>

// 使用QtCharts命名空间，避免每次都写QtCharts::
QT_CHARTS_USE_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void initTemperatureChart();

private slots:
    void readData();
    void scanSerialPort();
    void processFrame(QByteArray data);

    void on_btn_close_serial_clicked();
    void on_btn_open_serial_clicked();
    void on_btn_open_light_clicked();
    void on_btn_close_light_clicked();
    void on_btn_open_auto_clicked();
    void on_btn_close_auto_clicked();

private:
    Ui::Widget *ui;
    QSerialPort *serial;

    // 图表相关成员变量
    QChart *Chart;
    QSplineSeries *tempSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;
    QByteArray buffer;
    QByteArray newData;
    qreal xValue = 0;

    bool isSmokeAlarm = false;
    QByteArray buildControlFrame(quint8 subCmd, quint8 param);
    QByteArray buildSmokeAlarmFrame(quint8 param);

};

#endif // WIDGET_H
