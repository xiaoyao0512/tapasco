#
# Copyright (C) 2017 Jens Korinth, TU Darmstadt
#
# This file is part of Tapasco (TPC).
#
# Tapasco is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Tapasco is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Tapasco.  If not, see <http://www.gnu.org/licenses/>.
#
# @file		platform.tcl
# @brief	Platform skeleton implementations.
# @author	J. Korinth, TU Darmstadt (jk@esa.tu-darmstadt.de)
#
namespace eval platform {
  namespace export create
  namespace export generate
  namespace export get_address_map

  # Creates the platform infrastructure, consisting of a number of subsystems.
  # Subsystems "host", "clocks_and_resets", "memory", "intc" and "tapasco" are
  # mandatory, their wiring pre-defined. Custom subsystems can be instantiated
  # by implementing a "create_custom_subsystem_<NAME>" proc in platform::, where
  # <NAME> is a placeholder for the name of the subsystem.
  proc create {} {
    set instance [current_bd_instance]
    # create mandatory subsystems
    set ss_host    [create_subsystem "host"]
    set ss_cnrs    [create_subsystem "clocks_and_resets" false true]
    set ss_mem     [create_subsystem "memory"]
    set ss_intc    [create_subsystem "intc"]
    set ss_tapasco [create_subsystem "tapasco"]

    set sss [list $ss_cnrs $ss_host $ss_intc $ss_mem $ss_tapasco]

    foreach ss $sss {
      set name [string trim $ss "/"]
      set cmd  "create_subsystem_$name"
      puts "Creating subsystem $name ..."
      if {[llength [info commands $cmd]] == 0} {
        error "Platform does not implement mandatory command $cmd!"
      }
      current_bd_instance $ss
      eval $cmd
      current_bd_instance $instance
    }

    for {set i 1} {$i < [llength $sss]} {incr i} {
      connect_bd_intf_net [get_bd_intf_pins [lindex $sss [expr "$i - 1"]]/M_CLOCKS_RESETS] \
        [get_bd_intf_pins [lindex $sss $i]/S_CLOCKS_RESETS]
    }
    connect_bd_intf_net [get_bd_intf_pins [lindex $sss end]/M_CLOCKS_RESETS] \
      [get_bd_intf_pins -of_objects [get_bd_cells "/uArch"] -filter "VLNV == [tapasco::get_vlnv "tapasco_clocks_resets"] && MODE == Slave"]

    
    # create custom subsystems
    foreach ss [info commands create_custom_subsystem_*] {
      set name [regsub {.*create_custom_subsystem_(.*)} $ss {\1}]
      puts "Creating custom subsystem $name ..."
      current_bd_instance [create_subsystem $name]
      eval $ss
      current_bd_instance $instance
    }

    wire_subsystem_wires
    wire_subsystem_intfs
  }

  # Creates a hierarchical cell with given name and instantiates either a
  # ClocksResetsBridgeMaster or ClocksResetsBridgeSlave, depending on whether
  # is_reset is true or false, respectively. get_clock_reset_port can be used
  # to access the pins in this component.
  proc create_subsystem {name {has_slave true} {has_master true}} {
    set instance [current_bd_instance]
    set cell [create_bd_cell -type hier $name]
    set intf_vlnv [tapasco::get_vlnv "tapasco_clocks_resets"]
    current_bd_instance $cell

    if {$has_master} {
      set m_port [create_bd_intf_pin -vlnv $intf_vlnv -mode Master "M_CLOCKS_RESETS"]
      set m_cnrs [create_bd_cell -type ip -vlnv [tapasco::get_vlnv "clocks_resets_m"] "m_cnrs"]
      connect_bd_intf_net [get_bd_intf_pins -of_objects $m_cnrs -filter "VLNV == $intf_vlnv"] $m_port
    }

    if {$has_slave} {
      set s_port [create_bd_intf_pin -vlnv $intf_vlnv -mode Slave "S_CLOCKS_RESETS"]
      set s_cnrs [create_bd_cell -type ip -vlnv [tapasco::get_vlnv "clocks_resets_s"] "s_cnrs"]
      connect_bd_intf_net $s_port [get_bd_intf_pins -of_objects $s_cnrs -filter "VLNV == $intf_vlnv"]
    }

    if {$has_master && $has_slave} {
      # directly connect all wires
      foreach p [get_bd_pins -of_objects $m_cnrs -filter { NAME =~ i_* }] {
        set oname [regsub {i_} [get_property NAME $p] {o_}]
        set op [get_bd_pins -of_objects $s_cnrs -filter "NAME == $oname"]
        puts "  connecting $p to $op ..."
        connect_bd_net $p $op
      }
    }

    current_bd_instance $instance
    return $cell
  }

