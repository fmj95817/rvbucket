#ifndef SPEC_IO_UFSHC_H
#define SPEC_IO_UFSHC_H

#include "base/def.h"

#define UFSHC_REG_SIZE              0x2000u
#define UFSHC_MAX_QNUM              4u
#define UFSHC_BLOCK_SHIFT           12u
#define UFSHC_BLOCK_SIZE            (1u << UFSHC_BLOCK_SHIFT)
#define UFSHC_STORAGE_SIZE          (64u * MiB)
#define UFSHC_MAX_PRDT              64u
#define UFSHC_MAX_XFER              (1024u * KiB)
#define UFSHC_MAX_QUERY_DATA_SIZE   256u
#define UFSHC_CDB_SIZE              16u
#define UFSHC_SENSE_SIZE            18u

#define UFSHC_IMAGE_MAGIC           "RVBUFS01"
#define UFSHC_IMAGE_MAGIC_SIZE      8u
#define UFSHC_IMAGE_VERSION         1u
#define UFSHC_IMAGE_HEADER_SIZE     UFSHC_BLOCK_SIZE

#define UFSHC_A_CAP                 0x000u
#define UFSHC_A_MCQCAP              0x004u
#define UFSHC_A_VER                 0x008u
#define UFSHC_A_HCPID               0x010u
#define UFSHC_A_HCMID               0x014u
#define UFSHC_A_AHIT                0x018u
#define UFSHC_A_IS                  0x020u
#define UFSHC_A_IE                  0x024u
#define UFSHC_A_HCS                 0x030u
#define UFSHC_A_HCE                 0x034u
#define UFSHC_A_UECPA               0x038u
#define UFSHC_A_UECDL               0x03cu
#define UFSHC_A_UECN                0x040u
#define UFSHC_A_UECT                0x044u
#define UFSHC_A_UECDME              0x048u
#define UFSHC_A_UTRIACR             0x04cu
#define UFSHC_A_UTRLBA              0x050u
#define UFSHC_A_UTRLBAU             0x054u
#define UFSHC_A_UTRLDBR             0x058u
#define UFSHC_A_UTRLCLR             0x05cu
#define UFSHC_A_UTRLRSR             0x060u
#define UFSHC_A_UTRLCNR             0x064u
#define UFSHC_A_UTMRLBA             0x070u
#define UFSHC_A_UTMRLBAU            0x074u
#define UFSHC_A_UTMRLDBR            0x078u
#define UFSHC_A_UTMRLCLR            0x07cu
#define UFSHC_A_UTMRLRSR            0x080u
#define UFSHC_A_UICCMD              0x090u
#define UFSHC_A_UCMDARG1            0x094u
#define UFSHC_A_UCMDARG2            0x098u
#define UFSHC_A_UCMDARG3            0x09cu
#define UFSHC_A_CCAP                0x100u
#define UFSHC_A_CONFIG              0x300u
#define UFSHC_A_MCQCONFIG           0x380u
#define UFSHC_A_ESILBA              0x384u
#define UFSHC_A_ESIUBA              0x388u

#define UFSHC_MCQ_QCFGPTR           2u
#define UFSHC_MCQ_REG_BASE          (UFSHC_MCQ_QCFGPTR * 0x200u)
#define UFSHC_MCQ_REG_STRIDE        0x40u
#define UFSHC_MCQ_OP_BASE           0x1000u
#define UFSHC_MCQ_OP_STRIDE         0x30u

#define UFSHC_MCQ_A_SQATTR          0x00u
#define UFSHC_MCQ_A_SQLBA           0x04u
#define UFSHC_MCQ_A_SQUBA           0x08u
#define UFSHC_MCQ_A_SQDAO           0x0cu
#define UFSHC_MCQ_A_SQISAO          0x10u
#define UFSHC_MCQ_A_SQCFG           0x14u
#define UFSHC_MCQ_A_CQATTR          0x20u
#define UFSHC_MCQ_A_CQLBA           0x24u
#define UFSHC_MCQ_A_CQUBA           0x28u
#define UFSHC_MCQ_A_CQDAO           0x2cu
#define UFSHC_MCQ_A_CQISAO          0x30u
#define UFSHC_MCQ_A_CQCFG           0x34u

