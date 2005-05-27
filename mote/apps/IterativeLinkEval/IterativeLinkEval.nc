includes packet;

configuration IterativeLinkEval {

}

implementation {
    components Main, 
	IterativeLinkEvalM, 
	RadioCRCPacket as Comm,
	GenericComm as UART,
// 	GenericComm as Comm,
	//UARTFramedPacket as UARTListen,
	LedsC,
#ifdef PLATFORM_MICAZ
	CC2420ControlM,
#else
	CC1000ControlM,
#endif
	TimerC;

    Main.StdControl                  -> IterativeLinkEvalM;

//     IterativeLinkEvalM.CommControl   -> Comm;

//     IterativeLinkEvalM.CommSendCMD   -> UART.SendMsg[AM_PKT_CMD];
//     IterativeLinkEvalM.CommSendRR    -> UART.SendMsg[AM_PKT_RADIO_RECV];
//     IterativeLinkEvalM.CommSendBCAST -> Comm.SendMsg[AM_PKT_BCAST];

//     IterativeLinkEvalM.CommRecvCMD   -> Comm.ReceiveMsg[AM_PKT_CMD];
//     IterativeLinkEvalM.RadioControl -> Comm;
//     IterativeLinkEvalM.RadioSend -> Comm;
//     IterativeLinkEvalM.RadioReceive -> Comm;

    IterativeLinkEvalM.UARTControl -> UART;
    IterativeLinkEvalM.UARTSendCMD -> UART.SendMsg[AM_PKT_CMD];
    IterativeLinkEvalM.UARTSendRR -> UART.SendMsg[AM_PKT_RADIO_RECV];
    IterativeLinkEvalM.UARTReceive -> UART.ReceiveMsg[AM_PKT_CMD];
    IterativeLinkEvalM.RadioControl -> Comm;
    IterativeLinkEvalM.RadioSend -> Comm;
    IterativeLinkEvalM.RadioReceive -> Comm;

//     IterativeLinkEvalM.UARTListenControl -> UARTListen;
//     IterativeLinkEvalM.UARTListenReceive -> UARTListen.Receive;

#ifdef PLATFORM_MICAZ
    IterativeLinkEvalM.Radio -> CC2420ControlM;
#else
    IterativeLinkEvalM.Radio -> CC1000ControlM;
#endif

    IterativeLinkEvalM.TimerBCAST -> TimerC.Timer[unique("Timer")];

    IterativeLinkEvalM.Leds -> LedsC;
}
