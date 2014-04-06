Here is a praxis for issuing UART commands remotely over RF, primarily intended for testing things like BlueTooth connected to the only UART of the board.

The RF interface is simple. To make it reliable, every command is repeated N times (say 3 or 4) and tagged with a serial number. Duplicates are ignored. Same with replies.
