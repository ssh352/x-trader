﻿#include "quote_ctp.h"
#include <iomanip>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#include "quote_cmn_utility.h"

using namespace my_cmn;
using namespace std;

MYCTPDataHandler::MYCTPDataHandler()
{
    api_ = NULL;

    // 初始化
    api_ = CThostFtdcMdApi::CreateFtdcMdApi();
    api_->RegisterSpi(this);

    // set front address
	char addr[50] = "tcp://172.18.32.125:41213";
	api_->RegisterFront(addr);

    api_->Init();
}

MYCTPDataHandler::~MYCTPDataHandler(void)
{
    if (api_)
    {
        api_->RegisterSpi(NULL);
        api_->Release();
        api_ = NULL;
    }
}

void MYCTPDataHandler::OnFrontConnected()
{

    CThostFtdcReqUserLoginField req_login;
    memset(&req_login, 0, sizeof(CThostFtdcReqUserLoginField));
    api_->ReqUserLogin(&req_login, 0);

}

void MYCTPDataHandler::OnFrontDisconnected(int nReason)
{
}

void MYCTPDataHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
    bool bIsLast)
{
    int error_code = pRspInfo ? pRspInfo->ErrorID : 0;

    if (error_code == 0)
    {
        //api_->SubscribeMarketData(pp_instruments_, sub_count_);
    }
    else
    {
        std::string err_str("null");
        if (pRspInfo && pRspInfo->ErrorMsg[0] != '\0')
        {
            err_str = pRspInfo->ErrorMsg;
        }

        // 登录失败
    }
}

void MYCTPDataHandler::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
    int nRequestID, bool bIsLast)
{
    if (bIsLast)
    {
    }
}

void MYCTPDataHandler::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void MYCTPDataHandler::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *p)
{
    try
    {
        timeval t;
        gettimeofday(&t, NULL);

        RalaceInvalidValue_CTP(*p);
        CDepthMarketDataField q_level1 = Convert(*p);

		// TODO: debug
    }
    catch (...)
    {
    }
}

void MYCTPDataHandler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    int error_code = pRspInfo ? 0 : pRspInfo->ErrorID;
    if (error_code != 0)
    {
    }
}

void MYCTPDataHandler::SetQuoteDataHandler(boost::function<void(const CDepthMarketDataField *)> quote_data_handler)
{
    quote_data_handler_ = quote_data_handler;
}

void MYCTPDataHandler::RalaceInvalidValue_CTP(CThostFtdcDepthMarketDataField &d)
{
    d.Turnover = InvalidToZeroD(d.Turnover);
    d.LastPrice = InvalidToZeroD(d.LastPrice);
    d.UpperLimitPrice = InvalidToZeroD(d.UpperLimitPrice);
    d.LowerLimitPrice = InvalidToZeroD(d.LowerLimitPrice);
    d.HighestPrice = InvalidToZeroD(d.HighestPrice);
    d.LowestPrice = InvalidToZeroD(d.LowestPrice);
    d.OpenPrice = InvalidToZeroD(d.OpenPrice);
    d.ClosePrice = InvalidToZeroD(d.ClosePrice);
    d.PreClosePrice = InvalidToZeroD(d.PreClosePrice);
    d.OpenInterest = InvalidToZeroD(d.OpenInterest);
    d.PreOpenInterest = InvalidToZeroD(d.PreOpenInterest);
    d.AveragePrice = InvalidToZeroD(d.AveragePrice);

    d.BidPrice1 = InvalidToZeroD(d.BidPrice1);
//    d.BidPrice2 = InvalidToZeroD(d.BidPrice2);
//    d.BidPrice3 = InvalidToZeroD(d.BidPrice3);
//    d.BidPrice4 = InvalidToZeroD(d.BidPrice4);
//    d.BidPrice5 = InvalidToZeroD(d.BidPrice5);

    d.AskPrice1 = InvalidToZeroD(d.AskPrice1);
//    d.AskPrice2 = InvalidToZeroD(d.AskPrice2);
//    d.AskPrice3 = InvalidToZeroD(d.AskPrice3);
//    d.AskPrice4 = InvalidToZeroD(d.AskPrice4);
//    d.AskPrice5 = InvalidToZeroD(d.AskPrice5);

    d.SettlementPrice = InvalidToZeroD(d.SettlementPrice);
    d.PreSettlementPrice = InvalidToZeroD(d.PreSettlementPrice);

//    d.PreDelta = InvalidToZeroD(d.PreDelta);
//    d.CurrDelta = InvalidToZeroD(d.CurrDelta);
}

