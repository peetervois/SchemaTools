
/* produced with command:
 $ schemacheck.py --C=no-name codecs/bin_php/test/device_info.schema --out-path=codecs/bin_c/test/
*/

#include "tauschema_check.h"


const uint8_t tauschema_device_info_flatrows[] = {
 14	,100	,0	,0	,0	,5	,0	,1	,4	,17	,10	,0	,1	,7	,17	,15	// .d..............
,25	,2	,9	,5	,0	,20	,1	,1	,16	,0	,0	,8	,6	,5	,0	,30	// ................
,2	,18	,17	,15	,35	,3	,14	,17	,15	,40	,4	,17	,17	,15	,45	,5	// ....#....(....-.
,12	,17	,15	,50	,6	,13	,17	,15	,55	,7	,10	,18	,60	,0	,1	,11	// ...2....7...<...
,17	,65	,0	,1	,5	,4	,0	,70	,2	,7	,17	,15	,75	,3	,2	,17	// .A.....F....K...
,15	,80	,4	,16	,3	,0	,85	,5	,15	,4	,0	,90	,6	,8	,4	,0	// .P....U....Z....
,95	,7	,3	,4	,0	,0	,7										// _......

};
const tsch_size_t tauschema_device_info_flatsize = sizeof( tauschema_device_info_flatrows ); // 103
const tsch_size_t tauschema_device_info_maxtag = 32;

