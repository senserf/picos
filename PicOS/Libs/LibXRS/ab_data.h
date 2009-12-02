// ============================================================================

#ifdef __dcx_def__

// The definition view

#ifndef __ab_data_defined__
#define	__ab_data_defined__

byte	ab_xrs_sln __sinit (0), ab_xrs_rln __sinit (0),
	ab_xrs_new __sinit (0), ab_xrs_mod __sinit (1),
	ab_xrs_cur __sinit (0), ab_xrs_exp __sinit (0);

char	*ab_xrs_cou __sinit (NULL), *ab_xrs_cin __sinit (NULL);

int	ab_xrs_han __sinit (0);
word	ab_xrs_max __sinit (0);
address	ab_xrs_pac;

#endif

#endif

// ============================================================================

#ifdef __dcx_dcl__

// The declaration view

#ifdef __SMURPH__

#define	ab_xrs_sln	_dap (ab_xrs_sln)
#define	ab_xrs_rln	_dap (ab_xrs_rln)
#define	ab_xrs_new	_dap (ab_xrs_new)
#define	ab_xrs_mod	_dap (ab_xrs_mod)
#define	ab_xrs_cur	_dap (ab_xrs_cur)
#define	ab_xrs_exp	_dap (ab_xrs_exp)
#define	ab_xrs_cou	_dap (ab_xrs_cou)
#define	ab_xrs_cin	_dap (ab_xrs_cin)
#define	ab_xrs_han	_dap (ab_xrs_han)
#define	ab_xrs_max	_dap (ab_xrs_max)
#define	ab_xrs_pac	_dap (ab_xrs_pac)

#else

extern byte	ab_xrs_sln, ab_xrs_rln,
		ab_xrs_new, ab_xrs_mod,
		ab_xrs_cur, ab_xrs_exp;

extern char	*ab_xrs_cou, *ab_xrs_cin;

extern	int	ab_xrs_han;
extern	word	ab_xrs_max;
extern	address	ab_xrs_pac;

#endif

#endif

// ============================================================================

#ifdef __dcx_ini__

// The initialization view

ab_xrs_sln = 0; ab_xrs_rln = 0;
ab_xrs_new = 0; ab_xrs_mod = 1;
ab_xrs_cur = 0; ab_xrs_exp = 0;

ab_xrs_cou = NULL; ab_xrs_cin = NULL;

ab_xrs_han = 0;
ab_xrs_max = 0;

#endif

// ============================================================================