  # Returns pin with given name  on the clocks and resets bridge component in
  # the current subsystem.
  proc get_clock_reset_port {name} {
    set cells [get_bd_cells -filter "VLNV == [tapasco::get_vlnv clocks_resets_s] || VLNV == [tapasco::get_vlnv clocks_resets_m]"]
    return [get_bd_pins -of_objects $cells -filter "NAME == $name && INTF == false"]
  }

  proc get_pe_base_address {} {
    error "Platform does not implement mandatory proc get_pe_base_address!"
  }

  proc create_subsystem_tapasco {} {
    set port [create_bd_intf_pin -vlnv [tapasco::get_vlnv "aximm_intf"] -mode Slave "S_TAPASCO"]
    set tapasco_status [tapasco::createTapascoStatus "tapasco_status"]
    connect_bd_intf_net $port [get_bd_intf_pins -of_objects $tapasco_status -filter "VLNV == [tapasco::get_vlnv aximm_intf] && MODE == Slave"]
    connect_bd_net [get_clock_reset_port "o_design_clk"] [get_bd_pins -of_objects $tapasco_status -filter {TYPE == clk && DIR == I}]
    connect_bd_net [get_clock_reset_port "o_design_peripheral_reset"] [get_bd_pins -of_objects $tapasco_status -filter {TYPE == rst && DIR == I}]
  }

  proc wire_subsystem_wires {} {
    foreach p [get_bd_pins -of_objects [get_bd_cells] -filter {INTF == false && DIR == I}] {
      if {[llength [get_bd_nets -of_objects $p]] == 0} {
        set name [get_property NAME $p]
        set type [get_property TYPE $p]
        puts "Looking for matching source for $p ($name) with type $type ..."
        set src [get_bd_pins -of_objects [get_bd_cells] -filter "NAME == $name && TYPE == $type && INTF == false && DIR == O"]
        if {[llength $src] > 0} {
          puts "  found pin: $src, connecting $p -> $src"
          connect_bd_net $src $p
        } else {
          puts "  found no matching pin for $p"
        }
      }
    }
  }

  proc wire_subsystem_intfs {} {
    foreach p [get_bd_intf_pins -of_objects [get_bd_cells] -filter {MODE == Slave}] {
      if {[llength [get_bd_intf_nets -of_objects $p]] == 0} {
        set name [regsub {^S_} [get_property NAME $p] {M_}]
        set vlnv [get_property VLNV $p]
        puts "Looking for matching source for $p ($name) with VLNV $vlnv ..."
        set srcs [lsort [get_bd_intf_pins -of_objects [get_bd_cells] -filter "NAME == $name && VLNV == $vlnv && MODE == Master"]]
        foreach src $srcs {
          if {[llength [get_bd_intf_nets -of_objects $src]] == 0} {
            puts "  found pin: $src, connecting $p -> $src"
            connect_bd_intf_net $src $p
            break
          } else {
            puts "  found no matching pin for $p"
          }
        }
      }
    }
  }

