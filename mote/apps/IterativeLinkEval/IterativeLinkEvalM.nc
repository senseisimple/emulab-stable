includes packet;

/**
 * This is based on RobTransparentBase (except the base is no longer 
 * transparent.
 */

module IterativeLinkEvalM {

    provides interface StdControl;
    uses {
	interface StdControl as UARTControl;
// 	interface StdControl as UARTListenControl;
	interface SendMsg as UARTSendCMD;
	interface SendMsg as UARTSendRR;
	interface ReceiveMsg as UARTReceive;
// 	interface ReceiveMsg as UARTListenReceive;
	
	interface StdControl as RadioControl;
	interface BareSendMsg as RadioSend;
	interface ReceiveMsg as RadioReceive;

	interface Timer as TimerBCAST;

#ifdef PLATFORM_MICAZ
	interface CC2420Control as Radio;
#else
	interface CC1000Control as Radio;
#endif

	interface Leds;
    }
}

implementation
{
    /* holds teh cmd recv from serial line */
    //pkt_cmd_t *cmd_ptr;
    TOS_Msg cmd_msg;

    TOS_Msg hb_msg;

    /* flag to see if we've got a cmd executing from serial */
    uint8_t uart_task = 0;
    
    /* holds any pending bcast packets... we know it's done when the cmd
     * msg_dump_count field is zero
     */
    pkt_bcast_t send_bcast;
    TOS_Msg send_bcast_msg;
    //TOS_MsgPtr send_bcast_msg_ptr;

    /* this can be set... then it can be stuck into the AM header */
    /* FIXME: make that work for future... */
    uint16_t my_id = 0;

    enum {
	QUEUE_SIZE = 5
    };
    
    enum {
	TXFLAG_BUSY = 0x1,
	TXFLAG_TOKEN = 0x2
    };
    
    /* radio recv buffer */
    TOS_Msg gRxBufPool[QUEUE_SIZE]; 
    TOS_MsgPtr gRxBufPoolTbl[QUEUE_SIZE];
    uint8_t gRxHeadIndex,gRxTailIndex;

    

    task void UARTSendHeartbeat() {
	result_t result;

	pkt_cmd_t *cmd_ptr;

	//memset(&hb_msg,0,sizeof(TOS_Msg));
	
	//cmd_msg.type = AM_PKT_CMD;
	//cmd_msg.addr = TOS_LOCAL_ADDRESS;
	cmd_ptr = (pkt_cmd_t *)(hb_msg.data);
	//cmd_ptr->mote_id = TOS_LOCAL_ADDRESS;
	cmd_ptr->cmd_no = 0x0;
	cmd_ptr->cmd_type = CMD_TYPE_HEARTBEAT;
	
	result = call UARTSendCMD.send(TOS_UART_ADDR,
				       sizeof(pkt_cmd_t),
				       &hb_msg);
	if (result == SUCCESS) {
	    call Leds.redToggle();
	}
	else {
	    call Leds.greenToggle();
	}
    }

    /* this really ought to be a component with its own control iface */
    task void BcastCmdTask() {
	/* send the contents of the send_bcast_msg to bcast */
	result_t result;
	pkt_bcast_t *send_data = (pkt_bcast_t *)send_bcast_msg.data;
	pkt_cmd_t *cmd_ptr = (pkt_cmd_t *)cmd_msg.data;

	if (cmd_ptr->cmd_flags & CMD_FLAGS_INCR_FIRST) {
	    ++(cmd_ptr->bcast.data[0]);
	}

	//*((uint16_t *)&(cmd_ptr->bcast.data[8])) = TOS_LOCAL_ADDRESS;
	//cmd_ptr->bcast.src_mote_id = TOS_LOCAL_ADDRESS;
	
	send_bcast_msg.length = sizeof(pkt_bcast_t);
	*send_data = cmd_ptr->bcast;
	//send_bcast_msg.type = AM_RADIO_RECV;

	--(cmd_ptr->msg_dup_count);

	result = call RadioSend.send(&send_bcast_msg);
	/*^ radioDone will check to see if it ought to post more of these, or
	 * if it can return to the serial commander 
	 */
	/* but if result == FAIL, we don't get the event! */
	if (result == FAIL) {
	    /* send bad status to serial here */
	    cmd_ptr->cmd_status = CMD_STATUS_ERROR;

	    /* send the response to the uart */
	    call UARTSendCMD.send(TOS_UART_ADDR,
				  sizeof(pkt_cmd_t),
				  &cmd_msg);

	    atomic {
		uart_task = 0;
	    }
	}
	else {
	    //post UARTSendHeartbeat();

	    /* send a status msg from SendDone. */
	}

	return;
    }

    task void RadioRcvdTask() {
	TOS_MsgPtr pMsg;
	result_t   Result;
// 	//TOS_Msg temp_buf;
	pkt_bcast_t *bcast_ptr;
	pkt_radio_recv_t uart_dump;
	
	//dbg (DBG_USR1, "TOSBase forwarding Radio packet to UART\n");
	atomic {
	    pMsg = gRxBufPoolTbl[gRxTailIndex];
	    gRxTailIndex++; gRxTailIndex %= QUEUE_SIZE;
	}
	bcast_ptr = (pkt_bcast_t *)(pMsg->data);
	//temp_buf = *pMsg;
	//temp_buf.type = AM_PKT_RADIO_RECV;
	//temp_buf.length = sizeof(pkt_radio_recv_t);
	//uart_dump = (pkt_radio_recv_t *)(temp_buf.data);
	/* copy the bcast stuff */
	uart_dump.bcast = *bcast_ptr;
	/* drop in the rssi */
	uart_dump.rssi = pMsg->strength;
	//pMsg->length = 3;
	*((pkt_radio_recv_t *)pMsg->data) = uart_dump;
	//pMsg->length = sizeof(pkt_radio_recv_t);
	//pMsg->type = AM_PKT_RADIO_RECV;

	Result = call UARTSendRR.send(TOS_UART_ADDR,
				      sizeof(pkt_radio_recv_t),
				      //pMsg->length,
				      pMsg);

	if (Result != SUCCESS) {
	    pMsg->length = 0;
	}
	else {
	    call Leds.redToggle();
	}
    }

    task void UARTRcvdTask() {
	result_t result;
	pkt_cmd_t *cmd_ptr;
	pkt_bcast_t *send_bcast_ptr;

	cmd_ptr = (pkt_cmd_t *)(cmd_msg.data);

	switch (cmd_ptr->cmd_type) {
	case CMD_TYPE_BCAST:
	    /* set it up so that we send single pkt */
	    /* set up the xmit buf */
	    send_bcast_ptr = (pkt_bcast_t *)send_bcast_msg.data;
	    /* copy from cmd_ptr->bcast to send_bcast_ptr: */
	    *send_bcast_ptr = cmd_ptr->bcast;
	    /* set cmd_msg.msg_dup_count to one to ensure we send one pkt */
	    cmd_ptr->msg_dup_count = 1;

	    post BcastCmdTask();

	    break;
	case CMD_TYPE_BCAST_MULT:
	    /* send mults by decr the dup_count */
	    /* set up the xmit buf */
	    send_bcast_ptr = (pkt_bcast_t *)send_bcast_msg.data;
	    /* copy from cmd_ptr->bcast to send_bcast_ptr: */
	    *send_bcast_ptr = cmd_ptr->bcast;
	    if (cmd_ptr->msg_dup_count < 1) {
		cmd_ptr->msg_dup_count = 1;
	    }
	    else if (cmd_ptr->msg_dup_count > 1000) {
		cmd_ptr->msg_dup_count = 1000;
	    }

	    post BcastCmdTask();

	    break;
	case CMD_TYPE_RADIO_OFF:
	    /* simply turn off radio... this could be bad, but i don't 
	     * think so... things should come back up ok
	     */
	    result = call RadioControl.stop();
	    if (result == SUCCESS) {
		cmd_ptr->cmd_status = CMD_STATUS_SUCCESS;
	    }
	    else {
		cmd_ptr->cmd_status = CMD_STATUS_ERROR;
	    }
	    /* send the response to the uart */
	    call UARTSendCMD.send(TOS_UART_ADDR,
				  sizeof(pkt_cmd_t),
				  &cmd_msg);

	    atomic {
		uart_task = 0;
	    }

	    break;
	case CMD_TYPE_RADIO_ON:
	    /* if it's off, kick it back on... reinit the state too */
	    result = call RadioControl.start();
	    if (result == SUCCESS) {
		cmd_ptr->cmd_status = CMD_STATUS_SUCCESS;
	    }
	    else {
		cmd_ptr->cmd_status = CMD_STATUS_ERROR;
	    }
	    /* send the response to the uart */
	    call UARTSendCMD.send(TOS_UART_ADDR,
				  sizeof(pkt_cmd_t),
				  &cmd_msg);

	    atomic {
		uart_task = 0;
	    }

	    break;
	case CMD_TYPE_SET_MOTE_ID:
	    /* set mote id */
	    my_id = cmd_ptr->mote_id;
	    
	    cmd_ptr->cmd_status = CMD_STATUS_SUCCESS;

	    /* send the response to the uart */
	    call UARTSendCMD.send(TOS_UART_ADDR,
				  sizeof(pkt_cmd_t),
				  &cmd_msg);

	    atomic {
		uart_task = 0;
	    }

	    break;
	case CMD_TYPE_SET_RF_POWER:
	    /* set radio power */
	    result = call Radio.SetRFPower(cmd_ptr->power);

	    cmd_ptr->cmd_status = 
		(result == SUCCESS)?CMD_STATUS_SUCCESS:CMD_STATUS_ERROR;

	    /* send the response to the uart */
	    call UARTSendCMD.send(TOS_UART_ADDR,
				  sizeof(pkt_cmd_t),
				  &cmd_msg);

	    atomic {
		uart_task = 0;
	    }

	    break;
	default:
	    /* blink an led in error? */
	    
	    break;
	}

    }
    
    command result_t StdControl.init() {
	result_t ok1, ok2, ok3;
	uint8_t i;
	
	for (i = 0; i < QUEUE_SIZE; i++) {
	    gRxBufPool[i].length = 0;
	    gRxBufPoolTbl[i] = &gRxBufPool[i];
	}
	gRxHeadIndex = 0;
	gRxTailIndex = 0;
	
	
	ok1 = call UARTControl.init();
	//ok1 &= call UARTListenControl.init();
	ok2 = call RadioControl.init();
	ok3 = call Leds.init();
	
	return rcombine3(ok1, ok2, ok3);
    }
    
    command result_t StdControl.start() {
	result_t ok1, ok2;
	
	ok1 = call UARTControl.start();
	//ok1 &= call UARTListenControl.start();
	ok2 = call RadioControl.start();
	
	return rcombine(ok1, ok2);
    }
    
    command result_t StdControl.stop() {
	result_t ok1, ok2;
	
	ok1 = call UARTControl.stop();
	//ok1 &= call UARTListenControl.stop();
	ok2 = call RadioControl.stop();
	
	return rcombine(ok1, ok2);
    }
    
    event TOS_MsgPtr RadioReceive.receive(TOS_MsgPtr Msg) {
	TOS_MsgPtr pBuf;

	//post UARTSendHeartbeat();

	if (Msg->crc) {
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
		//post UARTSendHeartbeat();
		post RadioRcvdTask();
	    }
	    else {
		pBuf = Msg;
	    }
	}
	else {
	    pBuf = Msg;
	}
	
	return pBuf;
    }

