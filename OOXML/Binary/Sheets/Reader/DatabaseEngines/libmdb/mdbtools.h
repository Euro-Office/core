/* Shim: force libmdb sources to use our vendored mdbtools.h, not the
 * system-installed mdbtools-dev one which may be an older version
 * missing fields like 'locale', 'iconv_in', 'iconv_out' in MdbHandle. */
#include "../mdbtools/mdbtools.h"
