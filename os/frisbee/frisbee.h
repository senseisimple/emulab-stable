
#define FRISBEE_OK                 0x01
#define FRISBEE_DONE               0x02

#define FRISBEE_GENERIC_ERROR     -0x01
#define FRISBEE_INVALID_ARGUMENT  -0x02
#define FRISBEE_NO_CHUNK          -0x03

/* frisbeeInit
 *
 * initializes frisbee communication to server UDP network
 *
 * imageName  - name of image file to download (IGNORED currently)
 * broadcastAddr - IP address (byte order?) to broadcast to/receive from. 
 *
 * Returns FRISBEE_OK on success, <0 on failure.
 *
 */

int frisbeeInit( const char *   imageName,
		 unsigned int broadcastAddr );

int frisbeeInit2( const char * imageName,
		  const char * broadcastAddr );

/* frisbeeLockReadyChunk
 *
 * Obtains identifier, as well as a pointer to the meaty bits
 * of a chunk which has been entirely received by Frisbee.
 *
 * replyChunkId - pointer to int which will receive a chunk identifier
 * replyData    - pointer to uchar* which will receive a pointer to the
 *                actual data for the chunk.
 *
 * Returns FRISBEE_OK if there is a ready chunk 
 *   in this case, *replyChunkId and *replyData are modified.
 * Returns FRISBEE_NO_CHUNK if there was no ready chunk 
 *   in this case, *replyChunkId and *replyData are left untouched.
 *
 */

int frisbeeLockReadyChunk( unsigned int * replyChunkId,
			   unsigned char ** replyData );

/* frisbeeUnlockReadyChunk
 *
 * Called after the disk-writer has finished with a chunk.
 * (Buffer containing chunk is then freed.)
 *
 * Returns FRISBEE_OK on success, 
 * FRISBEE_INVALID_ARGUMENT if chunkId was invalid.
 *
 */
int frisbeeUnlockReadyChunk( unsigned int chunkId );

/* frisbeeLoop
 *
 * Called by disk-writer to service the frisbee protocol.
 * This should be called very frequently, as to not delay the network.
 *
 * Returns:
 *
 * FRISBEE_OK    in the normal case 
 * FRISBEE_DONE  to indicate there are no more chunks left 
 * <0            on error
 */
int frisbeeLoop();

/* frisbeeFinish
 * Called by disk-writer to close Frisbee communication once frisbeeLoop 
 * has returned FRISBEE_DONE, and ReadyChunks have been written to disk.
 *
 * Returns FRISBEE_OK.
 */
int frisbeeFinish();
