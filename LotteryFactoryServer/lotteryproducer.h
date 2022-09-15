#ifndef LOTTERYPRODUCER_H
#define LOTTERYPRODUCER_H

#include "lotteryfactorysetting.h"

#include <thread>
#include <mutex>

#include <iostream>
#include <sstream>

#include <vector>

#include <QApplication>

//============================================================================================================================

class ThreadInfo {
public:

    ThreadInfo() { nTicketsProduced = nTicketsInQueue = 0; }

    size_t nTicketsProduced;
    size_t nTicketsInQueue;
    std::string sStatus;
};

//============================================================================================================================

class LotteryProducer {
private:

    static void ParseParams(std::string sParams, const LotteryFactorySetting & settings);
    static size_t RunThreads(const LotteryFactorySetting & settings, uint32_t rseed);

public:

    static uint8_t nWinningTickets;              // Percentage of winning tickets
    static size_t nCurrentTicketID;
    static size_t nWinTicketsNumber;
    static std::vector<std::thread *> vThreads;
    static std::vector<size_t> vTicketsNumber;   // For every ticket type/format
    static std::vector<ThreadInfo> vInfo;

    static std::mutex TicketIDMutex;
    static std::mutex GUIMutex;
    static std::mutex WinFileMutex;
    static std::mutex LoseFileMutex;

    static const std::string sWinFN;
    static const std::string sLoseFN;

    static void Worker(size_t nThreadNumber, size_t nLineType, unsigned int nLineVelocity, size_t nTicketsNumber, uint32_t rseed);
    static size_t StartWorkers(std::string sParams, const LotteryFactorySetting & setting, uint32_t rseed);
    static void FreeThreadsVector();
    static size_t GetTicketID() { nCurrentTicketID++; return nCurrentTicketID; }
};

//============================================================================================================================

#endif // LOTTERYPRODUCER_H
