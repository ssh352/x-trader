﻿#include "quote_interface_shfe_my.h"

#include <string>
#include <thread>         // std::thread
#include "my_cmn_util_funcs.h"
#include "quote_cmn_utility.h"
#include "quote_shfe_my.h"
#include "vrt_value_obj.h"
#include "md_producer.h"

using namespace std;
using namespace my_cmn;

/* Note that the parameter for queue size is a power of 2. */
#define  QUEUE_SIZE  4096

// TODO:因消费者需要合并一档与全挡数据，所以需要考虑消费
// 者如何引用缓存在一档生产者的数据，一档数据与全挡数据合并后，需清空
// 一档行情缓存列表，合约映射 index（一档生产者缓存中的索引）


// TODO: 该类为消费者
MYQuoteData::MYQuoteData(const SubscribeContracts *subscribe,const string &provider_config)
	: module_name_("uni_consumer"),running_(true),seq_no_(0),server_(0)
{
	// clog init
	clog_set_minimum_level(CLOG_LEVEL_WARNING);
	clog_fp_ = fopen("./x-shfemd.log","w+");
	struct clog_handler *clog_handler = clog_stream_handler_new_fp(fp, true, "%l %m");
	clog_handler_push_process(clog_handler);

	ParseConfig();
	clog_warning("[%s] yield:%s", module_name_, config_.yield); 

	// disruptor init
	rip_check(queue_ = vrt_queue_new("x-shfemd queue",vrt_hybrid_value_type(),QUEUE_SIZE));
	fulldepth_md_producer_ = new FullDepthMDProducer(queue_);
	l1_md_producer_ = new L1MDProducer(queue_);
	consumer_ = vrt_consumer_new(module_name_, queue_);
	if(strcmp(config_.yield, "threaded") == 0){
		consumer_ ->yield = vrt_yield_strategy_threaded();
	}else if(strcmp(config_.yield, "spin") == 0){
		consumer_ ->yield = vrt_yield_strategy_spin_wait();
	}else if(strcmp(config_.yield, "hybrid") == 0){
		consumer_ ->yield = vrt_yield_strategy_hybrid();
	}

	for (int i=0; i<10; i++) repairers[i].server_ = i;

	consumer_thread_ = new thread(MYQuoteData::Start);
	consumer_thread_->detach();
}

MYQuoteData::~MYQuoteData()
{
	Stop();

	vrt_queue_free(queue);
	delete fulldepth_md_producer_;
	delete l1_md_producer_;

	fflush (clog_fp_);
	clog_handler_free(clog_handler);
}

void MYQuoteData::SetQuoteDataHandler(std::function<void(const SHFEQuote*)> quote_handler)
{
	// do not support
}

void MYQuoteData::SetQuoteDataHandler(std::function<void(const CDepthMarketDataField*)> quote_handler)
{
	// do not support
}

void MYQuoteData::SetQuoteDataHandler(std::function<void(const MYShfeMarketData*)> quote_handler)
{
	fulldepthmd_handler_ = quote_handler;
}

// TODO:改成从disruptor中接收并处理数据
// 在repairer等地方对MDPack数据，采用直接引用生产者缓存的数据，而不能再赋值一份
void Proc()
{
    while (!ended_){
        MDPack *p = (MDPack *)recv_buf;
    } // while (!ended_)
}

void UniConsumer::ParseConfig()
{
	std::string config_file = "x-trader.config";
	TiXmlDocument doc = TiXmlDocument(config_file.c_str());
    doc.LoadFile();
    TiXmlElement *root = doc.RootElement();    

	// yield strategy
    TiXmlElement *comp_node = root->FirstChildElement("Disruptor");
	if (comp_node != NULL){
		strcpy(config_.yield, comp_node->Attribute("yield"));
		clog_warning("[%s] yield:%s", module_name_, config_.yield); 
	} else { clog_error("[%s] x-trader.config error: Disruptor node missing.", module_name_); }
}

void MYQuoteData::Start()
{
	clog_debug("[%s] thread id:%ld", module_name_,std::this_thread::get_id() );

	running_  = true;
	int rc = 0;
	struct vrt_value *vvalue;
	while (running_ &&
		   (rc = vrt_consumer_next(consumer_, &vvalue)) != VRT_QUEUE_EOF) {
		if (rc == 0) {
			struct vrt_hybrid_value *ivalue = cork_container_of(vvalue, struct vrt_hybrid_value,
						parent);
			switch (ivalue->data){
				case L1_MD:
					ProcL1MdData(ivalue->index);
					break;
				case FULL_DEPTH_MD:
					ProcFullDepthData(ivalue->index);
					break;
				default:
					clog_error("[%s] [start] unexpected index: %d", module_name_, ivalue->index);
					break;
			}
		}
	} // end while (running_ &&

	if (rc == VRT_QUEUE_EOF) {
		clog_warning("[%s] [start] rev EOF.", module_name_);
	}
	clog_warning("[%s] start exit.", module_name_);
}

void UniConsumer::Stop()
{
	if(running_){
		running_ = false;
		fulldepth_md_producer_->End();
		l1_md_producer_->End();
		clog_warning("[%s] End exit", module_name_);
	}
}

