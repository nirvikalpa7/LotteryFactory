
#include "lotteryfactorysetting.h"

#include <QMessageBox>
#include <QString>

//============================================================================================================================

void LotteryFactorySetting::SetLinesNumber(size_t i, unsigned int LNum)
{
    if (i < vLinesNumber.size())
    {
        vLinesNumber[i] = LNum;
    }
}

//============================================================================================================================

unsigned int LotteryFactorySetting::GetLinesNumber(size_t i) const
{
    if (i < vLinesNumber.size())
    {
        return vLinesNumber[i];
    }
    return 0;
}

//============================================================================================================================

void LotteryFactorySetting::SetProduceVelocity(size_t i, unsigned int PVel)
{
    if (i < vProduceVelocity.size())
    {
        vProduceVelocity[i] = PVel;
    }
}

//============================================================================================================================

unsigned int LotteryFactorySetting::GetProduceVelocity(size_t i) const
{
    if (i < vProduceVelocity.size())
    {
        return vProduceVelocity[i];
    }
    return 0;
}

//============================================================================================================================

void LotteryFactorySetting::SetLineTypesNumber(unsigned int nLTNumber)
{
    nLineTypesNumber = nLTNumber;

    vLinesNumber.clear();
    vLinesNumber.resize(nLineTypesNumber);
    vProduceVelocity.clear();
    vProduceVelocity.resize(nLineTypesNumber);
}

//============================================================================================================================

bool LotteryFactorySetting::LoadSettingsFromFile(std::string sFileName)
{
    std::ifstream fin(sFileName);
    if(fin.is_open())
    {
        std::string buf;
        fin >> buf;
        if (buf.compare("LineTypesNumber") == 0)
        {
            size_t n;
            fin >> n;
            if (n > 0)
            {
                SetLineTypesNumber(n);
            }
        }

        ThreadsNumber = 0;
        for(size_t i = 0; i < nLineTypesNumber; i++)
        {
            fin >> buf;
            if (buf.compare("LinesNumber") == 0)
            {
                size_t a, b;
                fin >> a;
                fin >> b;
                SetLinesNumber(a - 1, b);
                ThreadsNumber += b;
            }
        }

        for(size_t i = 0; i < nLineTypesNumber; i++)
        {
            fin >> buf;
            if (buf.compare("ProduceVelocity") == 0)
            {
                size_t a, b;
                fin >> a;
                fin >> b;
                SetProduceVelocity(a - 1, b);
            }
        }

        fin.close();
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "Error", "Can not find settings file: " + QString(sFileName.c_str()));
    }

    return false;
}

//============================================================================================================================
