PIcos Software Development Assistant
====================================


config.prj:

P(CO,xx), where xx is:

MB - multiple boards (0 or 1)
BO - board or board list, depending on MB; in the latter case, the list is
     { { suf1 BOARD } { suf2 BOARD } ... }



TODO:

+ fsm tags
+ keep multiple tags in case of replication, use some file-name semblance in
  case of conflict? No! Better cycle through the alternatives on subsequent
  clicks of the same tag, giving priority to *.cc over *.h.
+ status field (running)
+ save in config.prj board selection
+ edit file by clicking in term area

- more friendly labels in build menu
- show editor line numbers
