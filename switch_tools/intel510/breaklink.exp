#!/usr/local/bin/expect -f
spawn telnet [lindex $argv 0]
send "\n"
send "TestBox\n"
send "cup"
set port [lindex $argv 1]
set port [format "%d" [lindex $argv 1]]
set str "s"
#set down to the escape sequence for down
set down \033\[B
while { $port > 1 } {
    set str [concat $down $str]
    set port [expr $port - 1]
}
send $str
send " \nn\ny"
send "qqqql"
interact
