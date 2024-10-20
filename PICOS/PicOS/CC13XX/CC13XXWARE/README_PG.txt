This is the TI version 2_04_03_17272 copied from TIRTOS

I have modified one little thing: 

   struct {
      uint32_t preScale:4;              //!<        Prescaler value
      uint32_t :4;
      uint32_t rateWord:21;             //!<        Rate word
// PG: inserted from the TIRTOS version, probably not needed
      uint32_t decimMode:3;
   } symbolRate;                   

in two place in driverlib/rf_prop_cmd.h. This item, apparently needed by
Smartrf Studio has been borrowed from TIRTOS (it is "reserved" int the book)
