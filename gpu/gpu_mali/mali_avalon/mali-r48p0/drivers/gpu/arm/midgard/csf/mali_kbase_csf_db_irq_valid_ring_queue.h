/*
 *  CSF doorbell and IRQ declarations
 */

#ifndef _KBASE_CSF_DB_IRQ_VALID_RING_QUEUE_H_
#define _KBASE_CSF_DB_IRQ_VALID_RING_QUEUE_H_

#include "mali_kbase_csf_db_valid_defs.h"


#define RING_QUEUE_SIZE                     256
#define RING_QUEUE_HSIZE                    (RING_QUEUE_SIZE >> 1)

#define RING_QUEUE_GLB_INPUT_OFFSET         DBVALID_IFACE_IO_PAGE_RING_QUEUE
#define RING_QUEUE_GLB_OUTPUT_OFFSET        DBVALID_IFACE_IO_PAGE_RING_QUEUE

#define RING_QUEUE_GLB_INPUT(idx)           (RING_QUEUE_GLB_INPUT_OFFSET + ((idx) << 2))
#define RING_QUEUE_GLB_OUTPUT(idx)          (RING_QUEUE_GLB_OUTPUT_OFFSET + ((idx) << 2))

#define USER_DB_BASE_SEQ_OFFSET             0x0010
#define USER_DB_KBASE_SEQ_OFFSET            0x0020

#endif /* _KBASE_CSF_DB_IRQ_VALID_RING_QUEUE_H_ */