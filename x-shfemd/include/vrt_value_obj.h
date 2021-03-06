#ifndef __VRT_VALUE_OBJ_H_
#define __VRT_VALUE_OBJ_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include "quote_datatype_shfe_deep.h"

#define gettid() syscall(__NR_gettid)

#define CLOG_CHANNEL  "x-shmd"

/* Note that the parameter for queue size is a power of 2. */
#define  QUEUE_SIZE  8192

#define PERSISTENCE_ENABLED

//#define LATENCY_MEASURE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
	#include <inttypes.h>		 
	#include <bowsprit.h>
	#include <libcork/core.h>
	#include <libcork/ds.h>
	#include <libcork/helpers/errors.h>
	#include <clogger.h>
	#include <vrt.h>

#ifdef __cplusplus
	};
#endif /* __cplusplus */

	class MDPackEx
	{
		public:
			MDPackEx(): damaged(false) { }

			MDPackEx(MDPack &cur_content): damaged(false)
			{
				this->content = cur_content;
			}

			MDPack content;
			bool damaged;
	};

	/* --------------------------------------------------------------
	 * x-shmd varon-t value and type
	 */

	enum HybridData {
		L1_MD = 0, 
		FULL_DEPTH_MD 
	};

	struct vrt_hybrid_value {
		struct vrt_value  parent;
		enum HybridData data;
		int32_t index;
	};

	static struct vrt_value *
	vrt_hybrid_value_new(struct vrt_value_type *type)
	{
		struct vrt_hybrid_value *self = (struct vrt_hybrid_value*)cork_new(struct vrt_hybrid_value);
		return &self->parent;
	}

	static void
	vrt_hybrid_value_free(struct vrt_value_type *type, struct vrt_value *vself)
	{
		struct vrt_hybrid_value *iself =
			cork_container_of(vself, struct vrt_hybrid_value, parent);
		free(iself);
	}

	static struct vrt_value_type  _vrt_hybrid_value_type = {
		vrt_hybrid_value_new,
		vrt_hybrid_value_free
	};

	static struct vrt_value_type *
	vrt_hybrid_value_type(void)
	{
		return &_vrt_hybrid_value_type;
	}


#endif
