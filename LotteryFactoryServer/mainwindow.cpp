#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lotteryproducer.h"

#include <QStandardItemModel>
#include <QStandardItem>
#include <QTimer>
#include <QTime>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QAction>

#include <sysinfoapi.h>
#include <math.h>

//============================================================================================================================

const std::string sSettingsFileName = "LotteryFactorySettings.txt";

const size_t nWinTicketsClientSendBufLimit = 50; // Change it as you need

const size_t FullWorkTimeCommand = 101;
const size_t WinningTicketsCommand = 102;

const QString FactoryStartCommand = "FactoryStart";
const QString GetWinningTicketsCommand = "GetWinningTickets";

const unsigned int nUIRefreshInterval = 500; // In ms

//============================================================================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("LotteryFactoryEmulator v1 - Server");

    ui->lineEdit_FN->setText(sSettingsFileName.c_str());
    ui->lineEdit_FN->setReadOnly(true);
    ui->lineEdit_TicketTypesNum->setReadOnly(true);
    if (LFSettings.LoadSettingsFromFile(sSettingsFileName))
    {
        const size_t nLineTypesNumber = LFSettings.GetLineTypesNumber();
        ui->lineEdit_TicketTypesNum->setText(std::to_string(nLineTypesNumber).c_str());
        if (nLineTypesNumber == 0)
        {
            QMessageBox::critical(this, "Error", "Error in a setting file (" + QString(sSettingsFileName.c_str()) + "), LineTypesNumber must be > 0");
        }
    }

    initTable();

    pTimer = new QTimer();
    pTimer->setInterval(nUIRefreshInterval);
    connect(pTimer, SIGNAL(timeout()), this, SLOT(on_repaintTable()));

    ui->lineEdit_SrvStatus->setReadOnly(true);
    ui->lineEdit_IPAddr->setReadOnly(true);
    ui->lineEdit_Port->setReadOnly(true);
    pClientSocket = nullptr;
    pTcpServer = new QTcpServer(this);
    const QHostAddress addr(ui->lineEdit_IPAddr->text());
    const quint16 nPort = ui->lineEdit_Port->text().toUInt();
    if (!pTcpServer->listen(addr, nPort))
    {
        QMessageBox::critical(0, "Server Error", "Unable to start the server:" + pTcpServer->errorString());
        pTcpServer->close();
        ui->lineEdit_SrvStatus->setText("Server error");
    }
    else
    {
        ui->lineEdit_SrvStatus->setText("Listening");
        connect(pTcpServer, SIGNAL(newConnection()), this, SLOT(on_newConnection()));
    }

    bFactoryWorking = false;

    ui->lineEdit_WorkTime->setReadOnly(true);

    // Changing colors of ReadOnly UI elements
    QPalette pal = ui->label->palette();
    const QColor color = pal.color(ui->label->backgroundRole());
    pal = ui->lineEdit_WorkTime->palette();
    pal.setColor(ui->lineEdit_WorkTime->backgroundRole(), color);
    ui->lineEdit_WorkTime->setPalette(pal);
    ui->lineEdit_SrvStatus->setPalette(pal);
    ui->lineEdit_IPAddr->setPalette(pal);
    ui->lineEdit_Port->setPalette(pal);
    ui->lineEdit_FN->setPalette(pal);
    ui->lineEdit_TicketTypesNum->setPalette(pal);
}

//============================================================================================================================

MainWindow::~MainWindow()
{
    if (pTimer != nullptr)
    {
        pTimer->stop();
        delete pTimer;
    }

    if (pClientSocket != nullptr)
    {
        pClientSocket->close();
        delete pClientSocket;
    }

    if (pTcpServer != nullptr)
    {
        pTcpServer->close();
        delete pTcpServer;
    }

    delete ui;
}

//============================================================================================================================

void MainWindow::initTable()
{
    QStandardItemModel *model = new QStandardItemModel;
    QStandardItem *item;

    // Заголовки столбцов
    QStringList horizontalHeader;
    horizontalHeader.append("LineType");
    horizontalHeader.append("ProducingTime");
    horizontalHeader.append("QueueTickets");
    horizontalHeader.append("ProducedTickets");
    horizontalHeader.append("LineStatus");

    // Заголовки строк
    QStringList verticalHeader;
    for(size_t i = 0; i < LFSettings.GetThreadsNumber(); i++)
    {
        std::string sTitle;
        sTitle = "Thread " + std::to_string(i+1);
        verticalHeader.append(sTitle.c_str());
    }

    model->setHorizontalHeaderLabels(horizontalHeader);
    model->setVerticalHeaderLabels(verticalHeader);

    const size_t nLineTypesNumber = LFSettings.GetLineTypesNumber();
    size_t nThreadIndex = 0;
    for(size_t i = 0; i < nLineTypesNumber; i++)
    {
        const size_t nLinesNumber = LFSettings.GetLinesNumber(i);
        for(size_t j = 0; j < nLinesNumber; j++)
        {
            // Ряд
            std::string sLineType = std::to_string(i+1);
            item = new QStandardItem(sLineType.c_str());
            model->setItem(nThreadIndex, 0, item);

            const unsigned int nVel = LFSettings.GetProduceVelocity(i);
            item = new QStandardItem(QString(std::to_string(nVel).c_str()) + QString(" ms"));
            model->setItem(nThreadIndex, 1, item);

            item = new QStandardItem(QString("0"));
            model->setItem(nThreadIndex, 2, item);

            item = new QStandardItem(QString("0"));
            model->setItem(nThreadIndex, 3, item);

            item = new QStandardItem(QString("Stop"));
            model->setItem(nThreadIndex, 4, item);

            nThreadIndex++;
        }
    }

    ui->tableView->setModel(model);
    ui->tableView->resizeRowsToContents();
    ui->tableView->resizeColumnsToContents();
    ui->tableView->setEditTriggers(QTableView::NoEditTriggers);
}