CDepthMarketDataField MYCTPDataHandler::Convert(const CThostFtdcDepthMarketDataField &ctp_data)
{
    CDepthMarketDataField quote_level1;
    memset(&quote_level1, 0, sizeof(CDepthMarketDataField));

    memcpy(quote_level1.TradingDay, ctp_data.TradingDay, 9); /// char       TradingDay[9];
    //SettlementGroupID[9];       /// char       SettlementGroupID[9];
    //SettlementID ;        /// int            SettlementID;
    quote_level1.LastPrice = ctp_data.LastPrice;           /// double LastPrice;
    quote_level1.PreSettlementPrice = ctp_data.PreSettlementPrice;  /// double PreSettlementPrice;
    quote_level1.PreClosePrice = ctp_data.PreClosePrice;       /// double PreClosePrice;
    quote_level1.PreOpenInterest = ctp_data.PreOpenInterest;     /// double PreOpenInterest;
    quote_level1.OpenPrice = ctp_data.OpenPrice;           /// double OpenPrice;
    quote_level1.HighestPrice = ctp_data.HighestPrice;        /// double HighestPrice;
    quote_level1.LowestPrice = ctp_data.LowestPrice;         /// double LowestPrice;
    quote_level1.Volume = ctp_data.Volume;              /// int            Volume;
    quote_level1.Turnover = ctp_data.Turnover;            /// double Turnover;
    quote_level1.OpenInterest = ctp_data.OpenInterest;        /// double OpenInterest;
    quote_level1.ClosePrice = ctp_data.ClosePrice;          /// double ClosePrice;
    quote_level1.SettlementPrice = ctp_data.SettlementPrice;     /// double SettlementPrice;
    quote_level1.UpperLimitPrice = ctp_data.UpperLimitPrice;     /// double UpperLimitPrice;
    quote_level1.LowerLimitPrice = ctp_data.LowerLimitPrice;     /// double LowerLimitPrice;
//    quote_level1.PreDelta = ctp_data.PreDelta;            /// double PreDelta;
//    quote_level1.CurrDelta = ctp_data.CurrDelta;           /// double CurrDelta;
    memcpy(quote_level1.UpdateTime, ctp_data.UpdateTime, 9);       /// char       UpdateTime[9]; typedef char TThostFtdcTimeType[9];
    quote_level1.UpdateMillisec = ctp_data.UpdateMillisec;      /// int            UpdateMillisec;
    memcpy(quote_level1.InstrumentID, ctp_data.InstrumentID, 31); /// char       InstrumentID[31]; typedef char TThostFtdcInstrumentIDType[31];
    quote_level1.BidPrice1 = ctp_data.BidPrice1;           /// double BidPrice1;
    quote_level1.BidVolume1 = ctp_data.BidVolume1;          /// int            BidVolume1;
    quote_level1.AskPrice1 = ctp_data.AskPrice1;           /// double AskPrice1;
    quote_level1.AskVolume1 = ctp_data.AskVolume1;          /// int            AskVolume1;
//    quote_level1.BidPrice2 = ctp_data.BidPrice2;           /// double BidPrice2;
//    quote_level1.BidVolume2 = ctp_data.BidVolume2;          /// int            BidVolume2;
//    quote_level1.AskPrice2 = ctp_data.AskPrice2;           /// double AskPrice2;
//    quote_level1.AskVolume2 = ctp_data.AskVolume2;          /// int            AskVolume2;
//    quote_level1.BidPrice3 = ctp_data.BidPrice3;           /// double BidPrice3;
//    quote_level1.BidVolume3 = ctp_data.BidVolume3;          /// int            BidVolume3;
//    quote_level1.AskPrice3 = ctp_data.AskPrice3;           /// double AskPrice3;
//    quote_level1.AskVolume3 = ctp_data.AskVolume3;          /// int            AskVolume3;
//    quote_level1.BidPrice4 = ctp_data.BidPrice4;           /// double BidPrice4;
//    quote_level1.BidVolume4 = ctp_data.BidVolume4;          /// int            BidVolume4;
//    quote_level1.AskPrice4 = ctp_data.AskPrice4;           /// double AskPrice4;
//    quote_level1.AskVolume4 = ctp_data.AskVolume4;          /// int            AskVolume4;
//    quote_level1.BidPrice5 = ctp_data.BidPrice5;           /// double BidPrice5;
//    quote_level1.BidVolume5 = ctp_data.BidVolume5;          /// int            BidVolume5;
//    quote_level1.AskPrice5 = ctp_data.AskPrice5;           /// double AskPrice5;
//    quote_level1.AskVolume5 = ctp_data.AskVolume5;          /// int            AskVolume5;
        //ActionDay[9];        /// char       ActionDay[9];

    return quote_level1;
}