#define UFSHC_OP_A_SQHP             0x00u
#define UFSHC_OP_A_SQTP             0x04u
#define UFSHC_OP_A_SQRTC            0x08u
#define UFSHC_OP_A_SQCTI            0x0cu
#define UFSHC_OP_A_SQRTS            0x10u
#define UFSHC_OP_A_SQIS             0x14u
#define UFSHC_OP_A_SQIE             0x18u
#define UFSHC_OP_A_CQHP             0x1cu
#define UFSHC_OP_A_CQTP             0x20u
#define UFSHC_OP_A_CQIS             0x24u
#define UFSHC_OP_A_CQIE             0x28u
#define UFSHC_OP_A_CQIACR           0x2cu

#define UFSHC_CAP_NUTRS_SHIFT       0u
#define UFSHC_CAP_RTT_SHIFT         8u
#define UFSHC_CAP_NUTMRS_SHIFT      16u
#define UFSHC_CAP_64AS              (1u << 24u)
#define UFSHC_CAP_MCQS              (1u << 30u)

#define UFSHC_MCQCAP_MAXQ_SHIFT     0u
#define UFSHC_MCQCAP_RRP            (1u << 9u)
#define UFSHC_MCQCAP_QCFGPTR_SHIFT  16u

#define UFSHC_MCQCONFIG_MAC_SHIFT   8u

#define UFSHC_IS_UTRCS              (1u << 0u)
#define UFSHC_IS_UPMS               (1u << 4u)
#define UFSHC_IS_UHXS               (1u << 5u)
#define UFSHC_IS_UHES               (1u << 6u)
#define UFSHC_IS_UCCS               (1u << 10u)
#define UFSHC_IS_CQES               (1u << 20u)
#define UFSHC_INTR_MASK             0x001f1fffu

#define UFSHC_HCS_DP                (1u << 0u)
#define UFSHC_HCS_UTRLRDY           (1u << 1u)
#define UFSHC_HCS_UTMRLRDY          (1u << 2u)
#define UFSHC_HCS_UCRDY             (1u << 3u)
#define UFSHC_HCS_UPMCRS_SHIFT      8u
#define UFSHC_HCE_HCE               (1u << 0u)

#define UFSHC_SQATTR_SIZE_MASK      0x0000ffffu
#define UFSHC_SQATTR_CQID_SHIFT     16u
#define UFSHC_SQATTR_CQID_MASK      0x00ff0000u
#define UFSHC_SQATTR_SQEN           (1u << 31u)
#define UFSHC_CQATTR_SIZE_MASK      0x0000ffffu
#define UFSHC_CQATTR_CQEN           (1u << 31u)
#define UFSHC_CQIS_TEPS             (1u << 0u)

#define UFSHC_SPEC_VER              0x0400u

#define UFSHC_UIC_CMD_DME_GET          0x01u
#define UFSHC_UIC_CMD_DME_SET          0x02u
#define UFSHC_UIC_CMD_DME_PEER_GET     0x03u
#define UFSHC_UIC_CMD_DME_PEER_SET     0x04u
#define UFSHC_UIC_CMD_DME_LINK_STARTUP 0x16u
#define UFSHC_UIC_CMD_DME_HIBER_ENTER  0x17u
#define UFSHC_UIC_CMD_DME_HIBER_EXIT   0x18u
#define UFSHC_UIC_RESULT_SUCCESS       0x00u
#define UFSHC_UIC_RESULT_FAILURE       0x01u
#define UFSHC_PWR_LOCAL                0x01u

#define UFSHC_PA_CONNECTEDTXDATALANES  0x1561u
#define UFSHC_PA_CONNECTEDRXDATALANES  0x1581u
#define UFSHC_PA_MAXRXPWMGEAR          0x1586u
#define UFSHC_PA_MAXRXHSGEAR           0x1587u
#define UFSHC_PA_PWRMODE               0x1571u

#define UFSHC_UTP_REQ_DESC_INT_CMD     0x01000000u
#define UFSHC_UTP_NO_DATA_TRANSFER     0x00000000u
#define UFSHC_UTP_HOST_TO_DEVICE       0x02000000u
#define UFSHC_UTP_DEVICE_TO_HOST       0x04000000u
#define UFSHC_OCS_SUCCESS              0x0u
#define UFSHC_OCS_INVALID_CMD_TABLE    0x1u
#define UFSHC_OCS_INVALID_PRDT         0x2u
#define UFSHC_OCS_INVALID_COMMAND      0xfu
#define UFSHC_OCS_MASK                 0xffu

