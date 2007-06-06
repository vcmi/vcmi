/* zpipe.c: example of proper use of zlib's inflate() and deflate()
   Not copyrighted -- provided to the public domain
   Version 1.4  11 December 2005  Mark Adler */

/* Version history:
   1.0  30 Oct 2004  First version
   1.1   8 Nov 2004  Add void casting for unused return values
					 Use switch statement for inflate() return values
   1.2   9 Nov 2004  Add assertions to document zlib guarantees
   1.3   6 Apr 2005  Remove incorrect assertion in inf()
   1.4  11 Dec 2005  Add hack to avoid MSDOS end-of-line conversions
					 Avoid some compiler warnings for input and output buffers
 */


#include "stdafx.h"
/* compress or decompress from stdin to stdout */
//int main(int argc, char **argv)
//{
//	int ret;
//
//	/* avoid end-of-line conversions */
//	SET_BINARY_MODE(stdin);
//	SET_BINARY_MODE(stdout);
//
//	/* do compression if no arguments */
//	if (argc == 1) {
//		ret = def(stdin, stdout, Z_DEFAULT_COMPRESSION);
//		if (ret != Z_OK)
//			zerr(ret);
//		return ret;
//	}
//
//	/* do decompression if -d specified */
//	else if (argc == 2 && strcmp(argv[1], "-d") == 0) {
//		ret = inf(stdin, stdout);
//		if (ret != Z_OK)
//			zerr(ret);
//		return ret;
//	}
//
//	/* otherwise, report usage */
//	else {
//		fputs("zpipe usage: zpipe [-d] < source > dest\n", stderr);
//		return 1;
//	}
//}
