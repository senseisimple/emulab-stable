#dummy decls and stuff.

Class Agent/Null
Class Agent/TCP
Class Agent/TCPSink
Class Agent/UDP

Class Application/Traffic/CBR

Application/Traffic/CBR instproc attach-agent args {
$self set packet_size_ 3
$self set interval_ 3
return nop
}

Class Source/FTP
