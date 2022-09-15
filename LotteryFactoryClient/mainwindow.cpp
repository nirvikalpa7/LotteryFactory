
#include <QStandardItemModel>
#include <QMessageBox>
#include <math.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

//================================================================================================================================

const size_t FullWorkTimeCommand = 101;
const size_t WinningTicketsCommand = 102;

const QString FactoryStartCommand = "FactoryStart";
const QString GetWinningTicketsCommand = "GetWinningTickets";

//================================================================================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("LotteryFactoryEmulator v1 - Client");

    pTcpSocket = new QTcpSocket(this);

    pTcpSocket->connectToHost(ui->lineEdit_IP->text(), ui->lineEdit_Port->text().toUInt());
    connect(pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotError(QAbstractSocket::SocketError)));

    QStandardItemModel *model = new QStandardItemModel;
    ui->listView->setModel(model);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->lineEdit_WorkTime->setReadOnly(true);
    ui->lineEdit_IP->setReadOnly(true);
    ui->lineEdit_Port->setReadOnly(true);

    // Changing colors of ReadOnly UI elements
    QPalette pal = ui->label->palette();
    const QColor color = pal.color(ui->label->backgroundRole());
    pal = ui->lineEdit_WorkTime->palette();
    pal.setColor(ui->lineEdit_WorkTime->backgroundRole(), color);
    ui->lineEdit_WorkTime->setPalette(pal);
    ui->lineEdit_IP->setPalette(pal);
    ui->lineEdit_Port->setPalette(pal);

    nWinTicketsNumberToGet = 0;
}

//================================================================================================================================

MainWindow::~MainWindow()
{
    if (pTcpSocket != nullptr)
    {
        pTcpSocket->disconnectFromHost();
        pTcpSocket->close();
        delete pTcpSocket;
    }

    delete ui;
}

//================================================================================================================================

void MainWindow::on_pushButton_Send_clicked()
{
    unsigned int nWinTickPrc = 0;
    nWinTickPrc = ui->lineEdit_Prc->text().toUInt();

    if ((nWinTickPrc >= 1) && (nWinTickPrc <= 100))
    {
        QByteArray  arrBlock;
        QDataStream out(&arrBlock, QIODevice::WriteOnly);
        QString str(FactoryStartCommand + " WinningTickets ");
        str += ui->lineEdit_Prc->text() + " " + ui->lineEdit_Params->text();
        out << str;
        if (pTcpSocket != nullptr)
        {
            pTcpSocket->write(arrBlock);
        }
    }
    else
    {
        QMessageBox::critical(this, "Error", " Incorrect WinningTickets percentage. It should be between 1 and 100.");
    }
}

//================================================================================================================================

void MainWindow::slotReadyRead()
{
    // Read winning tickets
    QTcpSocket* pClientSocket = (QTcpSocket*)sender();
    QDataStream in(pClientSocket);

    size_t nCommand = 0;
    if ((nWinTicketsNumberToGet == 0) && (pClientSocket->bytesAvailable() > 0))
    {
        in >> nCommand;
    }

    if(nCommand == FullWorkTimeCommand) // FullWorkTime
    {
        // Show factory FullWorkTime in UI
        size_t nFullWorkTime = 0;
        in >> nFullWorkTime;
        const QString sTime = std::to_string(size_t(round(nFullWorkTime / 1000.0))).c_str();
        ui->lineEdit_WorkTime->setText(sTime + " seconds");
    }
    else // WinningTickets
    {
        if (nCommand == WinningTicketsCommand)
        {
            in >> nWinTicketsNumberToGet;
        }

        // Add to the UI table
        QStandardItemModel * pModel = dynamic_cast<QStandardItemModel *>(ui->listView->model());
        QStandardItem *item;
        size_t id, type;
        QString str;
        while (pClientSocket->bytesAvailable() > 0)
        {
            in >> id;
            in >> type;
            str = QString(std::to_string(id).c_str()) + QString(" - ") + QString(std::to_string(type).c_str());
            item = new QStandardItem(str);
            pModel->appendRow(item);
            nWinTicketsNumberToGet -= 1;
        }
    }
}

//================================================================================================================================

void MainWindow::slotError(QAbstractSocket::SocketError err)
{
    const QString strError = "Error: " +
                     (err == QAbstractSocket::HostNotFoundError ?
                     "The host was not found." :
                     err == QAbstractSocket::RemoteHostClosedError ?
                     "The remote host is closed." :
                     err == QAbstractSocket::ConnectionRefusedError ?
                     "The connection was refused." :
                     QString(pTcpSocket->errorString()));

    QMessageBox::critical(this, "Socket Error", strError);
}

//================================================================================================================================

void MainWindow::on_pushButton_GetWinTick_clicked()
{
    nWinTicketsNumberToGet = 0;

    QStandardItemModel * pModel = dynamic_cast<QStandardItemModel *>(ui->listView->model());
    pModel->clear();

    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    QString str(GetWinningTicketsCommand);
    out << str;
    if (pTcpSocket != nullptr)
    {
        pTcpSocket->write(arrBlock);
    }
}

//================================================================================================================================

void MainWindow::slotConnected()
{

}

//================================================================================================================================
