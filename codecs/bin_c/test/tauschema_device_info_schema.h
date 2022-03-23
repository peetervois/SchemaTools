
/* produced with command:
 $ schemacheck.py --C=no-name codecs/bin_c/test/device_info.schema --out-path=codecs/bin_c/test/
*/

#ifndef _TAUSCHEMA_DEVICE_INFO_H_
#define _TAUSCHEMA_DEVICE_INFO_H_

#include "tauschema_codec.h"

   extern const uint8_t tauschema_device_info_flatrows[];
   extern const tsch_size_t tauschema_device_info_flatsize;

   extern const tsch_size_t tauschema_device_info_maxtag;

 #define TAUSCH_NAM_DEVICE_INFO_	(0)
 #define TAUSCH_NAM_DEVICE_INFO_data	(1)
 #define TAUSCH_NAM_DEVICE_INFO_demostring	(2)
 #define TAUSCH_NAM_DEVICE_INFO_desc	(3)
 #define TAUSCH_NAM_DEVICE_INFO_idx	(4)
 #define TAUSCH_NAM_DEVICE_INFO_info	(5)
 #define TAUSCH_NAM_DEVICE_INFO_item	(6)
 #define TAUSCH_NAM_DEVICE_INFO_msglen	(7)
 #define TAUSCH_NAM_DEVICE_INFO_name	(8)
 #define TAUSCH_NAM_DEVICE_INFO_next	(9)
 #define TAUSCH_NAM_DEVICE_INFO_orig	(10)
 #define TAUSCH_NAM_DEVICE_INFO_schbin	(11)
 #define TAUSCH_NAM_DEVICE_INFO_schrow	(12)
 #define TAUSCH_NAM_DEVICE_INFO_schtxt	(13)
 #define TAUSCH_NAM_DEVICE_INFO_schurl	(14)
 #define TAUSCH_NAM_DEVICE_INFO_serial	(15)
 #define TAUSCH_NAM_DEVICE_INFO_sub	(16)
 #define TAUSCH_NAM_DEVICE_INFO_type	(17)
 #define TAUSCH_NAM_DEVICE_INFO_vendor	(18)
 #define TAUSCH_NAM_DEVICE_INFO_version	(19)

#endif // _DEVICE_INFO_H_
