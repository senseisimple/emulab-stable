

/* initialize chunker, passing the size of the file (in MB) and
   the number of buffers to allocate (1 MB apiece) */
void c_init( uint sizeM, uint bufferCount );

/* close down chunker */
void c_finish();

/* suggest which kilobyte to request to help chunker on its path to glory */
int  c_suggestK();

/* slip the chunker a kilobyte of data we've obtained */
void c_addKtoM( uint kb, uchar * data );

/* query for a complete megabyte; 
   mb will get the mb identifier, data will get a pointer to data.
   returns 1 if there was something available, 0 otherwise. */
int  c_consumeM( uint * mb, uchar ** data );

/* indicate that a complete megabyte has been processed and may be dumped */
void c_finishedM( uint mb );
