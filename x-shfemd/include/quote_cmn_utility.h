﻿#if !defined(QUOTE_CMN_UTILITY_H_)
#define QUOTE_CMN_UTILITY_H_

#include <pthread.h>
#include <string>
#include <vector>
#include <set>
#include <string>
#include <float.h>

using namespace std;

#ifndef DLL_PUBLIC
#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#endif

#define MAX_PURE_DBL          (double)9007199254740991.0
#define MIN_PURE_DBL          (double)-9007199254740991.0
#define MAX_PURE_FLT          (double)16777215
#define MIN_PURE_FLT          (double)-16777215
inline bool IsValidDouble(double lfValue)
{
    return ((lfValue > MIN_PURE_DBL) && (lfValue < MAX_PURE_DBL));
}

inline bool IsValidFloat(float lfValue)
{
    return ((lfValue > MIN_PURE_FLT) && (lfValue < MAX_PURE_FLT));
}

inline double InvalidToZeroD(double dVal)
{
    return IsValidDouble(dVal) ? dVal : 0.0;
}

inline float InvalidToZeroF(float fVal)
{
    return IsValidFloat(fVal) ? fVal : 0.0;
}

template<typename DataType>
void MYUTIL_SaveDataToFile(const std::vector<DataType> &datas, int &data_count, FILE * pf)
{
    if (pf && !datas.empty())
    {
        fwrite(&(datas[0]), sizeof(DataType), datas.size(), pf);
        data_count += datas.size();
        fseek(pf, 0, SEEK_SET);
        fwrite(&data_count, sizeof(data_count), 1, pf);
        fseek(pf, 0, SEEK_END);
        fflush(pf);
    }
}

template<typename DataType, typename HeaderType>
void MYUTIL_SaveFileHeader(int data_type, FILE * pf)
{
    if (pf)
    {
        HeaderType header;
        header.data_count = 0;
        header.data_type = short(data_type);
        header.data_length = (short) (sizeof(DataType));
        fwrite(&header, sizeof(HeaderType), 1, pf);
    }
}


typedef std::pair<std::string, unsigned short> IPAndPortNum;
IPAndPortNum ParseIPAndPortNum(const std::string &addr_cfg);

typedef std::pair<std::string, std::string> IPAndPortStr;
IPAndPortStr ParseIPAndPortStr(const std::string &addr_cfg);

/*
 * 从文件file中读取主力合约，并存储到buffer中。
 * 假设主力合约最多20个。
 * 查找主力合约时，从位置0开始查找，遇到第一个空字符串止
 * 调用者需要对buffer清零
 * @return:返回主力合约个数
 */
int32_t LoadDominantContracts(string file, char buffer[20][10]);

/*
* check whether the given contract is dominant.
*/
bool IsDominantImp(const char *contract, char buffer[20][10], int32_t buffer_size);

#endif  //
