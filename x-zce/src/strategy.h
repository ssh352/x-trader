#pragma once

#include <list>
#include <string>
#include <vector>
#include <dlfcn.h>
#include <atomic>         // std::atomic_flag
#include <memory>
#include "signal.h"
#include "moduleloadlibrarylinux.h"
#include "loadlibraryproxy.h"
#include "quote_datatype_czce_level2.h"
#include "tunn_rpt_producer.h"

using namespace std;

#define STRATEGY_METHOD_INIT "st_init_"
#define STRATEGY_METHOD_FEED_MD_L2QUOTESNAPSHOT "st_feed_marketinfo_8_"
#define STRATEGY_METHOD_FEED_SIG_RESP "st_feed_sig_resp_"
#define STRATEGY_METHOD_FEED_DESTROY "st_destroy_"
#define STRATEGY_METHOD_FEED_INIT_POSITION  "st_feed_init_position_"

// 假设一个策略最多产生3000个信号
#define SIGANDRPT_TABLE_SIZE 1500

// 一个x-trader最多支持100个策略
#define MAX_STRATEGY_COUNT 100

#define MAX_LINES_FOR_LOG 3500

struct strat_out_log
{
    int exch_time; //exchange time
    char contract[16]; //contract
    int n_tick;
    double price;
    int vol;
    int bv1;
    double bp1;
    double sp1;
    int sv1;
    long amt;
    long oi;
    double buy_price, sell_price;
    int open_vol, close_vol;
    int long_pos, short_pos;
    int tot_ordervol, tot_cancelvol;
    int order_cnt, cancel_cnt;
    double cash, live;
    int tot_vol;
    double max_dd;
    int max_net_pos, max_side_pos;
    double sig[12];
};

struct StrategyPosition
{
	// long position
	int32_t cur_long;
	// short position
	int32_t cur_short;
	int32_t frozen_close_long;
	int32_t frozen_close_short;
	int32_t frozen_open_long;
	int32_t frozen_open_short;
};

struct StrategySetting
{
public:
	// the configuration to tell model
	st_config_t config;
	string file;
};

class Strategy	
{
public:
	typedef void (* Init_ptr)(st_config_t *config, int *ret_code, struct strat_out_log *log);
	typedef void ( *FeedL2QuoteSnapshot_ptr)(ZCEL2QuotSnapshotField_MY* md, int *sig_cnt, signal_t* signals, struct strat_out_log *log);	
	typedef void ( *FeedSignalResponse_ptr)(signal_resp_t* rpt, symbol_pos_t *pos, int *sig_cnt, signal_t* sigs, struct strat_out_log *log);
	typedef void (*Destroy_ptr)();
	typedef void (*FeedInitPosition_ptr)(strategy_init_pos_t *data, struct strat_out_log *log);

public:
	Strategy();
	virtual ~Strategy(void);

	// things relating to strategy interface
	void Init(StrategySetting &setting, CLoadLibraryProxy *pproxy);
	void FeedInitPosition();
	void FeedMd(ZCEL2QuotSnapshotField_MY* md, int *sig_cnt, signal_t* signals);
	void feed_sig_response(signal_resp_t* rpt, symbol_pos_t *pos, int *sig_cnt, signal_t* sigs);

	// things relating to x-trader internal logic
	void finalize(void);
	int32_t GetId();
	const char* GetContract();
	exchange_names GetExchange();
	int32_t GetMaxPosition();
	const char* GetSoFile();
	long GetLocalOrderID(int32_t sig_id);
	bool Deferred(int sig_id, unsigned short sig_openclose, unsigned short int sig_act);
	void PrepareForExecutingSig(long localorderid, const signal_t &sig, int32_t actual_vol);
	void FeedTunnRpt(TunnRpt &rpt, int *sig_cnt, signal_t* sigs);
	bool HasFrozenPosition();
	int32_t GetCounterByLocalOrderID(long local_ord_id);
	signal_t* GetSignalBySigID(int32_t sig_id);
	void Push(const signal_t &sig);
	int GetAvailableVol(int sig_id, unsigned short sig_openclose, unsigned short int sig_act, int32_t vol);
	int GetVol(const signal_t &sig);
	void End(void);

	// log
	/*
	 * isEnded:true,表示写完日之后，退出写日志线程
	 */ 
	void WriteLog(bool isEnded);
	void WriteLogImp();
	void WriteOne(FILE *pfDayLogFile, struct strat_out_log *pstratlog);
	void WriteLogTitle();

private:
	string generate_log_name(char * log_path);

	// things relating to strategy interface
	Init_ptr pfn_init_;
	FeedL2QuoteSnapshot_ptr pfn_feedl2quotesnapshot_;
	FeedSignalResponse_ptr pfn_feedsignalresponse_;
	Destroy_ptr pfn_destroy_;
	FeedInitPosition_ptr pfn_feedinitposition_;

	long GetLocalOrderIDByCounter(long counter);
	int cursor_;
	signal_t sig_table_[SIGANDRPT_TABLE_SIZE];
	signal_resp_t sigrpt_table_[SIGANDRPT_TABLE_SIZE];

	// key: LocalOrderID的counter部分; value:信号和报告所在数组的索引。
	// counter是全局累加的，所以数组长度：一个策略最大信号苏沪 * 支持的最大策略数 
	int localorderid_sigandrptidx_map_table_[SIGANDRPT_TABLE_SIZE * MAX_STRATEGY_COUNT]; 

	// key: signal id; value: LocalOrderID
	long sigid_localorderid_map_table_[SIGANDRPT_TABLE_SIZE];

	// key: signal id; value: 号所存数组的位置
	long sigid_sigidx_map_table_[SIGANDRPT_TABLE_SIZE];

	position_t pos_cache_;
	
	// log
	std::atomic_flag lock_log_;
	FILE * pfDayLogFile_;
	vector<strat_out_log> log_;
	int32_t log_cursor_;
	vector<strat_out_log> log_w_;
	int32_t log_write_count_;
	std::thread *thread_log_;
	bool log_ended_;

	// be used to check whether the stategy is valid
	bool valid_;

	CLoadLibraryProxy *pproxy_;
	StrategySetting setting_;
	const char *module_name_;  
	StrategyPosition position_;
	void LoadPosition();
	void SavePosition();
	/*
	 * 
	 */
	void UpdateSigrptByTunnrpt(signal_resp_t& sigrpt, const TunnRpt& tunnrpt);
	void UpdatePosition(const TunnRpt& rpt, unsigned short sig_openclose,
				unsigned short int sig_act);
	void FillPositionRpt(const TunnRpt& rpt, position_t& pos);
	const char * GetSymbol();
	bool Freeze(unsigned short sig_openclose, unsigned short int sig_act, int32_t updated_vol);
};

