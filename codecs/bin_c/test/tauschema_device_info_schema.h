
/* produced with command:
 $ schemacheck.py --C=no-name codecs/bin_php/test/device_info.schema --out-path=codecs/bin_c/test/
*/

#ifndef _TAUSCHEMA_DEVICE_INFO_H_
#define _TAUSCHEMA_DEVICE_INFO_H_

#include "tauschema_codec.h"

   extern const uint8_t tauschema_device_info_flatrows[];
   extern const tsch_size_t tauschema_device_info_flatsize;

   extern const tsch_size_t tauschema_device_info_maxtag;

 #define TAUSCH_NAM_DEVICE_INFO_	(0)
 #define TAUSCH_NAM_DEVICE_INFO_data	(1)
 #define TAUSCH_NAM_DEVICE_INFO_desc	(2)
 #define TAUSCH_NAM_DEVICE_INFO_idx	(3)
 #define TAUSCH_NAM_DEVICE_INFO_info	(4)
 #define TAUSCH_NAM_DEVICE_INFO_item	(5)
 #define TAUSCH_NAM_DEVICE_INFO_msglen	(6)
 #define TAUSCH_NAM_DEVICE_INFO_name	(7)
 #define TAUSCH_NAM_DEVICE_INFO_next	(8)
 #define TAUSCH_NAM_DEVICE_INFO_orig	(9)
 #define TAUSCH_NAM_DEVICE_INFO_schbin	(10)
 #define TAUSCH_NAM_DEVICE_INFO_schrow	(11)
 #define TAUSCH_NAM_DEVICE_INFO_schtxt	(12)
 #define TAUSCH_NAM_DEVICE_INFO_schurl	(13)
 #define TAUSCH_NAM_DEVICE_INFO_serial	(14)
 #define TAUSCH_NAM_DEVICE_INFO_sub	(15)
 #define TAUSCH_NAM_DEVICE_INFO_type	(16)
 #define TAUSCH_NAM_DEVICE_INFO_vendor	(17)
 #define TAUSCH_NAM_DEVICE_INFO_version	(18)

#endif // _DEVICE_INFO_H_
