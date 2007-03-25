/*
 * This file is included by all storage_.....c files
 */

lword ee_size (Boolean *er, lword *eru) {

	if (er)
		*er = (Boolean) EE_ERASE_BEFORE_WRITE;

	if (eru)
		*eru = (lword) EE_ERASE_UNIT;

	return EE_SIZE;
}
