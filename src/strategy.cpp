#include <sstream>
#include <stdio.h>
#include "strategy.h"

using namespace std;

Strategy::Strategy()
: module_name_("Strategy")
{
	pfn_init_;
	pfn_feedbestanddeep_ = NULL;
	pfn_feedorderstatistic_ = NULL;
	pfn_feedsignalresponse_ = NULL;
	pfn_destroy_ = NULL;
	pfn_feedinitposition_ = NULL;
	this->pfn_destroy_ = NULL;
	pproxy_ = NULL;
}

Strategy::~Strategy(void)
{
	SavePosition();

	if (this->pfn_destroy_ != NULL){
		// TODO:
		//pfn_destroy_ ();
		clog_info("[%s] strategy(id:%d) destroyed", module_name_, this->setting_.config.st_id);
	}
	if (pproxy_ != NULL){
		pproxy_->deleteObject(this->setting_.file);
		pproxy_ = NULL;
	}
}

string Strategy::generate_log_name(char* log_path)
 {
	string log_full_path = "";

	// parse model name
	string model_name = "";
	unsigned found = this->setting_.file.find_last_of("/");
	if(found==string::npos){ model_name = this->setting_.file; }
	else{ model_name = this->setting_.file.substr(found+1); }

	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer,80,"%Y%m%d",timeinfo);

	log_full_path = log_path;
	log_full_path += "/";
	log_full_path += model_name;
	log_full_path += "_";

	log_full_path += buffer;
	log_full_path += ".txt";

	return log_full_path;
}