//============================================================================================================================

void MainWindow::on_repaintTable()
{
    std::unique_lock<std::mutex> lock(LotteryProducer::GUIMutex);
    QString status, queue, produced;
    QModelIndex index;
    size_t nTicketsInQueueSum = 0;
    for (size_t i = 0; i < LotteryProducer::vInfo.size(); i++)
    {
        nTicketsInQueueSum += LotteryProducer::vInfo[i].nTicketsInQueue;

        queue = std::to_string(LotteryProducer::vInfo[i].nTicketsInQueue).c_str();
        produced = std::to_string(LotteryProducer::vInfo[i].nTicketsProduced).c_str();
        status = LotteryProducer::vInfo[i].sStatus.c_str();

        index = ui->tableView->model()->index(i, 2);
        ui->tableView->model()->setData(index, QVariant(queue));

        index = ui->tableView->model()->index(i, 3);
        ui->tableView->model()->setData(index, QVariant(produced));

        index = ui->tableView->model()->index(i, 4);
        ui->tableView->model()->setData(index, QVariant(status));
    }
    lock.unlock();

    if (nTicketsInQueueSum == 0)
    {
        pTimer->stop();
        bFactoryWorking = false;
    }
}

//============================================================================================================================

void MainWindow::on_pushButton_Start_clicked()
{
    if(!bFactoryWorking)
    {
        unsigned int nWinTickPrc = 0;
        nWinTickPrc = ui->lineEdit_Prc->text().toUInt();
        if ((nWinTickPrc >= 1) && (nWinTickPrc <= 100))
        {
            const QString str1 = ui->lineEdit_Params->text();
            const QString str2 = QString(FactoryStartCommand + " WinningTickets ") + ui->lineEdit_Prc->text() + QString(" ") + str1;

            runFactory(str2.toStdString());
        }
        else
        {
            QMessageBox::critical(this, "Error", " Incorrect WinningTickets percentage. It should be between 1 and 100.");
        }
    }
}

//============================================================================================================================

size_t MainWindow::runFactory(const std::string & sParams)
{
    uint32_t rseed = GetTickCount();

    bFactoryWorking = true;
    pTimer->start();

    const size_t nFullWorkTime = LotteryProducer::StartWorkers(sParams, LFSettings, rseed);
    LotteryProducer::FreeThreadsVector();

    const QString str = std::to_string(size_t(round(nFullWorkTime / 1000.0))).c_str();
    ui->lineEdit_WorkTime->setText(str + " seconds");

    return nFullWorkTime;
}

//============================================================================================================================

void MainWindow::on_newConnection()
{
    // Only one client we can have
    pClientSocket = pTcpServer->nextPendingConnection();
    connect(pClientSocket, SIGNAL(disconnected()), this, SLOT(on_disconnect()));
    connect(pClientSocket, SIGNAL(readyRead()), this, SLOT(on_readFromClient()));

    ui->lineEdit_SrvStatus->setText("Client connected");
}

//============================================================================================================================

void MainWindow::on_readFromClient()
{
    QTcpSocket* pClientSocket = (QTcpSocket*)sender();
    QDataStream in(pClientSocket);
    QString str;
    in >> str;
    std::stringstream StrStream;
    StrStream << str.toStdString();
    std::string sCommand;
    StrStream >> sCommand;

    if (QString(sCommand.c_str()) == FactoryStartCommand)
    {
        if (!bFactoryWorking)
        {
            const size_t nFullWorkTime = runFactory(str.toStdString());
            sendFullWorkTimeToClient(nFullWorkTime);
        }
    }
    else if (QString(sCommand.c_str()) == GetWinningTicketsCommand)
    {
        if (!bFactoryWorking)
        {
            sendWinTicketsToClient();
        }
    }
}

//============================================================================================================================

void MainWindow::sendFullWorkTimeToClient(size_t nTime)
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << size_t(FullWorkTimeCommand) << nTime;
    if (pClientSocket != nullptr)
    {
        pClientSocket->write(arrBlock);
        pClientSocket->waitForBytesWritten();
    }
}

//============================================================================================================================

void MainWindow::sendWinTicketsToClient()
{
    std::ifstream fin(LotteryProducer::sWinFN, std::ifstream::in);
    if (fin.is_open())
    {
        std::unique_ptr<QByteArray> pArrBlock(new QByteArray);
        std::unique_ptr<QDataStream> pOut(new QDataStream(pArrBlock.get(), QIODevice::WriteOnly));

        size_t nRealTicketNumber, id, type;
        *pOut << size_t(WinningTicketsCommand) << size_t(LotteryProducer::nWinTicketsNumber);
        while(fin.good())
        {
            for(nRealTicketNumber = 0; nRealTicketNumber < nWinTicketsClientSendBufLimit; nRealTicketNumber++)
            {
                if (fin.good())
                {
                    fin >> id;
                    fin >> type;

                    if (fin.good())
                    {
                        *pOut << id;
                        *pOut << type;
                    }
                }
                else
                {
                    break;
                }
            }

            // Send array to client
            if (pClientSocket != nullptr)
            {
                pClientSocket->write(*pArrBlock);
                pClientSocket->waitForBytesWritten();
                pArrBlock->remove(0, pArrBlock->size());
                pOut->device()->reset();
            }
        }
    }
}

//============================================================================================================================

void MainWindow::sendToClient(const QString& str)
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << str;
    if (pClientSocket != nullptr)
    {
        pClientSocket->write(arrBlock);
    }
}

//============================================================================================================================

void MainWindow::on_disconnect()
{
    ui->lineEdit_SrvStatus->setText("Listening");
}

//============================================================================================================================
