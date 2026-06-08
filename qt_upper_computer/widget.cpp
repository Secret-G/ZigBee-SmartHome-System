#include "widget.h"
#include "ui_widget.h"
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

     this->setWindowTitle("智能灯控系统");


    initTemperatureChart();

    serial = new QSerialPort(this);
    connect(serial, &QSerialPort::readyRead,
            this, &Widget::readData);


    scanSerialPort();

    ui->btn_open_auto->setProperty("active", false);
    ui->btn_close_auto->setProperty("active", true);

    ui->btn_open_auto->style()->unpolish(ui->btn_open_auto);
    ui->btn_open_auto->style()->polish(ui->btn_open_auto);

    ui->btn_close_auto->style()->unpolish(ui->btn_close_auto);
    ui->btn_close_auto->style()->polish(ui->btn_close_auto);
    //this->showFullScreen();

}

Widget::~Widget()
{
    delete ui;
}

void Widget::initTemperatureChart()
{
    // 完整最终修正版（直接替换你的原有代码）
    Chart = new QtCharts::QChart();
    Chart->legend()->hide();

    // 设置背景（暖灰色，和实时环境数据模块一致）
    QColor bgColor(64, 59, 69); // 暖灰色背景
    Chart->setBackgroundBrush(bgColor);
    Chart->setPlotAreaBackgroundBrush(bgColor);
    Chart->setBackgroundVisible(true);
    Chart->setPlotAreaBackgroundVisible(true);
    Chart->setBackgroundRoundness(0);
    // 关键：设置圆角半径（单位：像素）
    Chart->setBackgroundRoundness(15); // 数值越大，圆角越明显，可根据需要调整

    // 优化曲线样式
    tempSeries = new QtCharts::QSplineSeries();
    QPen pen(QColor(80, 220, 200)); // 青绿色曲线，对比明显
    pen.setWidth(2); // 线宽调整为1.5px，精致且醒目
    pen.setCapStyle(Qt::RoundCap); // 线条端点圆润
    pen.setJoinStyle(Qt::RoundJoin); // 线条拐角圆润
    tempSeries->setPen(pen);
    tempSeries->setPointsVisible(false);

    Chart->addSeries(tempSeries);

    // 初始化坐标轴（兼容所有Qt版本的写法）
    axisX = new QValueAxis();
    axisY = new QValueAxis();

    // ========== 核心修复：兼容Qt版本的轴样式设置 ==========
    // 1. 设置轴线+刻度线颜色（用Qt官方标准方法）
    axisX->setLinePen(QPen(Qt::white));  // X轴轴线+刻度线白色
    axisY->setLinePen(QPen(Qt::white));  // Y轴轴线+刻度线白色

    // 2. 设置刻度标签颜色
    axisX->setLabelsColor(Qt::white);
    axisY->setLabelsColor(Qt::white);

    // 3. 确保标签可见（显式开启）
    axisX->setLabelsVisible(true);
    axisY->setLabelsVisible(true);

    // 4. 关闭网格线（保留你的原有设置）
    axisX->setGridLineVisible(false);
    axisY->setGridLineVisible(false);

    // 5. 优化坐标轴范围和刻度（解决刻度消失关键）
    axisX->setRange(0, 50);
    axisY->setRange(15, 30); // 缩小Y轴范围，放大曲线起伏

    // 刻度步长+数量（15-30℃，步长2℃，共9个刻度）
    axisY->setTickInterval(2);
    axisY->setTickCount(9);
    axisX->setTickCount(6);     // X轴0-50，步长10，6个刻度

    // 添加坐标轴到图表
    Chart->addAxis(axisX, Qt::AlignBottom);
    Chart->addAxis(axisY, Qt::AlignLeft);
    tempSeries->attachAxis(axisX);
    tempSeries->attachAxis(axisY);

    // 最终设置
    ui->chartView_temp->setChart(Chart);
    ui->chartView_temp->setRenderHint(QPainter::Antialiasing); // 开启抗锯齿，曲线更平滑
}

void Widget::scanSerialPort()
{
    ui->comboBox_port->clear();

       QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();

       for(const QSerialPortInfo &info : portList)
       {
           ui->comboBox_port->addItem(info.portName());
       }

       if(portList.isEmpty())
       {
           qDebug() << "没有检测到串口";
       }
}




// 全局缓冲区（建议在Widget类中定义，避免函数内局部变量丢失数据）

