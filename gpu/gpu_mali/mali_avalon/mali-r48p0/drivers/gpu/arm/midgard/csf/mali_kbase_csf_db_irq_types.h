/*
 *  For CSF doorbell and IRQ types
 */

#ifndef _KBASE_CSF_DB_IRQ_TYPES_H_
#define _KBASE_CSF_DB_IRQ_TYPES_H_

// Doorbell
#define DOORBELL_EVENT(group, ev_id, x1, x2)    ((group << 24) | (ev_id << 16) | (x1 << 8) | (x2))
#define DOORBELL_GLB_EVENT(ev_id)               DOORBELL_EVENT(DOORBELL_GROUP_A, ev_id, 0, 0)
#define DOORBELL_CSG_EVENT(ev_id, csg)          DOORBELL_EVENT(DOORBELL_GROUP_B, ev_id, csg, 0)
#define DOORBELL_CSI_EVENT(ev_id, csg, csi)     DOORBELL_EVENT(DOORBELL_GROUP_C, ev_id, csg, csi)
#define DOORBELL_USER_EVENT(db_id)              DOORBELL_EVENT(DOORBELL_GROUP_D, db_id)

// Doorbell groups
#define DOORBELL_GROUP_A                        0x01
#define DOORBELL_GROUP_B                        0x02
#define DOORBELL_GROUP_C                        0x03
#define DOORBELL_GROUP_D                        0x04

// Doorbell ev_ids
#define DOORBELL_GLB                            DOORBELL_GLB_EVENT(0)
#define DOORBELL_GLB_HALT                       DOORBELL_GLB_EVENT(1)
#define DOORBELL_GLB_CFG_PROGRESS_TIMER         DOORBELL_GLB_EVENT(2)
#define DOORBELL_GLB_CFG_ALLOC_EN               DOORBELL_GLB_EVENT(3)
#define DOORBELL_GLB_CFG_PWROFF_TIMER           DOORBELL_GLB_EVENT(4)
#define DOORBELL_GLB_PROTM_ENTER                DOORBELL_GLB_EVENT(5)
#define DOORBELL_GLB_PRFCNT_ENABLE              DOORBELL_GLB_EVENT(6)
#define DOORBELL_GLB_PRFCNT_SAMPLE              DOORBELL_GLB_EVENT(7)
#define DOORBELL_GLB_COUNTER_ENABLE             DOORBELL_GLB_EVENT(8)
#define DOORBELL_GLB_PING                       DOORBELL_GLB_EVENT(9)
#define DOORBELL_GLB_FIRMWARE_CONFIG_UPDATE     DOORBELL_GLB_EVENT(10)
#define DOORBELL_GLB_IDLE_ENABLE                DOORBELL_GLB_EVENT(11)
#define DOORBELL_GLB_ITER_TRACE_ENABLE          DOORBELL_GLB_EVENT(12)
#define DOORBELL_GLB_SLEEP                      DOORBELL_GLB_EVENT(13)
#define DOORBELL_GLB_DEBUG_CSF_REQ              DOORBELL_GLB_EVENT(14)
#define DOORBELL_GLB_SYNC_NOTIFY                DOORBELL_GLB_EVENT(23)

#define DOORBELL_CSG_STATE(csg)                 DOORBELL_CSG_EVENT(1, csg)
#define DOORBELL_CSG_EP_CFG(csg)                DOORBELL_CSG_EVENT(2, csg)
#define DOORBELL_CSG_STATUS_UPDATE(csg)         DOORBELL_CSG_EVENT(3, csg)

#define DOORBELL_CSI_STATE(csg, csi)            DOORBELL_CSI_EVENT(1, csg, csi)
#define DOORBELL_CSI_EXTRACT_EVENT(csg, csi)    DOORBELL_CSI_EVENT(2, csg, csi)
#define DOORBELL_CSI_TILER_OOM(csg, csi)        DOORBELL_CSI_EVENT(3, csg, csi)
#define DOORBELL_CSI_FAULT(csg, csi)            DOORBELL_CSI_EVENT(4, csg, csi)

// Doorbell getting information
#define DOORBELL_GET_EVENT_GROUP(event)         ((event >> 24) & 0x0F)
#define DOORBELL_GET_EVENT_ID(event)            ((event >> 16) & 0xFF)
#define DOORBELL_GET_EVENT_CSG(event)           ((event >> 8) & 0xFF)
#define DOORBELL_GET_EVENT_CSI(event)           ((event) & 0xFF)
#define DOORBELL_GET_EVENT_DB_ID(event)         DOORBELL_GET_EVENT_ID(event)


// Job IRQ








#endif /* _KBASE_CSF_DB_IRQ_TYPES_H_ */