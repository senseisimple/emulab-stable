Class link 

#output format is 
# link src srcport dst dstport

link instproc print {file} {

puts $file [format "l[$self set id] [$self set src] [$self set srcport] \
                     [$self set dst] [$self set dstport] [$self set bw] [$self set bw] [$self set delay] [$self set delay]"]
}
