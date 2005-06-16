/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file garciaUtil.cc
 *
 * Implementation file for various utility functions related to the garcia.
 */

#include "config.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "garciaUtil.hh"

//#include "acpGarcia.h"
//#include "acpValue.h"
/* necessary to reset brainstem mods */
#include "aStem.h"

int debug = 0;

int find_range_start(unsigned long *ranges, unsigned long index)
{
    int lpc, retval = -1;

    assert(ranges != NULL);

    for (lpc = 0; (retval == -1) && (ranges[lpc + 1] != 0); lpc++) {
	if ((ranges[lpc] <= index) && (index < ranges[lpc + 1])) {
	    retval = lpc;
	}
    }

    return retval;
}

bool wait_for_brainstem_link(aIOLib ioRef, acpGarcia &garcia)
{
    int lpc;

    if (debug) {
	fprintf(stderr, "debug: establishing connection to brainstem\n");
    }
    
    aIO_MSSleep(ioRef, 500, NULL);
    for (lpc = 0;
	 (lpc < 30) && !garcia.getNamedValue("active")->getBoolVal();
	 lpc++) {
	printf(".");
	fflush(stdout);
	aIO_MSSleep(ioRef, 100, NULL);
    }
    if (lpc > 0)
	printf("\n");

    return garcia.getNamedValue("active")->getBoolVal();
}

/**
 * reset code for the brainstem GP/Moto modules.  Will work IF the router 
 * hasn't gotten slammed.  This could be called whenever the garcia won't
 * accept commands, or if it gets into a bad state that we can identify...
 */
int brainstem_reset(aIOLib &ioRef,unsigned char *modules, int modules_len) {
    aStemLib stem;
    aErr err;
    aPacketRef pkt;
    aPacketRef recvPkt;
    //aIOLib ioRef;
    aStreamRef stream;
    char code[2];
    int retval = 0;
    /** 
     * theoretically packets can't be longer than 8 bytes... but we're careful
     * cause the aStem api is woefully underspecified!
     */
    char recvData[16];
    unsigned char recvLength;
    unsigned char recvAddr;
    int i;
    
    /**
     * doing this cause the prototype from acroname ain't clear as to whether
     * the length param to aPacket_Create is actually used wrt the data ptr;
     * they don't say if the data ptr has to be a null term byte seq.
     */
    code[0] = CMD_RESET_CODE;
    code[1] = '\0';
    
    if (aStem_GetLibRef(&stem,&err) == 0) {
	fprintf(stderr,"BReset: connected to brainstem.\n");

	/* grab the io conn */
	//aIO_GetLibRef(&ioRef, &err);

	/* associate a stream with the stem connection */
	if (aStream_CreateSerial(ioRef,"tts/1",38400,&stream,&err) != 0) {
	    fprintf(stderr,"BReset: get stream failed (%d)!\n",err);
	    aStem_ReleaseLibRef(stem,&err);
	    return -1;
	}

	/** 
	 * the other possibility would be a kStemRelayStream (if you use
	 * the aStem_CreateRelayStream... which apparently is the wrong choice
	 * here...) 
	 */
	if (aStem_SetStream(stem,stream,kStemModuleStream,&err) != 0) {
	    fprintf(stderr,
		    "BReset: could not associate stream with stem (%d)!\n",
		    err);
	    aStem_ReleaseLibRef(stem,&err);
	    return -1;
	}


	/* reset the modules in order given... */
	for (i = 0; i < modules_len; ++i) {
	    if (aPacket_Create(stem,modules[i],1,code,&pkt,&err) != 0) {
		fprintf(stderr,
			"BReset: pkt create failed for module %d (%d)\n",
			modules[i],
			err);
		retval = modules[i];
		break;
	    }
		
	    if (aStem_SendPacket(stem,pkt,&err) != 0) {
		fprintf(stderr,
			"BReset: send failed for module %d.\n",
			modules[i]);
		retval = modules[i];
		break;
	    }

	    if (aStem_GetPacket(stem,NULL,NULL,1000,&recvPkt,&err) != 0) {
		fprintf(stderr,"BReset: did not get packet!\n");
		retval = modules[i];
		break;
	    }
	    
	    if (aPacket_GetData(stem,
				recvPkt,
				&recvAddr,
				&recvLength,
				recvData,
				&err) == 0) {
		
		
		if (recvLength == 2
		    && 1
		    && recvData[0] == 128 
		    && recvData[1] == 4) {
		    fprintf(stderr,
			    "BReset: succeeded for module %d\n",
			    modules[i]);
		}
		else {
		    fprintf(stderr,
			    "BReset: packet: module address = %d, "
			    "length = %d; ",
			    recvAddr,
			    recvLength);
		    
		    for (i = 0; i < recvLength; ++i) {
			fprintf(stderr,"d[%d]=%d ",i,recvData[i]);
		    }
		    
		    fprintf(stderr,"\n");
		    ;
		}
	    }
	    else {
		fprintf(stderr,"could not get data!\n");
		retval = modules[i];
		break;
	    }
			
	    aPacket_Destroy(stem,recvPkt,&err);

	    if ((i+1) != modules_len) {
		fprintf(stderr,"BReset: inter-module sleep\n");
		sleep(1);
	    }

	}
	
	aStem_ReleaseLibRef(stem,&err);
	fprintf(stderr,"BReset: done.\n");
    }
    else {
	/* need to send somebody mail saying we couldn't reset the stem */
	fprintf(stderr,
		"BReset: could not connect to brainstem for reset!!!\n");
	retval = -1;
    }

    return retval;

}
