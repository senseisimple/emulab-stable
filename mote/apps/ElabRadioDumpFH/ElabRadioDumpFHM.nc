// $Id: ElabRadioDumpFHM.nc,v 1.3 2005-06-27 22:11:57 johnsond Exp $

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
 * Revision:	$Id: ElabRadioDumpFHM.nc,v 1.3 2005-06-27 22:11:57 johnsond Exp $
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


module ElabRadioDumpFHM {
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
    interface Timer as TimerFH;

    interface CC1000Control;

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

    int freqTableLength = 236;
    uint32_t freqTable[236] = 
    {
 	902012557, 
 	902096400, 902167353, 902280720, 902403600, 902491249, 902608400, 
 	902682811, 902734492, 903017877, 903018000, 903301630, 903353066, 
 	903427600, 903544506, 903632400, 903755280, 903868769, 903939600, 
 	904023320, 904071134, 904246800, 904435907, 904492560, 904597763, 
 	904693575, 904861200, 905003046, 905066000, 905124391, 905229840, 
 	905363829, 905475600, 905570184, 905651020, 905782800, 905885200, 
	905967120, 906034084, 906090000, 906137323, 906177648, 906704277, 
 	906704461, 907230905, 907271600, 907318800, 907374593, 907441680, 
 	907523600, 907626000, 907757534, 907838738, 907933200, 908044847, 
 	908178960, 908284162, 908342800, 908405877, 908547600, 908715102, 
 	908810791, 908916240, 908973015, 909162000, 909337419, 909385356, 
 	909469200, 909540154, 909653520, 909776400, 909864048, 909981200, 
 	910055611, 910107292, 910390676, 910390800, 910674431, 910725865, 
 	910800400, 910917305, 911005200, 911128080, 911241569, 911312400, 
 	911396120, 911443933, 911619600, 911808708, 911865360, 911970562, 
 	912066374, 912234000, 912375846, 912438800, 912497190, 912602640, 
 	912736629, 912848400, 912942985, 913023819, 913155600, 913258000, 
 	913339920, 913406883, 913462800, 913510123, 913550447, 914077076, 
 	914077262, 914603704, 914644400, 914691600, 914747392, 914814480, 
 	914896400, 914998800, 915130333, 915211539, 915306000, 915417647, 
 	915551760, 915656961, 915715600, 915778677, 915920400, 916087901,
	
	916183590, 916289040, 916345816, 916534800, 916710218, 916758156, 
 	916842000, 916912954, 917026320, 917149200, 917236847, 917354000, 
 	917428410, 917480093, 917763475, 917763600, 918047231, 918098665, 
 	918173200, 918290104, 918378000, 918500880, 918614370, 918685200, 
 	918768919, 918816732, 918992400, 919181508, 919238160, 919343361, 
 	919439174, 919606800, 919748647, 919811600, 919869989, 919975440, 
 	920109428, 920221200, 920315785, 920396618, 920528400, 920630800, 
 	920712720, 920779683, 920835600, 920882924, 920923246, 921449875, 
 	921450000, 921450062, 921976503, 922017201, 922064400, 922120192, 
 	922187280, 922269200, 922371600, 922503132, 922584339, 922678800, 
 	922790446, 922924560, 923029760, 923088400, 923151478, 923293200, 
 	923460701, 923556389, 923661840, 923718616, 923907600, 924083017, 
 	924130955, 924214800, 924285755, 924399120, 924522000, 924609646, 
 	924726800, 924801210, 924852893, 925136274, 925136400, 925420032, 
 	925471464, 925546000, 925662903, 925750800, 925873680, 925987170, 
 	926058000, 926141719, 926189531, 926365200, 926554309, 926610960, 
 	926716160, 926811973, 926979600, 927121447, 927184400, 927242788, 
 	927348240, 927482228, 927594000, 927688586, 927769417, 927901200, 
 	928003600, 
    };
    int freqIdx = 0;


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
    
    dbg (DBG_USR1, "ElabRadioDumpFH forwarding Radio packet to UART\n");
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
    
    // this gets done in the event handler!

    //atomic {
	//pMsg->data[8] = freqTable[freqIdx];
	//pMsg->data[9] = freqTable[freqIdx] >> 8;
	//pMsg->data[10] = freqTable[freqIdx] >> 16;
	//pMsg->data[11] = freqTable[freqIdx] >> 24;
    //}

    pMsg->length = (pMsg->length > 12)?pMsg->length:12;

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

    dbg (DBG_USR1, "ElabRadioDumpFH forwarding UART packet to Radio\n");
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

    freqIdx = 0;

    dbg(DBG_BOOT, "ElabRadioDumpFH initialized\n");

    return rcombine3(ok1, ok2, ok3);
  }

  command result_t StdControl.start() {
    result_t ok1, ok2;
    
    ok1 = call UARTControl.start();
    ok2 = call RadioControl.start();

    //call CC1000Control.TuneManual(freqTable[4]);

    //call Leds.greenOn();
    //call Timer.start(TIMER_REPEAT,1024);

    freqIdx = 0;

#ifndef FH_INTERVAL
#define FH_INTERVAL 1024
#endif

    atomic {
        call Leds.greenOn();
        // freqIdx starts out at -1, so this works.
        freqIdx = (++freqIdx)%freqTableLength;
        call RadioControl.stop();
        if (call CC1000Control.TuneManual(freqTable[freqIdx]) == 916710218) {
            call Leds.redOn();
        }
        else {
            call Leds.redOff();
        }
        call RadioControl.start();
        call Leds.greenOff();
    }

    call TimerFH.start(TIMER_REPEAT,FH_INTERVAL);

    return rcombine(ok1, ok2);
  }

  command result_t StdControl.stop() {
    result_t ok1, ok2;
    
    ok1 = call UARTControl.stop();
    ok2 = call RadioControl.stop();

    call TimerFH.stop();

    return rcombine(ok1, ok2);
  }

  event TOS_MsgPtr RadioReceive.receive(TOS_MsgPtr Msg) {
    TOS_MsgPtr pBuf;

    dbg(DBG_USR1, "ElabRadioDumpFH received radio packet.\n");

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
	// do this here to minimize the chances of reporting the wrong freq
	// for a packet.
	atomic {
	    pBuf->data[8] = freqTable[freqIdx];
	    pBuf->data[9] = freqTable[freqIdx] >> 8;
	    pBuf->data[10] = freqTable[freqIdx] >> 16;
	    pBuf->data[11] = freqTable[freqIdx] >> 24;
	}
	post RadioRcvdTask();
    }
    else {
	pBuf = Msg;
    }

    return pBuf;
  }
  
  event TOS_MsgPtr UARTReceive.receive(TOS_MsgPtr Msg) {
    TOS_MsgPtr  pBuf;

    dbg(DBG_USR1, "ElabRadioDumpFH received UART packet.\n");

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
    
    dbg(DBG_USR1, "ElabRadioDumpFH received UART token packet.\n");

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
	//call Leds.greenToggle();
	return SUCCESS;
    }

    event result_t TimerFH.fired() {
	atomic {
	    call Leds.greenOn();
	    // freqIdx starts out at -1, so this works.
	    freqIdx++;
	    freqIdx = freqIdx%freqTableLength;
	    call RadioControl.stop();
	    if (call CC1000Control.TuneManual(freqTable[freqIdx]) == 916710218) {
		call Leds.redOn();
	    }
	    else {
		call Leds.redOff();
	    }
	    call RadioControl.start();
	    call Leds.greenOff();
	}
	
	return SUCCESS;
    }

}  
