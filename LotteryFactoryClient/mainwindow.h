#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

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
    void on_pushButton_Send_clicked();
    void on_pushButton_GetWinTick_clicked();
    void slotReadyRead();
    void slotError(QAbstractSocket::SocketError);
    void slotConnected();

private:

    Ui::MainWindow *ui;
    QTcpSocket* pTcpSocket;
    size_t nWinTicketsNumberToGet;
};
#endif // MAINWINDOW_H
