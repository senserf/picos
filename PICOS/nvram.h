#ifndef	__picos_nvram_h__
#define	__picos_nvram_h__

typedef struct {
	word start, length;
	byte *ptr;
} nvram_chunk_t;

class NVRAM {
	
	word tsize;		// Total size in bytes
	word esize;		// Current formal size of chunks
	word asize;		// Number of valid entries in chunks
	word pmask;		// Page mask (needed for erase with argument)

	nvram_chunk_t *chunks;

	void merge (byte*, const byte*, word len);
	void grow ();

	public:

#if 0
	void dump ();
#endif
	void get (word, byte*, word);
	void put (word, const byte*, word);
	void erase (void);
	void erase (word);
	NVRAM (word, word);
	~NVRAM ();
};

#endif
