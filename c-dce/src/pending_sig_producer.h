#ifndef __PENDING_SIG_PRODUCER_H__
#define __PENDING_SIG_PRODUCER_H__

#include <functional>
#include <array>
#include <string>
#include "vrt_value_obj.h"
#include "signal.h"

using namespace std;

/*
 * 10 power of 2
 */
#define PENDINGSIG_BUFFER_SIZE 2048 

class PendingSigProducer
{
	public:
		PendingSigProducer(struct vrt_queue  *queue);
		~PendingSigProducer();

		signal_t* GetSignal(int32_t index);

		void End();
		void Publish(const signal_t& sig);

	private:
		int32_t Push(const signal_t& sig);

		struct vrt_producer  *producer_;
		std::array<signal_t, PENDINGSIG_BUFFER_SIZE> sig_buffer_;

		const char* module_name_;  
		bool ended_;
};

#endif
