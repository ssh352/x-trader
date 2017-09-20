
#ifndef __UNI_CONSUMER_H__
#define __UNI_CONSUMER_H__

#include <functional>
#include <array>
#include <string>
#include <list>
#include <unordered_map>
#include "vrt_value_obj.h"
#include "strategy.h"
#include "md_producer.h"
#include "tunn_rpt_producer.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"
#include "compliance.h"

#define STRA_TABLE_SIZE 100 
#define SIG_BUFFER_SIZE 32 

// key2: stoi(年月)，如1801
#define MAX_STRATEGY_KEY2 3000 
// 品种字符相加：如jd1801，key1: j+d(ascii 值相加
#define MAX_STRATEGY_KEY1 500

struct Uniconfig
{
	// disruptor yield strategy
	char yield[20];
};

class X1FieldConverter
{
	public:
		static void Convert(const signal_t& sig,const char *account, long localorderid, 
					int32_t vol, CX1FtdcInsertOrderField& insert_order)
		{
			memset(&insert_order, 0, sizeof(CX1FtdcInsertOrderField));
			strncpy(insert_order.AccountID, account, sizeof(TX1FtdcAccountIDType));
			insert_order.LocalOrderID = localorderid;
			insert_order.RequestID = localorderid;
			strncpy(insert_order.InstrumentID, sig.symbol, sizeof(TX1FtdcInstrumentIDType));

			if (sig.sig_act == signal_act_t::buy){
				insert_order.InsertPrice = sig.buy_price;
				insert_order.BuySellType = X1_FTDC_SPD_BUY;
			}
			else if (sig.sig_act == signal_act_t::sell){
				insert_order.InsertPrice = sig.sell_price;
				insert_order.BuySellType = X1_FTDC_SPD_SELL;
			}
			else{
				 clog_warning("[%s] do support BuySellType value:%d; sig id:%d", "X1FieldConverter",
					insert_order.BuySellType, sig.sig_id); 
			}

			if (sig.sig_openclose == alloc_position_effect_t::open_){
				insert_order.OpenCloseType = X1_FTDC_SPD_OPEN;
			}
			else if (sig.sig_openclose == alloc_position_effect_t::close_){
				insert_order.OpenCloseType = X1_FTDC_SPD_CLOSE;
			}
			else{
				// log
			}


			if (sig.instr == instr_t::MARKET){
				insert_order.OrderType = X1_FTDC_MKORDER; 
			}
			else{
				insert_order.OrderType = X1_FTDC_LIMITORDER; 
			}

			insert_order.OrderAmount = vol;
			insert_order.OrderProperty = X1_FTDC_SP_NON;
			insert_order.InsertType = X1_FTDC_BASIC_ORDER;        //委托类别,默认为普通订单
			insert_order.InstrumentType = X1FTDC_INSTRUMENT_TYPE_COMM;      //合约类型, 期货
			insert_order.Speculator = X1_FTDC_SPD_SPECULATOR;
		}
};

class UniConsumer
{
	public:
		UniConsumer(struct vrt_queue  *queue, MDProducer *md_producer,
					TunnRptProducer *tunn_rpt_producer);
		~UniConsumer();

		void Start();
		void Stop();

	private:
		bool running_;
		const char* module_name_;  
		struct vrt_consumer *consumer_;
		MDProducer *md_producer_;
		TunnRptProducer *tunn_rpt_producer_;
		CLoadLibraryProxy *pproxy_;
		int32_t strategy_counter_;

		Strategy stra_table_[STRA_TABLE_SIZE];

#if FIND_STRATEGIES == 1
		// unordered_multimap  key: contract; value: indices of strategies in stra_table_
		std::unordered_multimap<std::string, int32_t> cont_straidx_map_table_;
#endif

#if FIND_STRATEGIES == 2
		// two-dimensional array
		int32_t stra_idx_table_[STRA_TABLE_SIZE][STRA_TABLE_SIZE];
		int32_t cont_straidx_map_table_[MAX_STRATEGY_KEY1][MAX_STRATEGY_KEY2];
#endif

		// key: strategy id; value: index of strategy in stra_table_
		int32_t straid_straidx_map_table_[STRA_TABLE_SIZE];

		std::list<StrategySetting> strategy_settings_;
		StrategySetting CreateStrategySetting(const TiXmlElement *ele);
		void ParseConfig();
		void CreateStrategies();
		void GetKeys(const char* contract, int &key1, int &key2);
		int32_t GetEmptyNode();

		// business logic
		void ProcBestAndDeep(int32_t index);
		void FeedBestAndDeep(int32_t straidx);
		void ProcOrderStatistic(int32_t index);
		void ProcSigs(Strategy &strategy, int32_t sig_cnt, signal_t *sigs);
		void ProcTunnRpt(int32_t index);
		void CancelOrder(Strategy &strategy,signal_t &sig);
		void PlaceOrder(Strategy &strategy, const signal_t &sig);
		signal_t sig_buffer_[SIG_BUFFER_SIZE];
		Uniconfig config_;

		/*
		 *		 * pending_signals_[st_id][n]:pending_signals_[st_id]存储st_id
		 *				 * 策略待处理的信号的信号id。-1表示无效
		 *						 *
		 *								 */
		int32_t pending_signals_[200][2];

#ifdef COMPLIANCE_CHECK
		Compliance compliance_;
#endif
		
};

#endif