#define UFSHC_UPIU_NOP_OUT             0x00u
#define UFSHC_UPIU_COMMAND             0x01u
#define UFSHC_UPIU_QUERY_REQ           0x16u
#define UFSHC_UPIU_NOP_IN              0x20u
#define UFSHC_UPIU_RESPONSE            0x21u
#define UFSHC_UPIU_QUERY_RSP           0x36u
#define UFSHC_UPIU_FLAG_WRITE          0x20u
#define UFSHC_UPIU_FLAG_READ           0x40u
#define UFSHC_UPIU_FLAG_UNDERFLOW      0x20u
#define UFSHC_UPIU_FLAG_OVERFLOW       0x40u
#define UFSHC_UPIU_QUERY_READ          0x01u
#define UFSHC_UPIU_QUERY_WRITE         0x81u
#define UFSHC_UPIU_CMDSET_SCSI         0x0u
#define UFSHC_UPIU_CMDSET_QUERY        0x2u
#define UFSHC_COMMAND_RESULT_SUCCESS   0x00u
#define UFSHC_COMMAND_RESULT_FAIL      0x01u

#define UFSHC_QUERY_OPCODE_NOP         0x00u
#define UFSHC_QUERY_OPCODE_READ_DESC   0x01u
#define UFSHC_QUERY_OPCODE_WRITE_DESC  0x02u
#define UFSHC_QUERY_OPCODE_READ_ATTR   0x03u
#define UFSHC_QUERY_OPCODE_WRITE_ATTR  0x04u
#define UFSHC_QUERY_OPCODE_READ_FLAG   0x05u
#define UFSHC_QUERY_OPCODE_SET_FLAG    0x06u
#define UFSHC_QUERY_OPCODE_CLEAR_FLAG  0x07u
#define UFSHC_QUERY_OPCODE_TOGGLE_FLAG 0x08u

#define UFSHC_QUERY_RESULT_SUCCESS     0x00u
#define UFSHC_QUERY_RESULT_NOT_READABLE 0xf6u
#define UFSHC_QUERY_RESULT_NOT_WRITEABLE 0xf7u
#define UFSHC_QUERY_RESULT_INVALID_LENGTH 0xf9u
#define UFSHC_QUERY_RESULT_INVALID_VALUE  0xfau
#define UFSHC_QUERY_RESULT_INVALID_SELECTOR 0xfbu
#define UFSHC_QUERY_RESULT_INVALID_INDEX 0xfcu
#define UFSHC_QUERY_RESULT_INVALID_IDN 0xfdu
#define UFSHC_QUERY_RESULT_INVALID_OPCODE 0xfeu
#define UFSHC_QUERY_RESULT_GENERAL_FAILURE 0xffu

#define UFSHC_DESC_IDN_DEVICE          0x00u
#define UFSHC_DESC_IDN_UNIT            0x02u
#define UFSHC_DESC_IDN_INTERCONNECT    0x04u
#define UFSHC_DESC_IDN_STRING          0x05u
#define UFSHC_DESC_IDN_GEOMETRY        0x07u
#define UFSHC_DESC_IDN_POWER           0x08u
#define UFSHC_DESC_IDN_HEALTH          0x09u

#define UFSHC_FLAG_IDN_FDEVICEINIT     0x01u
#define UFSHC_FLAG_IDN_PERM_WPE        0x02u
#define UFSHC_FLAG_IDN_PWR_ON_WPE      0x03u
#define UFSHC_FLAG_IDN_BKOPS_EN        0x04u
#define UFSHC_FLAG_IDN_LIFE_SPAN_EN    0x05u
#define UFSHC_FLAG_IDN_PHY_REMOVAL     0x08u
#define UFSHC_FLAG_IDN_BUSY_RTC        0x09u
#define UFSHC_FLAG_IDN_PERM_FW_DISABLE 0x0bu
#define UFSHC_FLAG_IDN_WB_EN           0x0eu
#define UFSHC_FLAG_IDN_WB_FLUSH_EN     0x0fu
#define UFSHC_FLAG_IDN_WB_FLUSH_HIBER  0x10u
#define UFSHC_FLAG_IDN_COUNT           0x13u

