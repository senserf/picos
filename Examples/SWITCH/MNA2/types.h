#include "utraffin.h"
#include "mnaswtch.h"

#define col(n)  ((n) % NCols)   // Macros to convert Id to column/row
#define row(n)  ((n) / NCols)
#define odd(n)  ((n) & 1)       // Macros to tell whether a number is odd/event
#define evn(n)  (!odd (n))
