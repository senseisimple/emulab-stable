# This is a nop tb_compact.tcl file that should be used when running scripts
# under ns.

proc tb-set-ip {node ip} {}
proc tb-set-ip-interface {src dst ip} {}
proc tb-set-ip-link {src link ip} {}
proc tb-set-ip-lan {src lan ip} {}
proc tb-set-hardware {node type args} {}
proc tb-set-node-os {node os} {}
proc tb-create-os {label path partition} {}
proc tb-set-link-loss {src args} {}
proc tb-set-lan-loss {lan rate} {}

# The following commands are not clearly defined and probably will be
# changed or removed
proc tb-set-dnard-ip {shelf number ip} {}
proc tb-set-dnard-os {shelf number os} {}
proc tb-set-node-deltas {node deltas} {}
proc tb-set-dnard-deltas {shelf number deltas} {}
