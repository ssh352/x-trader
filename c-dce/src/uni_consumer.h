
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
#include "pending_sig_producer.h"
#include <tinyxml.h>
#include <tinystr.h>
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"

// 最大支持的策略数
#define STRA_TABLE_SIZE 512 

// 最大缓冲的信号数
#define SIG_BUFFER_SIZE 32 

// key2: stoi(年月)，如1801
#define MAX_STRATEGY_KEY2 3000 
// 品种字符相加：如jd1801，key1: j+d(ascii 值相加
#define MAX_STRATEGY_KEY1 500

// 通过合约查找订阅该合约行情的方法:
// 1: unordered_multimap  
// 2: two-dimensional array
// 3: strcmp
#define FIND_STRATEGIES 2

// need to be changed
class TunnelFieldConverter
{
	public:
		static void Convert(const signal_t& sig,long localorderid, int32_t vol, T_PlaceOrder &insert_order)
		{
			insert_order.serial_no = localorderid;
			strncpy(insert_order.stock_code, sig.symbol, sizeof(StockCodeType));

			if (sig.sig_act == signal_act_t::buy){
				insert_order.limit_price = sig.buy_price;
				insert_order.direction = MY_TNL_D_BUY;
			}
			else if (sig.sig_act == signal_act_t::sell){
				insert_order.limit_price = sig.sell_price;
				insert_order.direction = MY_TNL_D_SELL;
			}

			if (sig.sig_openclose == alloc_position_effect_t::open_){
				insert_order.open_close = MY_TNL_D_OPEN;
			}
			else if (sig.sig_openclose == alloc_position_effect_t::close_){
				insert_order.open_close = MY_TNL_D_CLOSE;
			}

			if (sig.instr == instr_t::MARKET){
				insert_order.order_kind = MY_TNL_OPT_ANY_PRICE;
			}
			else{
				insert_order.order_kind = MY_TNL_OPT_LIMIT_PRICE;
			}

			insert_order.volume = vol;
			insert_order.speculator = MY_TNL_HF_SPECULATION;
			insert_order.order_type = MY_TNL_HF_NORMAL;
			insert_order.exchange_type = MY_TNL_EC_DCE;
		}
};

class UniConsumer
{
	public:
		UniConsumer(struct vrt_queue  *queue, MDProducer *md_producer,
					TunnRptProducer *tunn_rpt_producer,
					PendingSigProducer *pendingsig_producer);
		~UniConsumer();

		void Start();
		void Stop();

	private:
		bool running_;
		const char* module_name_;  
		struct vrt_consumer *consumer_;
		MDProducer *md_producer_;
		TunnRptProducer *tunn_rpt_producer_;
		PendingSigProducer *pendingsig_producer_;
		CLoadLibraryProxy *pproxy_;
		int32_t strategy_counter_;

		Strategy stra_table_[STRA_TABLE_SIZE];

#if FIND_STRATEGIES == 1
		// unordered_multimap  key: contract; value: indices of strategies in stra_table_
		//std::unordered_multimap<std::string, int32_t> cont_straidx_map_table_;
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
		void ProcPendingSig(int32_t index);
		void ProcSigs(Strategy &strategy, int32_t sig_cnt, signal_t *sigs);
		void ProcTunnRpt(int32_t index);
		void CancelOrder(Strategy &strategy,signal_t &sig);
		void PlaceOrder(Strategy &strategy, const signal_t &sig);
		signal_t sig_buffer_[SIG_BUFFER_SIZE];
		
};

#endif