// done:
void UniConsumer::ProcFullDepthData(int32_t index)
{
	MDPackEx* md = fulldepth_md_producer_->GetData(index);
	int new_svr = p->content.seqno % 10;
	if (new_svr != server_) { 
		clog_info("[%s] server from %d to %d",module_name_ server_, new_svr); 
	}

	repairers[new_svr].rev(index);

	bool empty = true;
	char cur_contract[10];
	strcpy(cur_contract,"");
	char new_contract[10];
	strcpy(new_contract,"");
	Reset();
	MDPackEx* data = repairers[new_svr].next(empty);
	while (!empty) { 
		if(strcmp(cur_contract) == ""){
			strcpy(cur_contract,data->instrument);
		}
		strcpy(new_contract,data->instrument);

		if(strcmp(cur_contract,new_contract) != 0){
			FillFullDepthInfo();
			Send(cur_contract);
			Reset();
		}
		
		// 别放到买卖对用缓冲，待该合约的数据都接受完后，统一处理
		if(data->direction == SHFE_FTDC_D_Buy){
			buy_data_cursor_ = buy_data_cursor_ + 1;
			buy_data_buffer_[buy_data_cursor_] = data;
		}else if(data->direction == SHFE_FTDC_D_Sell){
			sell_data_cursor_ = sell_data_cursor_ + 1;
			sell_data_buffer_[sell_data_cursor_] = data;
		}
		strcpy(cur_contract,data->Instrument);

		data = repairers[new_svr].next(empty);
		if(empty){ 
			FillFullDepthInfo();
			Send(cur_contract); 
			Reset();
		}
	}

	server_ = new_svr;
}

// TODO: to here
void MYQuoteData::FillFullDepthInfo(MYShfeMarketData &target,MDPackEx &src)
{
	// new data, copy 30 elements at the end on 2017-06-25
	int buy_cnt = std::min(MY_SHFE_QUOTE_PRICE_POS_COUNT, src.buy_count);
	if (buy_cnt == MY_SHFE_QUOTE_PRICE_POS_COUNT){
		int price_num = buy_cnt * sizeof(double);
		memcpy(my_data.buy_price, src.buy_price + (src.buy_count - buy_cnt),price_num);
		int vol_num = buy_cnt * sizeof(int);
		memcpy(my_data.buy_volume, src.buy_volume + (src.buy_count-buy_cnt),vol_num);
	}else{
		int price_num = src.buy_price,buy_cnt * sizeof(double);
		memcpy(my_data.buy_price + (MY_SHFE_QUOTE_PRICE_POS_COUNT-buy_cnt),price_num);
		int vol_num = src.buy_volume, buy_cnt * sizeof(int);
		memcpy(my_data.buy_volume + (MY_SHFE_QUOTE_PRICE_POS_COUNT-buy_cnt), vol_num);
	}

	int sell_cnt = std::min(MY_SHFE_QUOTE_PRICE_POS_COUNT,src.sell_count);
	if (sell_cnt==MY_SHFE_QUOTE_PRICE_POS_COUNT){
		int price_num = sell_cnt * sizeof(double);
		memcpy(my_data.sell_price,src.sell_price+(src.sell_count-sell_cnt),price_num);
		int vol_num = sell_cnt * sizeof(int);
		memcpy(my_data.sell_volume,src.sell_volume+(src.sell_count-sell_cnt),vol_num);
	}else{
		int price_num = src.sell_price, sell_cnt * sizeof(double);
		memcpy(my_data.sell_price+(MY_SHFE_QUOTE_PRICE_POS_COUNT-sell_cnt),price_num);
		int vol_num = src.sell_volume, sell_el_cpy_cnt * sizeof(int);
		memcpy(my_data.sell_volume+(MY_SHFE_QUOTE_PRICE_POS_COUNT-sell_cnt),vol_num);
	}

    FillStatisticFields(my_data, src);
}

// done
void MYQuoteData::Send(const char* contract)
{
	// 合并一档行情
	CDepthMarketDataField* l1_md = l1_md_producer_->GetLastData(contract);
	if(NULL != l1_md){
		target_data_.data_flag = 6; 
		memcpy(&target_data_, l1_md, sizeof(CDepthMarketDataField));
	} else target_data_.data_flag = 5; 

	// 发给数据客户
	if (fulldepthmd_handler_ != NULL) { fulldepthmd_handler_(&target_md_); }
}

// done
void UniConsumer::ProcL1MdData(int32_t index)
{
	CDepthMarketDataField* md = md_producer_->GetData(index);

	memcpy(target_md_.InstrumentID, md->InstrumentID, sizeof(target_md_.InstrumentID));
	memcpy(&target_md_, md, sizeof(CDepthMarketDataField));
	target_md_.data_flag = 1;
	// 发给数据客户
	if (fulldepthmd_handler_ != NULL) { fulldepthmd_handler_(&target_md_); }
}

// done
void UniConsumer::Reset()
{
	memset(target_md_.buy_price, 0, sizeof(target_md_.buy_price));
	memset(target_md_.buy_volume, 0, sizeof(target_md_.buy_volume));
	memset(target_md_.sell_price, 0, sizeof(target_md_.sell_price));
	memset(target_md_.sell_volume, 0, sizeof(target_md_.sell_volume));

	buy_data_cursor_ = -1;
	sell_data_cursor_ = -1;
}
