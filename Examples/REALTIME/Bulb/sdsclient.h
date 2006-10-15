/* Client request types */

#define  CRT_ACT     0     /* Action */
#define  CRT_ATL     1     /* Add attribute to monitor list */
#define  CRT_RAT     2     /* Remove attribute from monitor list */
#define  CRT_RDA     3     /* Read attribute */
#define  CRT_WRA     4     /* Write attribute */
#define  CRT_ADE     5     /* Get attribute descriptor */
#define  CRT_RIN     6     /* Read input */
#define  CRT_ROU     7     /* Read output */
#define  CRT_WOU     8     /* Write output */
#define  CRT_AIS     9     /* Monitor input status */
#define  CRT_AOS    10     /* Monitor output status */
#define  CRT_RIS    11     /* Unmonitor input status */
#define  CRT_ROS    12     /* Unmonitor output status */

/* This is a special status value to indicate the beginning of an attribute */
/* change message                                                           */
#define  SDS_ATTRIBUTE_CHANGE  109
#define  SDS_ISTAT_CHANGE      110
#define  SDS_OSTAT_CHANGE      111


#define  ATTR_ISTAT (-1)   /* Special attribute number - input status */
#define  ATTR_OSTAT (-2)   /* Special attribute number - output status */
