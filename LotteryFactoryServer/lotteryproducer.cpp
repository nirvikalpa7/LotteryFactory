#include "lotteryproducer.h"
#include "myrand.h"

#include <stdlib.h>
#include <QRandomGenerator>
#include <fstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

uint8_t LotteryProducer::nWinningTickets = 0;
size_t LotteryProducer::nCurrentTicketID = 0;
size_t LotteryProducer::nWinTicketsNumber = 0;

std::vector<std::thread *> LotteryProducer::vThreads;
std::vector<size_t> LotteryProducer::vTicketsNumber;
std::vector<ThreadInfo> LotteryProducer::vInfo;

std::mutex LotteryProducer::TicketIDMutex;
std::mutex LotteryProducer::GUIMutex;
std::mutex LotteryProducer::WinFileMutex;
std::mutex LotteryProducer::LoseFileMutex;

const std::string LotteryProducer::sWinFN = "win.txt";
const std::string LotteryProducer::sLoseFN = "lose.txt";

//============================================================================================================================

void LotteryProducer::Worker(size_t nThreadNumber, size_t nLineType, unsigned int nLineVelocity, size_t nTicketsNumber, uint32_t rseed)
{
    std::unique_lock<std::mutex> lock(GUIMutex);
        vInfo[nThreadNumber].sStatus = "Working";
        vInfo[nThreadNumber].nTicketsInQueue = nTicketsNumber;
        vInfo[nThreadNumber].nTicketsProduced = 0;
    lock.unlock();

    size_t nTicketID;
    bool bWinning;
    std::ofstream out;

    QRandomGenerator rand;
    rand.seed(static_cast<uint32_t>(nThreadNumber) ^ rseed);

    for(size_t i = 0; i < nTicketsNumber; i++)
    {
        TicketIDMutex.lock();
            nTicketID = GetTicketID();
        TicketIDMutex.unlock();

        const size_t my_rand = rand.generate() % 100 + 1;

        bWinning = (my_rand <= nWinningTickets) ? true : false;

        if(bWinning)
        {
            std::unique_lock<std::mutex> lock(WinFileMutex);
            out.open(sWinFN, std::ofstream::out | std::ofstream::app);
            if (out.is_open())
            {
                out << nTicketID << " " << nLineType << std::endl;
            }
            out.close();
            nWinTicketsNumber++;
        }
        else
        {
            std::unique_lock<std::mutex> lock(LoseFileMutex);
            out.open(sLoseFN, std::ofstream::out | std::ofstream::app);
            if (out.is_open())
            {
                out << nTicketID << " " << nLineType << std::endl;
            }
            out.close();
        }

        std::chrono::milliseconds time(nLineVelocity);
        std::this_thread::sleep_for(time);

        GUIMutex.lock();
            vInfo[nThreadNumber].nTicketsInQueue = nTicketsNumber - i - 1;
            vInfo[nThreadNumber].nTicketsProduced = i + 1;
        GUIMutex.unlock();
    }

    GUIMutex.lock();
        vInfo[nThreadNumber].sStatus = "Stop";
    GUIMutex.unlock();
}

//============================================================================================================================

void LotteryProducer::FreeThreadsVector()
{
    if(vThreads.size() > 0)
    {
        for(std::thread * pT : vThreads)
        {
            delete pT;
        }
        vThreads.clear();
    }
}

//============================================================================================================================

size_t LotteryProducer::StartWorkers(std::string sParams, const LotteryFactorySetting & settings, uint32_t rseed)
{
    ParseParams(sParams, settings);

    FreeThreadsVector();
    nCurrentTicketID = 0;

    if (vInfo.size() > 0)
    {
        vInfo.clear();
    }
    vInfo.resize(settings.GetThreadsNumber());

    fs::path p1(sWinFN);
    if (fs::exists(p1))
    {
        fs::remove(p1);
    }
    fs::path p2(sLoseFN);
    if (fs::exists(p2))
    {
        fs::remove(p2);
    }

    return RunThreads(settings, rseed);
}

//============================================================================================================================