  # Returns the address map of the current composition.
  # Format: <SLAVE INTF> <BASE ADDR> <RANGE> <KIND>
  # Kind is either Mem or Register, depending on the usage.
  # Must be implemented by Platforms.
  proc get_address_map {offset} {
    set ret [list]
    set pes [lsort [arch::get_processing_elements]]
    #set offset 0x00300000
    foreach pe $pes {
      set usrs [lsort [get_bd_addr_segs $pe/* -filter { USAGE != memory }]]
      for {set i 0} {$i < [llength $usrs]} {incr i; incr offset 0x10000} {
        set seg [lindex $usrs $i]
        set intf [get_bd_intf_pins -of_objects $seg]
        set range [get_property RANGE $seg]
        lappend ret "interface $intf [format "offset 0x%08x range 0x%08x" $offset $range] kind register"
      }
      set usrs [lsort [get_bd_addr_segs $pe/* -filter { USAGE == memory }]]
      for {set i 0} {$i < [llength $usrs]} {incr i; incr offset 0x10000} {
        set seg [lindex $usrs $i]
        set intf [get_bd_intf_pins -of_objects $seg]
        set range [get_property RANGE $seg]
        lappend ret "interface $intf [format "offset 0x%08x range 0x%08x" $offset $range] kind memory"
      }
    }
    return $ret
  }

  # Checks all current runs at given step for errors, outputs their log files in case.
  # @param synthesis Checks synthesis runs if true, implementation runs otherwise.
  proc check_run_errors {{synthesis true}} {
    if {$synthesis} {
      set failed_runs [get_runs -filter "IS_SYNTHESIS == 1 && PROGRESS != {100%}"]
    } else {
      set failed_runs [get_runs -filter "IS_IMPLEMENTATION == 1 && PROGRESS != {100%}"]
    }
    # check number of failed runs
    if {[llength $failed_runs] > 0} {
      puts "at least one run failed, check these logs for errors:"
      foreach r $failed_runs {
        puts "  [get_property DIRECTORY $r]/runme.log"
      }
      exit 1
    }
  }

  # Platform API: Main entry point to generate the bitstream.
  proc generate {} {
    global bitstreamname
    # generate bitstream from given design and report utilization / timing closure
    set jobs [tapasco::get_number_of_processors]
    puts "  using $jobs parallel jobs"

    generate_target all [get_files system.bd]
    set synth_run [get_runs synth_1]
    set_property -dict [list \
      strategy Flow_PerfOptimized_high \
      STEPS.SYNTH_DESIGN.ARGS.DIRECTIVE AlternateRoutability \
      STEPS.SYNTH_DESIGN.ARGS.RETIMING true \
    ] $synth_run
    current_run $synth_run
    launch_runs -jobs $jobs $synth_run
    wait_on_run $synth_run
    if {[get_property PROGRESS $synth_run] != {100%}} { check_run_errors true }
    open_run $synth_run

    # call plugins
    tapasco::call_plugins "post-synth"

    set impl_run [get_runs impl_1]
    set_property -dict [list \
      STEPS.ROUTE_DESIGN.ARGS.DIRECTIVE Explore \
      STEPS.PHYS_OPT_DESIGN.IS_ENABLED true \
    ] $impl_run
    current_run $impl_run
    launch_runs -jobs $jobs -to_step route_design $impl_run
    wait_on_run $impl_run
    if {[get_property PROGRESS $impl_run] != {100%}} { check_run_errors false }
    open_run $impl_run

    # call plugins
    tapasco::call_plugins "post-impl"

    if {[get_property PROGRESS [get_runs $impl_run]] != "100%"} {
      error "ERROR: impl failed!"
    }
    report_timing_summary -warn_on_violation -file timing.txt
    report_utilization -file utilization.txt
    report_utilization -file utilization_userlogic.txt -cells [get_cells -hierarchical -filter {NAME =~ *target_ip_*}]
    report_power -file power.txt
    set wns [tapasco::get_wns_from_timing_report "timing.txt"]
    if {$wns >= -0.3} {
      write_bitstream -force "${bitstreamname}.bit"
    } else {
      error "timing failure, WNS: $wns"
    }
  }
}