void Widget::readData()
{
    newData = serial->readAll();
    // 将新接收的数据追加到缓冲区
    buffer.append(newData);
    while (true)
    {
        // 找帧头0xAA的位置
        int startIdx = buffer.indexOf(0xAA);
        if (startIdx < 0) {
            buffer.clear();
            return;
        }
        // 移除帧头前的脏数据
        if (startIdx > 0) {
            buffer.remove(0, startIdx);
        }
        // 至少需要读取到帧头、地址、命令码和长度字段
        if (buffer.size() < 5) {
            return;
        }

        // 计算完整帧长度
        quint8 dataLen = (quint8)buffer[4];
        int totalFrameLen = 7 + dataLen;

         // 数据未接收完整，等待下一次串口数据到达
        if (buffer.size() < totalFrameLen) {
            return;
        }
        // 校验帧尾0x55
        if ((quint8)buffer[totalFrameLen - 1] != 0x55) {
            buffer.remove(0, 1);
            continue;
        }
        // 提取完整数据帧并交由后续模块处理
        QByteArray frame = buffer.mid(0, totalFrameLen);
        processFrame(frame);
        // 移除已处理的帧
        buffer.remove(0, totalFrameLen);
    }
}

void Widget::processFrame(QByteArray data)
{
    // 基础校验：至少7字节（AA+地址2+CMD1+LEN1+数据1+校验1+55）
    if (data.size() < 7) {
        qDebug() << "帧长度不足：" << data.size();
        return;
    }

    // 提取命令码
    quint8 cmd = (quint8)data[3];
    //qDebug() << "处理帧 - 命令码: 0x" << QString::number(cmd, 16).toUpper()
           //      << "  完整帧:" << data.toHex(' ').toUpper();

    switch (cmd)
    {
        case 0x01:   // 温湿度终端数据（保留你的原有逻辑）
        {
            // 校验数据长度是否匹配（避免越界）
            if (data.size() < 11) {
                qDebug() << "温湿度帧长度错误：" << data.size();
                break;
            }
            quint8 temp = (quint8)data[5];
            quint8 humi = (quint8)data[6];
            quint8 gasDO = (quint8)data[7];
            quint16 gasAO = ((quint8)data[8] << 8) | (quint8)data[9];
            quint8 status = (quint8)data[10];

            ui->label_temp_value->setText(QString::number(temp) + " °C");

           // qDebug() << "解析环境数据：温度" << temp << "°C, 湿度" << humi
                    // << "%, 烟雾AO" << gasAO;

            ui->label_humidity_value->setText(QString::number(humi) + " %");

            tempSeries->append(xValue++, temp);

                // 关键：动态调整 Y 轴范围，而不是固定 0-30
                double minTemp = temp - 2;
                double maxTemp = temp + 2;
                if (minTemp < 15) minTemp = 15;
                if (maxTemp > 30) maxTemp = 30;
                axisY->setRange(minTemp, maxTemp);

                if (xValue > 30) {
                    axisX->setRange(xValue - 30, xValue);
                }
            QString level;
            if(gasAO < 500)
                level = "低";
            else if(gasAO < 1500)
                level = "中";
            else
                level = "高";

            ui->label_smoke_value->setText(QString::number(gasAO) + " (" + level + ")");

            if(status == 0)
            {
                ui->label_status_value->setText("正常");
                // 恢复正常样式（绿色）
                ui->label_status_value->setStyleSheet(R"(
                        background-color: #4f6358;
                        color: #f0ece6;
                        font-size: 18px;
                        font-weight: bold;
                        border-radius: 16px;
                        padding: 8px 16px;
                        min-width: 120px;
                        text-align: center;
                    )");

                // 新增：仅当之前是报警状态，现在恢复正常时，才发送解除指令
                    if(isSmokeAlarm)
                    {
                        QByteArray cancelFrame;
                        cancelFrame = buildSmokeAlarmFrame(0x00);
                        if(serial->isOpen())
                        {
                            serial->write(cancelFrame);
                            qDebug() << "发送解除异常报警指令:" << cancelFrame.toHex(' ');
                        }

                        isSmokeAlarm = false; // 重置报警状态标记
                    }
            }
            else
            {
                ui->label_status_value->setText("异常报警");
                ui->label_status_value->setStyleSheet(R"(
                                    background-color: #FF4444;
                                    color: white;
                                    font-size: 16px;
                                    font-weight: bold;
                                    border-radius: 8px;
                                    padding: 8px 16px;
                                    min-width: 120px;
                                    text-align: center;
                                )");
                // 仅当之前不是报警状态时，才发送报警指令（避免重复发送）
                    if(!isSmokeAlarm)
                    {
                        // 发送烟雾报警指令帧
                        QByteArray alarmFrame;
                        alarmFrame = buildSmokeAlarmFrame(0x01);
                        if(serial->isOpen())
                        {
                            serial->write(alarmFrame);
                            qDebug() << "发送烟雾报警指令:" << alarmFrame.toHex(' ');
                        }

                        isSmokeAlarm = true; // 标记为报警状态
                    }
            }

            break;
        }

        case 0x02:   // 光照终端数据（修复+增强）
        {
            // 校验光照帧长度是否为9字节（AA+00+02+02+02+luxH+luxL+sum+55）
            if (data.size() != 9) {
                qDebug() << "光照帧长度错误：" << data.size() << "（预期9）";
                break;
            }

            // 计算光照值（luxH在前，luxL在后）
            quint16 lux = ((quint8)data[5] << 8) | (quint8)data[6];

            // 更新UI（确保在主线程，QT串口回调默认是主线程）
            ui->label_light_value->setText(QString::number(lux) + " lx");
            break;
        }

        default:
            qDebug() << "未知命令码：0x" << QString::number(cmd, 16).toUpper();
            break;
    }
}
void Widget::on_btn_close_serial_clicked()
{
    serial->close();
    qDebug() << "串口关闭";
}

