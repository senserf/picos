#ifndef __tarp_hooks_h
#define __tarp_hooks_h

/*
 * These ones must be provided by the praxis (and must be methods in VUEE)
 */

__VIRTUAL int _da (tr_offset) (headerType*) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isBind) (msg_t) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isTrace) (msg_t) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isMaster) (msg_t) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isNew) (msg_t) __ABSTRACT;
__VIRTUAL word _da (guide_rtr) (headerType*) __ABSTRACT;
__VIRTUAL void _da (set_master_chg) (void) __ABSTRACT;

#endif
