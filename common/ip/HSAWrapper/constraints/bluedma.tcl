##
## set properties to help out clock domain crossing analysis
##

foreach pat {"reset_hold_reg[*]" "sGEnqPtr*_reg[*]" "dGDeqPtr*_reg[*]" "sSyncReg*_reg[*]" "dSyncReg*_reg[*]" "dEnqPtr*_reg[*]"} {
     set cells [get_cells -hier $pat]
     if {[llength $cells] > 0} {
         puts "ASYNC_REG $cells"
         set_property ASYNC_REG 1 $cells
     }
}
