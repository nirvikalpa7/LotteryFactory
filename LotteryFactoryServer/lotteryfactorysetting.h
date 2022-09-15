#ifndef LOTTERYFACTORYSETTING_H
#define LOTTERYFACTORYSETTING_H

#include <vector>
#include <fstream>

class LotteryFactorySetting
{
private:

    size_t ThreadsNumber;
    size_t nLineTypesNumber;                    // numbers of formats of lottery tickets
    std::vector<unsigned int> vLinesNumber;     // concurrent lines number for every line type
    std::vector<unsigned int> vProduceVelocity; // speed in ms for every line type

public:

    LotteryFactorySetting() { nLineTypesNumber = 0; ThreadsNumber = 0; }

    void SetLinesNumber(size_t i, unsigned int LNum);
    void SetProduceVelocity(size_t i, unsigned int PVel);

    unsigned int GetProduceVelocity(size_t i) const;
    unsigned int GetLinesNumber(size_t i) const;
    size_t GetThreadsNumber() const { return ThreadsNumber; }
    size_t GetLineTypesNumber() const { return nLineTypesNumber; };

    void SetLineTypesNumber(unsigned int nLTNumber);

    bool LoadSettingsFromFile(std::string sFileName);
};

#endif // LOTTERYFACTORYSETTING_H
