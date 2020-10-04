#include "__vueehdr.h"
void __NT::init () {
memset (__attr_init_origin, 0, __attr_init_end - __attr_init_origin);
# 15 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
__vattr_control_interval=16 ;__vattr_monitor_interval=1024 ;
# 25 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
__vattr_Kp=0 ;__vattr_Ki=0 ;__vattr_Kd=0 ;
# 28 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
__vattr_Active=0 ;
__praxis_starter ();
}
void __build_node (data_no_t *nddata) { create __NT (nddata); }