#define UFSHC_ATTR_IDN_BOOT_LU_EN      0x00u
#define UFSHC_ATTR_IDN_POWER_MODE      0x02u
#define UFSHC_ATTR_IDN_ACTIVE_ICC_LVL  0x03u
#define UFSHC_ATTR_IDN_OOO_DATA_EN     0x04u
#define UFSHC_ATTR_IDN_BKOPS_STATUS    0x05u
#define UFSHC_ATTR_IDN_PURGE_STATUS    0x06u
#define UFSHC_ATTR_IDN_MAX_DATA_IN     0x07u
#define UFSHC_ATTR_IDN_MAX_DATA_OUT    0x08u
#define UFSHC_ATTR_IDN_DYN_CAP_NEEDED  0x09u
#define UFSHC_ATTR_IDN_REF_CLK_FREQ    0x0au
#define UFSHC_ATTR_IDN_CONF_DESC_LOCK  0x0bu
#define UFSHC_ATTR_IDN_MAX_NUM_RTT     0x0cu
#define UFSHC_ATTR_IDN_EE_CONTROL      0x0du
#define UFSHC_ATTR_IDN_EE_STATUS       0x0eu
#define UFSHC_ATTR_IDN_SECONDS_PASSED  0x0fu
#define UFSHC_ATTR_IDN_CNTX_CONF       0x10u
#define UFSHC_ATTR_IDN_FFU_STATUS      0x14u
#define UFSHC_ATTR_IDN_PSA_STATE       0x15u
#define UFSHC_ATTR_IDN_PSA_DATA_SIZE   0x16u
#define UFSHC_ATTR_IDN_REF_CLK_WAIT    0x17u
#define UFSHC_ATTR_IDN_CASE_TEMP       0x18u
#define UFSHC_ATTR_IDN_HIGH_TEMP_BOUND 0x19u
#define UFSHC_ATTR_IDN_LOW_TEMP_BOUND  0x1au
#define UFSHC_ATTR_IDN_THROTTLING      0x1bu
#define UFSHC_ATTR_IDN_WB_FLUSH_STATUS 0x1cu
#define UFSHC_ATTR_IDN_AVAIL_WB_SIZE   0x1du
#define UFSHC_ATTR_IDN_WB_LIFE_EST     0x1eu
#define UFSHC_ATTR_IDN_CURR_WB_SIZE    0x1fu
#define UFSHC_ATTR_IDN_REFRESH_STATUS  0x2cu
#define UFSHC_ATTR_IDN_REFRESH_FREQ    0x2du
#define UFSHC_ATTR_IDN_REFRESH_UNIT    0x2eu
#define UFSHC_ATTR_IDN_TIMESTAMP       0x30u
#define UFSHC_ATTR_IDN_COUNT           0x31u

#define UFSHC_WLUN_REPORT_LUNS         0x81u
#define UFSHC_WLUN_UFS_DEVICE          0xd0u
#define UFSHC_WLUN_BOOT                0xb0u
#define UFSHC_WLUN_RPMB                0xc4u

#define UFSHC_SCSI_GOOD                0x00u
#define UFSHC_SCSI_CHECK_CONDITION     0x02u
#define UFSHC_SCSI_TEST_UNIT_READY     0x00u
#define UFSHC_SCSI_REQUEST_SENSE       0x03u
#define UFSHC_SCSI_INQUIRY             0x12u
#define UFSHC_SCSI_MODE_SENSE_6        0x1au
#define UFSHC_SCSI_START_STOP          0x1bu
#define UFSHC_SCSI_READ_CAPACITY_10    0x25u
#define UFSHC_SCSI_READ_10             0x28u
#define UFSHC_SCSI_WRITE_10            0x2au
#define UFSHC_SCSI_SYNCHRONIZE_CACHE   0x35u
#define UFSHC_SCSI_UNMAP               0x42u
#define UFSHC_SCSI_MODE_SENSE_10       0x5au
#define UFSHC_SCSI_REPORT_LUNS         0xa0u
#define UFSHC_SCSI_READ_16             0x88u
#define UFSHC_SCSI_WRITE_16            0x8au
#define UFSHC_SCSI_SERVICE_ACTION_IN_16 0x9eu
#define UFSHC_SCSI_READ_CAPACITY_16_SA 0x10u

#define UFSHC_CQ_ENTRY_SIZE            32u
#define UFSHC_SQ_ENTRY_SIZE            32u

#endif
