#include "__vueehdr.h"

# 23 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
#define control_interval (((__NT*)TheStation)->__vattr_control_interval)
#define monitor_interval (((__NT*)TheStation)->__vattr_monitor_interval)








#define Kp (((__NT*)TheStation)->__vattr_Kp)
#define Ki (((__NT*)TheStation)->__vattr_Ki)
#define Kd (((__NT*)TheStation)->__vattr_Kd)
#define Active (((__NT*)TheStation)->__vattr_Active)

#define setpoint (((__NT*)TheStation)->__vattr_setpoint)
#define output (((__NT*)TheStation)->__vattr_output)
#define setting (((__NT*)TheStation)->__vattr_setting)
#define error (((__NT*)TheStation)->__vattr_error)
#define previous_error (((__NT*)TheStation)->__vattr_previous_error)
#define integral (((__NT*)TheStation)->__vattr_integral)
#define derivative (((__NT*)TheStation)->__vattr_derivative)





void get_error (word st) {



 word val;


 read_sensor (st, 0, &val);


 do { if ((val) < (0)) (val) = (0); else if ((val) > (2047)) (val) = (2047); } while (0);

 output = (lint) val;


 error = setpoint - output;
}

void set_plant (word st) {



 word val = (word) setting;

 write_actuator (st, 0, &val);
}

void update_integral () {






 if (Ki == 0 || error == 0) {

  integral = 0;
  return;
 }
 if (previous_error > 0 && error < 0 ||
     previous_error < 0 && error > 0) {

  integral = error;
  return;
 }


 integral += error;
}

void update_derivative () {



 derivative = error - previous_error;
}

void calculate_new_input () {



 lint delta;



 delta = ((


  Kp * error +





  (Ki * integral * control_interval)/1024 +




  (Kd * derivative * 1024)/control_interval



  ) * (1023 - 512)) /



  ((lint)(2047 - 0) * 100);



 setting = 512 + delta;


 do { if ((setting) < (1)) (setting) = (1); else if ((setting) > (1023)) (setting) = (1023); } while (0);
}

Boolean start () {

 if (Active)
  return 0;

 previous_error = integral = output = 0;
 setting = 512;
 (TheNode->tally_in_pcs()?(create controller )->_pp_apid_():0);
 ((void)(((PicOSNode*)TheStation)->TB.signal (__cpint(&monitor_interval))));

 Active = 1;
 return 1;
}

Boolean stop () {

 if (Active) {
  __pi_killall ((&zz_controller_prcs));
  Active = 0;
  return 1;
 }

 return 0;
}

void control_cycle (word st) {



 get_error (st);

 update_integral ();
 update_derivative ();
 calculate_new_input ();


 previous_error = error;
}




# 188 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
controller::perform { _pp_enter_ (); 

# 188 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"


 transient LOOP: {

  set_plant (LOOP);
  delay (control_interval, GET_RESPONSE);
  release;

 } transient GET_RESPONSE: {

  control_cycle (GET_RESPONSE);
  sameas LOOP;
}}


# 202 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
monitor::perform { _pp_enter_ (); 

# 202 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"


 transient LOOP: {

  ser_outf (LOOP, "%lu : %ld %ld <- %ld "
     "[E: %ld, I: %ld, D: %ld]\r\n",
      ((lword)(((lword) ituToEtu (Time)) - ((PicOSNode*)TheStation)->SecondOffset)),
      setting,
      setpoint,
      output,
      error,
      integral,
      derivative);

  if (Active)

   delay (monitor_interval, LOOP);


  __pi_when (__cpint (&monitor_interval), LOOP);
}}




#define cmd (((__NT*)TheStation)->__vattr_root_cmd)
# 226 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
root::perform { _pp_enter_ (); 

# 226 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"


 

 transient INIT: {

  (TheNode->tally_in_pcs()?(create monitor )->_pp_apid_():0);

 } transient BANNER: {

  ser_out (BANNER,
   "Commands:\r\n"
   "  c v [set control interval]\r\n"
   "  m v [set logging frequency]\r\n"
   "  t v [set the setpoint]\r\n"
   "  a v [set the actuator]\r\n"
   "  p v [set Kp]\r\n"
   "  i v [set Ki]\r\n"
   "  d v [set Kd]\r\n"
   "  r [run] \r\n"
   "  s [stop]\r\n"
   "  v [view, show]\r\n"
   "  . [cycle step]\r\n"
  );

 } transient UART_INPUT: {

  lint a, b, c;

  ser_in (UART_INPUT, cmd, 64);

  switch (cmd [0]) {

      case 'c' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a < 16 || a > 32768)
    sameas ILLEGAL_PARAMETER;
   control_interval = (word) a;
   sameas UART_INPUT;

      case 'm' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a > 32768)
    sameas ILLEGAL_PARAMETER;

   monitor_interval = (word) a;
   ((void)(((PicOSNode*)TheStation)->TB.signal (__cpint(&monitor_interval))));
   sameas UART_INPUT;

      case 't' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a < 0 || a > 2047)
    sameas ILLEGAL_PARAMETER;

   setpoint = (word) a;
   sameas UART_INPUT;

      case 'a' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a < 1 || a > 1023)
    sameas ILLEGAL_PARAMETER;

   setting = a;
   sameas SET_PLANT;

      case 'p' :

   a = 0;
   scan (cmd + 1, "%ld", &a);
   Kp = a;
   sameas UART_INPUT;

      case 'i' :

   a = 0;
   scan (cmd + 1, "%ld", &a);
   Ki = a;
   sameas UART_INPUT;

      case 'd' :

   a = 0;
   scan (cmd + 1, "%ld", &a);
   Kd = a;
   sameas UART_INPUT;

      case 'r' :

   if (!start ())
    sameas RUNNING_ALREADY;
   sameas UART_INPUT;

      case 's' :

   if (!stop ())
    sameas STOPPED_ALREADY;
   sameas UART_INPUT;

      case 'v' :

   proceed SHOW_PARAMS;

      case '.' :


   if (Active)
    sameas RUNNING_ALREADY;
   proceed RUN_CYCLE;
  }

 } transient BAD_COMMAND: {

  ser_out (BAD_COMMAND, "bad command!\r\n");
  sameas BANNER;

 } transient ILLEGAL_PARAMETER: {

  ser_out (ILLEGAL_PARAMETER, "llegal parameter!\r\n");
  sameas BANNER;

 } transient RUNNING_ALREADY: {

  ser_out (RUNNING_ALREADY, "running already!\r\n");
  sameas UART_INPUT;

 } transient STOPPED_ALREADY: {

  ser_out (STOPPED_ALREADY, "stopped already!\r\n");
  sameas UART_INPUT;

 } transient SET_PLANT: {

  set_plant (SET_PLANT);
  ((void)(((PicOSNode*)TheStation)->TB.signal (__cpint(&monitor_interval))));
  proceed UART_INPUT;

 } transient SHOW_PARAMS: {

  ser_outf (SHOW_PARAMS,
         "c=%ld, m=%ld, t=%ld, a=%ld, p=%ld, i=%ld, d=%ld\r\n",
   control_interval,
   monitor_interval,
   setpoint,
   setting,
   Kp, Ki, Kd);

  proceed UART_INPUT;

 } transient RUN_CYCLE: {

  control_cycle (RUN_CYCLE);
  sameas SET_PLANT;

}}
#undef cmd
# 391 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"


void __NT::__praxis_starter () { (TheNode->tally_in_pcs()?(create root )->_pp_apid_():0); }
