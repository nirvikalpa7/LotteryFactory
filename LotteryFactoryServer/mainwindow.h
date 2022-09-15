#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <vector>

#include "lotteryfactorysetting.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_repaintTable();
    void on_pushButton_Start_clicked();
    void on_newConnection();
    void on_disconnect();
    void on_readFromClient();

private:

    void sendToClient(const QString& str);
    void sendWinTicketsToClient();
    void sendFullWorkTimeToClient(size_t nTime);
    void initTable();
    size_t runFactory(const std::string & sParams);

    Ui::MainWindow * ui;
    LotteryFactorySetting LFSettings;
    QTimer * pTimer;
    QTcpServer * pTcpServer;
    QTcpSocket * pClientSocket;
    bool bFactoryWorking;
};
#endif // MAINWINDOW_H
