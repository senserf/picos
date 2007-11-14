#ifndef	__picos_nvram_h__
#define	__picos_nvram_h__

typedef struct {
	lword start, length;
	byte *ptr;
} nvram_chunk_t;

typedef	struct {
	TIME	UnHang;
	double	Bounds [EP_N_BOUNDS];
} nvram_timing_t;

#define	NVRAM_TYPE_NOOVER	0x01		// Writing over zero is void
#define	NVRAM_TYPE_ERPAGE	0x02		// Erase by pages (blocks)
#define	NVRAM_FLAG_WEHANG	0x80000000	// Write delayed
#define	NVRAM_FLAG_UNSNCD	0x40000000	// Unsynced

class NVRAM {
	
	lword tsize;		// Total size in bytes
	lword esize;		// Current formal size of chunks
	lword asize;		// Number of valid entries in chunks
	lword pmask;		// Page mask (needed for erase with argument)

	nvram_timing_t	*ftimes;

	FLAGS	TP;		// Type flags, e.g., writing 1 to 0 is void

	nvram_chunk_t	*chunks;

	void merge (byte*, const byte*, lword len);
	void grow ();

	public:

#if 0
	void dump ();
#endif
	lword size (Boolean*, lword*);
	word get (lword, byte*, lword);
	word put (word, lword, const byte*, lword);
	word erase (word, lword, lword);
	word sync (word);
	NVRAM (lword, lword, FLAGS, double*);
	~NVRAM ();
};

#endif
