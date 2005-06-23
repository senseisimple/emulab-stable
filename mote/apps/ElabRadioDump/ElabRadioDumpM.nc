// $Id: ElabRadioDumpM.nc,v 1.2 2005-06-23 16:46:32 johnsond Exp $

/*									tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE     
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA, 
 * 94704.  Attention:  Intel License Inquiry.
 */
/* 
 * Author:	Phil Buonadonna
 * Revision:	$Id: ElabRadioDumpM.nc,v 1.2 2005-06-23 16:46:32 johnsond Exp $
 *
 *
 */
  
/* TOSBaseM
   - captures all the packets that it can hear and report it back to the UART
   - forward all incoming UART messages out to the radio
*/

/**
 * @author Phil Buonadonna
 */


module ElabRadioDumpM {
  provides interface StdControl;
  uses {
    interface StdControl as UARTControl;
    //interface BareSendMsg as UARTSend;
    interface SendMsg as UARTSend;
    interface ReceiveMsg as UARTReceive;
    //interface TokenReceiveMsg as UARTTokenReceive;

    interface StdControl as RadioControl;
    interface BareSendMsg as RadioSend;
    interface ReceiveMsg as RadioReceive;

    interface Timer;

    interface Leds;
  }
}
implementation
{
  enum {
    QUEUE_SIZE = 5
  };

  enum {
    TXFLAG_BUSY = 0x1,
    TXFLAG_TOKEN = 0x2
  };


  TOS_Msg gRxBufPool[QUEUE_SIZE]; 
  TOS_MsgPtr gRxBufPoolTbl[QUEUE_SIZE];
  uint8_t gRxHeadIndex,gRxTailIndex;

  TOS_Msg    gTxBuf;
  TOS_MsgPtr gpTxMsg;
  uint8_t    gTxPendingToken;
  uint8_t    gfTxFlags;

//     uint8_t timer_going = 0;

  task void RadioRcvdTask() {
    TOS_MsgPtr pMsg;
    result_t   Result;
//     int st = 0;

    /* blink yellow leds */
//     atomic {
// 	if (!timer_going) {
// 	    timer_going = 1;
// 	    st = 1;
// 	}
//     }

//     if (st) {
// 	call Timer.start(TIMER_ONE_SHOT,128);
// 	call Leds.yellowOn();
//     }

    // we just use a toggle, in hopes that this will better illustrate speed:
    call Leds.yellowToggle();
    
    dbg (DBG_USR1, "ElabRadioDump forwarding Radio packet to UART\n");
    atomic {
      pMsg = gRxBufPoolTbl[gRxTailIndex];
      gRxTailIndex++; gRxTailIndex %= QUEUE_SIZE;
    }
    //*(pMsg->data) = 1;
    pMsg->data[1] = pMsg->strength;
    pMsg->data[2] = pMsg->strength >> 8;
    pMsg->data[3] = pMsg->type;
    pMsg->data[4] = pMsg->addr;
    pMsg->data[5] = pMsg->addr >> 8;
    pMsg->data[6] = pMsg->crc;
    pMsg->data[7] = pMsg->crc >> 8;

    pMsg->length = (pMsg->length > 8)?pMsg->length:8;

    Result = call UARTSend.send(TOS_UART_ADDR,pMsg->length,pMsg);
    if (Result != SUCCESS) {
      pMsg->length = 0;
    }
    else {
	//call Leds.redToggle();
    }
  }

  task void UARTRcvdTask() {
    result_t Result;

    dbg (DBG_USR1, "ElabRadioDump forwarding UART packet to Radio\n");
    Result = call RadioSend.send(gpTxMsg);

    if (Result != SUCCESS) {
      atomic gfTxFlags = 0;
    }
    else {
	//call Leds.greenToggle();
    }
  }

#if 0
  task void SendAckTask() {
     call UARTTokenReceive.ReflectToken(gTxPendingToken);
     call Leds.yellowToggle();
     atomic {
       gpTxMsg->length = 0;
       gfTxFlags = 0;
     }
  } 
#endif

  command result_t StdControl.init() {
    result_t ok1, ok2, ok3;
    uint8_t i;

    for (i = 0; i < QUEUE_SIZE; i++) {
      gRxBufPool[i].length = 0;
      gRxBufPoolTbl[i] = &gRxBufPool[i];
    }
    gRxHeadIndex = 0;
    gRxTailIndex = 0;

    gTxBuf.length = 0;
    gpTxMsg = &gTxBuf;
    gfTxFlags = 0;

    ok1 = call UARTControl.init();
    ok2 = call RadioControl.init();
    ok3 = call Leds.init();

    dbg(DBG_BOOT, "ElabRadioDump initialized\n");

    return rcombine3(ok1, ok2, ok3);
  }

  command result_t StdControl.start() {
    result_t ok1, ok2;
    
    ok1 = call UARTControl.start();
    ok2 = call RadioControl.start();


    //call Leds.greenOn();
    call Timer.start(TIMER_REPEAT,1024);

    return rcombine(ok1, ok2);
  }

  command result_t StdControl.stop() {
    result_t ok1, ok2;
    
    ok1 = call UARTControl.stop();
    ok2 = call RadioControl.stop();

    return rcombine(ok1, ok2);
  }

  event TOS_MsgPtr RadioReceive.receive(TOS_MsgPtr Msg) {
    TOS_MsgPtr pBuf;

    dbg(DBG_USR1, "ElabRadioDump received radio packet.\n");

    if (Msg->crc) {
      /* turn off cause passed the crc */
	call Leds.redOff();
    }
    else {
	call Leds.redOn();
    }

    atomic {
	pBuf = gRxBufPoolTbl[gRxHeadIndex];
	if (pBuf->length == 0) {
	    gRxBufPoolTbl[gRxHeadIndex] = Msg;
	    gRxHeadIndex++; gRxHeadIndex %= QUEUE_SIZE;
	}
	else {
	    pBuf = NULL;
	}
    }
      
    if (pBuf) {
	post RadioRcvdTask();
    }
    else {
	pBuf = Msg;
    }

    return pBuf;
  }
  
  event TOS_MsgPtr UARTReceive.receive(TOS_MsgPtr Msg) {
    TOS_MsgPtr  pBuf;

    dbg(DBG_USR1, "ElabRadioDump received UART packet.\n");

    atomic {
      if (gfTxFlags & TXFLAG_BUSY) {
        pBuf = NULL;
      }
      else {
        pBuf = gpTxMsg;
        gfTxFlags |= (TXFLAG_BUSY);
        gpTxMsg = Msg;
      }
    }

    if (pBuf == NULL) {
      pBuf = Msg; 
    }
    else {
      post UARTRcvdTask();
    }

    return pBuf;

  }

#if 0
  event TOS_MsgPtr UARTTokenReceive.receive(TOS_MsgPtr Msg, uint8_t Token) {
    TOS_MsgPtr  pBuf;
    
    dbg(DBG_USR1, "ElabRadioDump received UART token packet.\n");

    atomic {
      if (gfTxFlags & TXFLAG_BUSY) {
        pBuf = NULL;
      }
      else {
        pBuf = gpTxMsg;
        gfTxFlags |= (TXFLAG_BUSY | TXFLAG_TOKEN);
        gpTxMsg = Msg;
        gTxPendingToken = Token;
      }
    }

    if (pBuf == NULL) {
      pBuf = Msg; 
    }
    else {

      post UARTRcvdTask();
    }

    return pBuf;
  }
#endif
  
  event result_t UARTSend.sendDone(TOS_MsgPtr Msg, result_t success) {
    Msg->length = 0;
    return SUCCESS;
  }
  
  event result_t RadioSend.sendDone(TOS_MsgPtr Msg, result_t success) {


    if ((gfTxFlags & TXFLAG_TOKEN)) {
      if (success == SUCCESS) {
        
        //post SendAckTask();
      }
    }
    else {
      atomic {
        gpTxMsg->length = 0;
        gfTxFlags = 0;
      }
    }
    return SUCCESS;
  }

    event result_t Timer.fired() {
	call Leds.greenToggle();
	return SUCCESS;
    }

}  