void Widget::on_btn_open_serial_clicked()
{
     //scanSerialPort();
    /*端口号*/
    serial->setPortName(ui->comboBox_port->currentText());
    serial->setBaudRate(115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setParity(QSerialPort::NoParity);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    if(serial->open(QIODevice::ReadWrite))
    {
        qDebug()<< "打开成功";
    }
    else
    {
        qDebug()<< "打开失败";
    }


}

void Widget::on_btn_open_light_clicked()
{
    QByteArray frame;
    frame = buildControlFrame(0x01,0x01);

    if (serial->isOpen()) {
        serial->write(frame);
        qDebug() << "发送开灯指令:" << frame.toHex(' ');

        ui->btn_open_light->setProperty("active", true);
        ui->btn_close_light->setProperty("active", false);

        ui->btn_open_light->style()->unpolish(ui->btn_open_light);
        ui->btn_open_light->style()->polish(ui->btn_open_light);

        ui->btn_close_light->style()->unpolish(ui->btn_close_light);
        ui->btn_close_light->style()->polish(ui->btn_close_light);
    } else {
        qDebug() << "串口未打开，发送失败！";
    }
}


// 关灯按键
void Widget::on_btn_close_light_clicked()
{
    QByteArray frame;
    frame = buildControlFrame(0x01,0x00);

    if (serial->isOpen()) {
        serial->write(frame);
        qDebug() << "发送关灯指令:" << frame.toHex(' ');

        ui->btn_open_light->setProperty("active", false);
        ui->btn_close_light->setProperty("active", true);

        ui->btn_open_light->style()->unpolish(ui->btn_open_light);
        ui->btn_open_light->style()->polish(ui->btn_open_light);

        ui->btn_close_light->style()->unpolish(ui->btn_close_light);
        ui->btn_close_light->style()->polish(ui->btn_close_light);
    } else {
        qDebug() << "串口未打开，发送失败！";
    }
}

// 开启自动模式按键
void Widget::on_btn_open_auto_clicked()
{
    QByteArray frame;
    frame = buildControlFrame(0x02,0x01);
    if (serial->isOpen()) {
        serial->write(frame);
        qDebug() << "发送开启自动模式指令:" << frame.toHex(' ');

        // 更新按钮状态
        ui->btn_open_auto->setProperty("active", true);
        ui->btn_close_auto->setProperty("active", false);

        ui->btn_open_auto->style()->unpolish(ui->btn_open_auto);
        ui->btn_open_auto->style()->polish(ui->btn_open_auto);

        ui->btn_close_auto->style()->unpolish(ui->btn_close_auto);
        ui->btn_close_auto->style()->polish(ui->btn_close_auto);
    } else {
        qDebug() << "串口未打开，发送失败！";
    }
}

// 关闭自动模式按键
void Widget::on_btn_close_auto_clicked()
{
    QByteArray frame;
    frame = buildControlFrame(0x02,0x00);
    if (serial->isOpen()) {
        serial->write(frame);
        qDebug() << "发送关闭自动模式指令:" << frame.toHex(' ');

        // 更新按钮状态
        ui->btn_open_auto->setProperty("active", false);
        ui->btn_close_auto->setProperty("active", true);

        ui->btn_open_auto->style()->unpolish(ui->btn_open_auto);
        ui->btn_open_auto->style()->polish(ui->btn_open_auto);

        ui->btn_close_auto->style()->unpolish(ui->btn_close_auto);
        ui->btn_close_auto->style()->polish(ui->btn_close_auto);
    } else {
        qDebug() << "串口未打开，发送失败！";
    }
}

QByteArray Widget::buildControlFrame(quint8 subCmd, quint8 param)
{
    QByteArray frame;
       frame.append(quint8(0xAA));
       frame.append(quint8(0x00));
       frame.append(quint8(0x02));
       frame.append(quint8(0x10));
       frame.append(quint8(0x02));
       frame.append(subCmd);
       frame.append(param);

       quint8 sum = 0x00 + 0x02 + 0x10 +0x02+ subCmd + param;
       frame.append(quint8(sum));
       frame.append(quint8(0x55));

       return frame;
}

QByteArray Widget::buildSmokeAlarmFrame(quint8 param)
{
    QByteArray frame;
        frame.append(quint8(0xAA));
        frame.append(quint8(0x00));
        frame.append(quint8(0x02));
        frame.append(quint8(0x10));
        frame.append(quint8(0x02));
        frame.append(quint8(0x03));
        frame.append(param);

        quint8 sum = 0x00 + 0x02 + 0x10 + 0x02 + 0x03 + param;
        frame.append(quint8(sum));
        frame.append(quint8(0x55));

        return frame;
}