void LotteryProducer::ParseParams(std::string sParams, const LotteryFactorySetting & settings)
{
    std::stringstream StrStream;
    StrStream << sParams;

    std::string buf;
    unsigned int t;

    StrStream >> buf;

    if (buf.compare("FactoryStart") != 0)
    {
        return;
    }

    StrStream >> buf;

    nWinningTickets = 0;
    if (buf.compare("WinningTickets") == 0)
    {
        StrStream >> t;
        if (t >= 1 && t <= 100)
        {
            nWinningTickets = t;
        }
    }

    unsigned int type, number;
    vTicketsNumber.clear();
    const size_t nLineTypesNumber = settings.GetLineTypesNumber();
    vTicketsNumber.resize(nLineTypesNumber);
    for (size_t i = 0; i < nLineTypesNumber; i++)
    {
        StrStream >> buf;

        if (buf.compare("LotteryTicketsNumber") == 0)
        {
            StrStream >> type;
            StrStream >> number;
            if (number > 0)
            {
                vTicketsNumber[i] = number;
            }
        }
    }
}

//============================================================================================================================

size_t LotteryProducer::RunThreads(const LotteryFactorySetting & settings, uint32_t rseed)
{
    // Start Threads
    nWinTicketsNumber = 0;
    size_t nThreadIndex = 0;
    const size_t nLineTypesNumber = settings.GetLineTypesNumber();
    std::vector<size_t> vTimeTakes;

    for(size_t i = 0; i < nLineTypesNumber; i++)
    {
        const size_t nLinesNumber = settings.GetLinesNumber(i);

        if (vTicketsNumber[i] == 0) // If lines do not have load
        {
            for(size_t i = 0; i < nLinesNumber; i++)
            {
                vInfo[nThreadIndex].sStatus = "Stop";
                nThreadIndex++;
            }
            continue;
        }

        const unsigned int nLineVelocity = settings.GetProduceVelocity(i);
        size_t nTicketsNum = vTicketsNumber[i] / nLinesNumber;
        size_t nTicketsTail = vTicketsNumber[i] - nTicketsNum * (nLinesNumber - 1);
        const size_t nLineType = i + 1;

        bool bNotFullLoadMode = false;
        if (vTicketsNumber[i] < nLinesNumber)
        {
            bNotFullLoadMode = true;
            nTicketsNum = 1;
        }

        bool bArrayMode = false;
        std::vector<size_t> vLoadArray;
        if (nTicketsTail > nTicketsNum + 1)
        {
            bArrayMode = true;
            for (size_t i = 0; i < nLinesNumber; i++)
            {
                vLoadArray.push_back(nTicketsNum);
            }
            const size_t nExtraPass = nTicketsTail - nTicketsNum;
            for (size_t i = 0; i < nExtraPass; i++)
            {
                vLoadArray[i] += 1;
            }
        }

        for(size_t j = 0; j < nLinesNumber; j++)
        {
            if (bNotFullLoadMode)
            {
                if (j+1 > vTicketsNumber[i])
                {
                    for(size_t k = j; k < nLinesNumber; k++)
                    {
                        vInfo[nThreadIndex].sStatus = "Stop";
                        nThreadIndex++;
                    }
                    break;
                }
            }
            else if (bArrayMode)
            {
                nTicketsNum = vLoadArray[j];
            }
            else if (j == nLinesNumber - 1)
            {
                nTicketsNum = nTicketsTail;
            }

            std::thread * pThread = new std::thread(LotteryProducer::Worker, nThreadIndex, nLineType, nLineVelocity, nTicketsNum, rseed);
            vThreads.push_back(pThread);
            nThreadIndex++;
            vTimeTakes.push_back(nTicketsNum * nLineVelocity);
        }
    }

    for(size_t i = 0; i < vThreads.size(); i++)
    {
        vThreads[i]->detach();
    }

    size_t nFullTime = 0;
    if (vTimeTakes.size() > 0)
    {
        std::sort(vTimeTakes.begin(), vTimeTakes.end());
        nFullTime = vTimeTakes[vTimeTakes.size() - 1];
    }

    return nFullTime;
}

//============================================================================================================================
