/*
** Mission override of the to_lab subscription table.
**
** Subscribes TO_LAB to the F Prime packetized telemetry stream so the
** cFS GroundSystem receives it directly over UDP. The fprime_app telemetry
** framing produces space packets with the secondary header flag set, so the
** message ID is the secondary header flag (0x0800) plus the packetized
** telemetry APID (ComCfg.Apid.FW_PACKET_PACKETIZED_TLM = 0x020, chosen to
** stay clear of the default cFE core housekeeping telemetry topics).
*/

#include "cfe_tbl_filedef.h" /* Required to obtain the CFE_TBL_FILEDEF macro definition */
#include "cfe_sb_api_typedefs.h"
#include "to_lab_tbl.h"
#include "cfe_msgids.h"

#include "to_lab_msgids.h"

/* F Prime packetized telemetry: secondary header flag + APID 0x020 (see fprime_app) */
#define FPRIME_PACKETIZED_TLM_MID 0x0820

TO_LAB_Subs_t Subscriptions = { .Subs = {
                                    { CFE_SB_MSGID_WRAP_VALUE(FPRIME_PACKETIZED_TLM_MID), { 0, 0 }, 4 },

                                    /* CFE_SB_MSGID_RESERVED entry to mark the end of valid MsgIds */
                                    { CFE_SB_MSGID_RESERVED, { 0, 0 }, 0 } } };

CFE_TBL_FILEDEF(Subscriptions, TO_LAB.Subscriptions, TO Lab Sub Tbl, to_lab_sub.tbl)