void Strategy::Init(StrategySetting &setting, CLoadLibraryProxy *pproxy)
{
	// set breakpoint here
	this->setting_ = setting;
	this->pproxy_ = pproxy;

	pfn_init_ = (Init_ptr)pproxy_->findObject(this->setting_.file, STRATEGY_METHOD_INIT);
	if (!pfn_init_){
		clog_info("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, this->setting_.file.c_str(), STRATEGY_METHOD_INIT, errno);
	}

	pfn_feedbestanddeep_ = (FeedBestAndDeep_ptr)pproxy_->findObject(
					this->setting_.file, STRATEGY_METHOD_FEED_MD_BESTANDDEEP);
	if (!pfn_feedbestanddeep_){
		clog_info("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, this->setting_.file.c_str(), STRATEGY_METHOD_FEED_MD_BESTANDDEEP, errno);
	}

	pfn_feedorderstatistic_ = (FeedOrderStatistic_ptr)pproxy_->findObject(
					this->setting_.file, STRATEGY_METHOD_FEED_MD_ORDERSTATISTICS);
	if (!pfn_feedorderstatistic_){
		clog_info("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, this->setting_.file.c_str(), STRATEGY_METHOD_FEED_MD_ORDERSTATISTICS, errno);
	}

	pfn_feedinitposition_ = (FeedInitPosition_ptr)pproxy_->findObject(
				this->setting_.file, STRATEGY_METHOD_FEED_INIT_POSITION);
	if (!pfn_feedinitposition_ ){
		clog_info("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, this->setting_.file.c_str(), STRATEGY_METHOD_FEED_INIT_POSITION, errno);
	}

	pfn_feedsignalresponse_ = (FeedSignalResponse_ptr)pproxy_->findObject(
				this->setting_.file, STRATEGY_METHOD_FEED_SIG_RESP);
	if (!pfn_feedsignalresponse_){
		clog_info("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, this->setting_.file.c_str(), STRATEGY_METHOD_FEED_SIG_RESP, errno);
	}

	pfn_destroy_ = (Destroy_ptr)pproxy_->findObject(
				this->setting_.file, STRATEGY_METHOD_FEED_DESTROY );
	if (!pfn_destroy_){
		clog_info("[%s] findObject failed, file:%s; method:%s; errno:%d", 
					module_name_, this->setting_.file.c_str(), STRATEGY_METHOD_FEED_DESTROY, errno);
	}

	string model_log = generate_log_name(setting_.config.log_name);
	strcpy(setting_.config.log_name, model_log.c_str());
	setting_.config.log_id = setting_.config.st_id;

	memset(&position_, 0, sizeof(StrategyPosition));
	LoadPosition();
	memset(&pos_cache_.s_pos[0], 0, sizeof(symbol_pos_t));
	strcpy(pos_cache_.s_pos[0].symbol, GetSymbol());
	pos_cache_.symbol_cnt = 1;

	int err = 0;
	this->pfn_init_(&this->setting_.config, &err);
}

void Strategy::feed_init_position(strategy_init_pos_t *data,int *sig_cnt, signal_t *sigs)
{
	*sig_cnt = 0;
	this->pfn_feedinitposition_(data,sig_cnt, sigs);
	for (int i = 0; i < *sig_cnt; i++ ){
		sigs[i].st_id = this->setting_.config.st_id;
	}
}

void Strategy::FeedMd(MDBestAndDeep_MY* md, int *sig_cnt, signal_t* sigs)
{
	*sig_cnt = 0;
	this->pfn_feedbestanddeep_(md, sig_cnt, sigs);
	for (int i = 0; i < *sig_cnt; i++ ){
		sigs[i].st_id = this->setting_.config.st_id;
	}
}

void Strategy::FeedMd(MDOrderStatistic_MY* md, int *sig_cnt, signal_t* sigs)
{
	*sig_cnt = 0;
	this->pfn_feedorderstatistic_(md, sig_cnt, sigs);
	for (int i = 0; i < *sig_cnt; i++ ){
		sigs[i].st_id = this->setting_.config.st_id;
	}
}

void Strategy::feed_sig_response(signal_resp_t* rpt, symbol_pos_t *pos, pending_order_t *pending_ord, int *sig_cnt, signal_t* sigs)
{
	*sig_cnt = 0;
	this->pfn_feedsignalresponse_(rpt, pos, pending_ord, sig_cnt, sigs);
	for (int i = 0; i < *sig_cnt; i++ ){
		sigs[i].st_id = this->setting_.config.st_id;
	}
}


int32_t Strategy::GetId()
{
	return this->setting_.config.st_id;
}

const char* Strategy::GetContract()
{
	return this->setting_.config.symbols[0].name;
}

int32_t Strategy::GetMaxPosition()
{
	return this->setting_.config.symbols[0].max_pos;
}

const char* Strategy::GetSoFile()
{
	return this->setting_.file.c_str();
}

long Strategy::GetLocalOrderID(int32_t sig_id)
{
	return sigid_localorderid_map_table_[sig_id];
}

bool Strategy::Deferred(unsigned short sig_openclose, unsigned short int sig_act, int32_t vol, int32_t& updated_vol)
{
	updated_vol = 0;

	if (sig_openclose==alloc_position_effect_t::open_&& sig_act==signal_act_t::buy){
		if (position_.frozen_open_long==0 && position_.cur_long<GetMaxPosition()){
			updated_vol = GetMaxPosition() - position_.cur_long - position_.frozen_open_long;
			return false;
		} else { return true; }
	}
	else if (sig_openclose==alloc_position_effect_t::open_&& sig_act==signal_act_t::sell){
		if (position_.frozen_open_short==0 && position_.cur_short< GetMaxPosition()){
			updated_vol = GetMaxPosition() - position_.cur_short - position_.frozen_open_short;
			return false;
		} else { return true; }
	} else{ clog_error("[%s] Deferred: strategy id:%d; act:%d",module_name_, setting_.config.st_id, sig_act); }

	if (sig_openclose==alloc_position_effect_t::close_&& sig_act==signal_act_t::buy){
		if (position_.frozen_close_short==0 && position_.cur_short>0){
			updated_vol = position_.cur_short - position_.frozen_close_short;
			return false;
		} else { return true; }
	}
	else if (sig_openclose==alloc_position_effect_t::close_&& sig_act==signal_act_t::sell){
		if (position_.frozen_open_long==0 && position_.cur_long>0){
			updated_vol = position_.cur_long - position_.frozen_close_long;
			return false;
		} else { return true; }
	}
	else{ clog_error("[%s] Deferred: strategy id:%d; act:%d",module_name_, setting_.config.st_id, sig_act); }

	if (updated_vol > vol) updated_vol = vol; 

	clog_debug("[%s] Deferred: strategy id:%d; current long:%d; current short:%d; \
				frozen_close_long:%d; frozen_close_short:%d; frozen_open_long:%d; \
				frozen_open_short:%d; updated vol:%d",
				module_name_, setting_.config.st_id, position_.cur_long, position_.cur_short,
				position_.frozen_close_long, position_.frozen_close_short,
				position_.frozen_open_long, position_.frozen_open_short, updated_vol);
} 

void Strategy::PrepareForExecutingSig(long localorderid, signal_t &sig)
{
	// get next cursor
	static int32_t cursor = SIGANDRPT_TABLE_SIZE - 1;
	cursor++;
	if ((cursor % SIGANDRPT_TABLE_SIZE ) == 0){
		cursor = 0;
	}

	// signal
	sig_table_[cursor] = sig;
	
	// signal response
	memset(&(sigrpt_table_[cursor]), 0, sizeof(signal_resp_t));
	sigrpt_table_[cursor].sig_id = sig.st_id;
	sigrpt_table_[cursor].sig_act = sig.sig_act;
	strcpy(sigrpt_table_[cursor].symbol, sig.symbol);

	if (alloc_position_effect_t::open_==sig.sig_openclose){
		sigrpt_table_[cursor].order_volume = sig.open_volume;
	} else if (alloc_position_effect_t::close_==sig.sig_openclose){
		sigrpt_table_[cursor].order_volume = sig.close_volume;
	}

	if (sig.sig_act==signal_act_t::buy){
		sigrpt_table_[cursor].order_price = sig.buy_price;
	} else if (sig.sig_act==signal_act_t::sell){
		sigrpt_table_[cursor].order_price = sig.sell_price;
	}

	// mapping table
	sigid_sigandrptidx_map_table_[sig.st_id] = cursor;
	localorderid_sigandrptidx_map_table_[localorderid] = cursor;
	sigid_localorderid_map_table_[sig.st_id] = localorderid;

	clog_debug("[%s] PrepareForExecutingSig: sig id: %d; cursor,%d; LocalOrderID:%d;",
				module_name_, sig.st_id, cursor, localorderid);
}

void Strategy::FeedTunnRpt(TunnRpt &rpt, int *sig_cnt, signal_t* sigs)
{
	// get signal report by LocalOrderID
	int32_t index = localorderid_sigandrptidx_map_table_[rpt.LocalOrderID];
	signal_resp_t& sigrpt = sigrpt_table_[index];
	signal_t& sig = sig_table_[index];

	// update strategy's position
	UpdatePosition(rpt, sig);
	// fill signal position report by tunnel report
	if (rpt.MatchedAmount > 0){
		FillPositionRpt(rpt, pos_cache_);
	}

	// update signal report
	UpdateSigrptByTunnrpt(sigrpt, rpt);
	pending_order_cache_.req_cnt = 0;

	feed_sig_response(&sigrpt, &pos_cache_.s_pos[0], &pending_order_cache_, sig_cnt, sigs);
}

void Strategy::UpdatePosition(const TunnRpt& rpt, const signal_t& sig)
{
	if (rpt.MatchedAmount > 0){
		if (sig.sig_openclose==alloc_position_effect_t::open_ && sig.sig_act==signal_act_t::buy){
			position_.cur_long += rpt.MatchedAmount;
			position_.frozen_open_long -= rpt.MatchedAmount;
		}
		else if (sig.sig_openclose==alloc_position_effect_t::open_ && sig.sig_act==signal_act_t::sell){
			position_.cur_short += rpt.MatchedAmount;
			position_.frozen_open_short -= rpt.MatchedAmount;
		}
		else if (sig.sig_openclose==alloc_position_effect_t::close_ && sig.sig_act==signal_act_t::buy){
			position_.cur_short -= rpt.MatchedAmount;
			position_.frozen_open_short -= rpt.MatchedAmount;
		}
		else if (sig.sig_openclose==alloc_position_effect_t::close_ && sig.sig_act==signal_act_t::sell){
			position_.cur_long -= rpt.MatchedAmount;
			position_.frozen_open_long -= rpt.MatchedAmount;
		}
	} //end if (rpt.MatchedAmount > 0)

	if (rpt.OrderStatus==X1_FTDC_SPD_CANCELED ||
		rpt.OrderStatus==X1_FTDC_SPD_FILLED ||
		rpt.OrderStatus==X1_FTDC_SPD_PARTIAL_CANCELED ||
		rpt.OrderStatus==X1_FTDC_SPD_ERROR ||
		rpt.OrderStatus==X1_FTDC_SPD_IN_CANCELED){
		if (sig.sig_openclose==alloc_position_effect_t::open_ && sig.sig_act==signal_act_t::buy){
			position_.frozen_open_long = 0;
		}
		else if (sig.sig_openclose==alloc_position_effect_t::open_ && sig.sig_act==signal_act_t::sell){
			position_.frozen_open_short = 0;
		}
		else if (sig.sig_openclose==alloc_position_effect_t::close_ && sig.sig_act==signal_act_t::buy){
			position_.frozen_open_short = 0;
		}
		else if (sig.sig_openclose==alloc_position_effect_t::close_ && sig.sig_act==signal_act_t::sell){
			position_.frozen_open_long = 0;
		}
	}
}

void Strategy::FillPositionRpt(const TunnRpt& rpt, position_t &pos)
{
	memset(&pos.s_pos[0], 0, sizeof(symbol_pos_t));
	pos.symbol_cnt = 1;
	pos.s_pos[0].long_volume = position_.cur_long;
	pos.s_pos[0].short_volume = position_.cur_short;
	pos.s_pos[0].changed = 1;
}
void Strategy::UpdateSigrptByTunnrpt(signal_resp_t& sigrpt,const  TunnRpt& tunnrpt)
{
	sigrpt.exec_volume = tunnrpt.MatchedAmount;
	sigrpt.acc_volume += tunnrpt.MatchedAmount;

	if (tunnrpt.OrderStatus==X1_FTDC_SPD_CANCELED ||
		tunnrpt.OrderStatus==X1_FTDC_SPD_PARTIAL_CANCELED ||
		tunnrpt.OrderStatus==X1_FTDC_SPD_IN_CANCELED){
		sigrpt.killed = sigrpt.order_volume - sigrpt.acc_volume;
	}else{ sigrpt.killed = 0; }

	if (tunnrpt.OrderStatus==X1_FTDC_SPD_ERROR) sigrpt.rejected = sigrpt.order_volume;
	else sigrpt.rejected = 0; 

	sigrpt.error_no = tunnrpt.ErrorID;

	if (tunnrpt.OrderStatus==X1_FTDC_SPD_CANCELED ||
		tunnrpt.OrderStatus==X1_FTDC_SPD_PARTIAL_CANCELED ||
		tunnrpt.OrderStatus==X1_FTDC_SPD_IN_CANCELED){
		sigrpt.status = if_sig_state_t::SIG_STATUS_CANCEL;
	}
	else if(tunnrpt.OrderStatus==X1_FTDC_SPD_FILLED){
		sigrpt.status = if_sig_state_t::SIG_STATUS_SUCCESS;
	}
	else if(tunnrpt.OrderStatus==X1_FTDC_SPD_PARTIAL){
		sigrpt.status = if_sig_state_t::SIG_STATUS_PARTED;
	}
	else if(tunnrpt.OrderStatus==X1_FTDC_SPD_ERROR){
		sigrpt.status = if_sig_state_t::SIG_STATUS_REJECTED;
	}
	else if(tunnrpt.OrderStatus==X1_FTDC_SPD_IN_QUEUE || 
			tunnrpt.OrderStatus==X1_FTDC_SPD_PARTIAL ||
			tunnrpt.OrderStatus==X1_FTDC_SPD_PLACED ||
			tunnrpt.OrderStatus==X1_FTDC_SPD_TRIGGERED){
		sigrpt.status = if_sig_state_t::SIG_STATUS_ENTRUSTED;
	}
	else{
		// log error
	}
}

void Strategy::LoadPosition()
{
	memset(&position_, 0, sizeof(position_));
	// TODO:
}

void Strategy::SavePosition()
{
	// TODO:
}

const char * Strategy::GetSymbol()
{
	return setting_.config.symbols[0].name;
}