//     event TOS_MsgPtr UARTListenReceive.receive(TOS_MsgPtr Msg) {
// 	recv_type = Msg->type;
// 	post UARTSend();

// 	return Msg;

// 	TOS_MsgPtr pBuf = Msg;
// 	uint8_t r = 0;

// 	atomic {
// 	    if (uart_task) {
// 		r = 1;
// 	    }
// 	    else {
// 		r = 0;
// 		uart_task = 1;
// 	    }
// 	}

// 	/* we just drop the cmd on the floor, i'm afraid... */
// 	if (r) {
// 	    //return pBuf;
// 	    post UARTSendHeartbeat();
// 	}
// 	else {
// 	    cmd_msg = *Msg;
// 	    post UARTRcvdTask();
// 	}

// 	return pBuf;

//     }
    
    event TOS_MsgPtr UARTReceive.receive(TOS_MsgPtr Msg) {
	TOS_MsgPtr pBuf = Msg;
	uint8_t r = 0;

	

	atomic {
	    if (uart_task) {
		r = 1;
	    }
	    else {
		r = 0;
		uart_task = 1;
	    }
	}

	/* we just drop the cmd on the floor, i'm afraid... */
	if (r) {
	    //return pBuf;
	}
	else {
	    cmd_msg = *Msg;
	    post UARTRcvdTask();
	}

	return pBuf;
	
    }
    
    event result_t UARTSendCMD.sendDone(TOS_MsgPtr Msg, result_t success) {
	//Msg->length = 0;
	return SUCCESS;
    }
    
    event result_t UARTSendRR.sendDone(TOS_MsgPtr Msg, result_t success) {
	Msg->length = 0;
	return SUCCESS;
    }

    event result_t TimerBCAST.fired() {
	// just in case...
	call TimerBCAST.stop();
	post BcastCmdTask();
    }
    
    event result_t RadioSend.sendDone(TOS_MsgPtr Msg, result_t success) {
	//pkt_bcast_t *send_bcast_ptr;
	pkt_cmd_t *cmd_ptr;

	cmd_ptr = (pkt_cmd_t *)cmd_msg.data;
	//send_bcast_ptr = (pkt_bcast_t *)send_bcast_msg.data;

	/* post more bcast tasks if we need to; otherwise, return cmd status
	 * to serial line
	 */
// 	if (uart_task) {
// 	    return 0;
// 	}
	

// 	if (success != SUCCESS) {
// 	    /* send err msg to serial */
// 	    cmd_ptr->cmd_status = CMD_STATUS_ERROR;
// 	    /* send the response to the uart */
// 	    call UARTSendCMD.send(TOS_UART_ADDR,
// 				  sizeof(pkt_cmd_t),
// 				  &cmd_msg);

// 	    atomic {
// 		uart_task = 0;
// 	    }

// 	    return success;
// 	}
	
	if (cmd_ptr->msg_dup_count > 0) {
	    if (cmd_ptr->interval > 0) {
		call TimerBCAST.start(TIMER_ONE_SHOT,cmd_ptr->interval);
	    }
	    else {
		post BcastCmdTask();
	    }
	}
	else {
	    /* send notif that the cmd is done */
	    if (success == SUCCESS) {
		cmd_ptr->cmd_status = CMD_STATUS_SUCCESS;
	    }
	    else {
		cmd_ptr->cmd_status = CMD_STATUS_ERROR;
	    }
	    /* send the response to the uart */
	    call UARTSendCMD.send(TOS_UART_ADDR,
				  sizeof(pkt_cmd_t),
				  &cmd_msg);

	    atomic {
		uart_task = 0;
	    }
	}
 	return SUCCESS;
    }
}  
