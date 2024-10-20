Directory BROKEN contains files for the broken version of the board
(conflicting connections of CMA3000 and EEPROM).

Directory FIXES contains the correct versions of the affected files for the
target board.

Make sure to copy the respective file to here according to the board version
you use.

Note that board_options.sys in BROKEN includes frequency adjustments for the
"bad" RF crystal.
