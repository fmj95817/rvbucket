proc dc_fail {msg} {
    puts "RVBUCKET_ASIC_SYN_ERROR: ${msg}"
    exit 1
}

proc dc_must_bool {msg cmd} {
    if {[catch {uplevel 1 $cmd} result]} {
        dc_fail "${msg}: ${result}"
    }
    if {$result != 1} {
        dc_fail "${msg}: command returned ${result}"
    }
    return $result
}

proc dc_must {msg cmd} {
    if {[catch {uplevel 1 $cmd} result]} {
        dc_fail "${msg}: ${result}"
    }
    return $result
}

if {[info exists ::env(RVBUCKET_ROOT)]} {
    set root_dir [file normalize $::env(RVBUCKET_ROOT)]
} else {
    set root_dir [file normalize [file join [pwd] ../../../..]]
}

if {[info exists ::env(RVBUCKET_ASIC_CONFIG_TCL)]} {
    set asic_config_tcl [file normalize $::env(RVBUCKET_ASIC_CONFIG_TCL)]
} else {
    set asic_config_tcl [file normalize "config.tcl"]
}
if {![file exists ${asic_config_tcl}]} {
    dc_fail "ASIC config Tcl not found: ${asic_config_tcl}"
}
source ${asic_config_tcl}

foreach required_var {
    asic_conf_name
    top_module
    clock_name
    clock_port
    clock_period
    clock_uncertainty
    clock_latency
    pdk_dir
    pdk_search_path
    synthetic_library_list
    target_library_list
    link_library_list
    symbol_library_list
    rtl_defines
    rtl_list_file
    input_delay_max_ratio
    input_delay_min_ratio
    output_delay_max_ratio
    output_delay_min_ratio
    max_transition
    max_fanout
    output_load
    critical_range_ratio
    high_fanout_net_threshold
    driving_cell
    driving_cell_pin
    driving_cell_input_transition_rise
    driving_cell_input_transition_fall
    use_operating_conditions
    operating_conditions_analysis_type
    operating_conditions_max
    operating_conditions_max_lib
    operating_conditions_min
    operating_conditions_min_lib
} {
    if {![info exists ${required_var}]} {
        dc_fail "ASIC config missing variable ${required_var}"
    }
}

set rpt_dir "../reports/${asic_conf_name}"
set data_dir "../data/${asic_conf_name}"
set work_dir "./work/${asic_conf_name}"

sh mkdir -p ${rpt_dir} ${data_dir} ${work_dir}
sh rm -rf ${rpt_dir}/* ${data_dir}/* ${work_dir}/*

set_app_var search_path [concat $search_path [list \
    "${root_dir}/base/rtl" \
    "${root_dir}/design/rtl" \
    "${root_dir}/design/rtl/core" \
    "${root_dir}/asic/stub" \
    ] ${pdk_search_path}]

set_app_var synthetic_library ${synthetic_library_list}
set_app_var target_library ${target_library_list}
set_app_var link_library ${link_library_list}
set_app_var symbol_library ${symbol_library_list}

set_app_var hdlin_keep_signal_name user
set_app_var hdlin_check_no_latch true
set_app_var verilogout_no_tri true
set_app_var verilogout_equation false
set_app_var bus_naming_style "%s_%d"
set_app_var bus_dimension_separator_style "_"
set_app_var bus_range_separator_style ":"
set_app_var compile_seqmap_propagate_constants false
set_app_var high_fanout_net_threshold ${high_fanout_net_threshold}

define_design_lib work -path ${work_dir}

if {${rtl_list_file} eq ""} {
    dc_fail "ASIC config missing rtl_list_file"
}
if {[file pathtype ${rtl_list_file}] ne "absolute"} {
    set rtl_list_file [file normalize "${root_dir}/${rtl_list_file}"]
} else {
    set rtl_list_file [file normalize ${rtl_list_file}]
}
if {![file exists ${rtl_list_file}]} {
    dc_fail "RTL file list not found: ${rtl_list_file}"
}
set rtl_files [list]
set fp [open ${rtl_list_file} r]
while {[gets $fp line] >= 0} {
    set line [string trim $line]
    if {$line eq ""} {
        continue
    }
    if {[string match "#*" $line]} {
        continue
    }
    if {[file pathtype ${line}] eq "absolute"} {
        lappend rtl_files [file normalize ${line}]
    } else {
        lappend rtl_files [file normalize "${root_dir}/${line}"]
    }
}
close $fp

set_svf "${data_dir}/${top_module}.svf"

if {[llength ${rtl_defines}] > 0} {
    dc_must_bool "analyze RTL failed" {
        analyze -format sverilog -define ${rtl_defines} $rtl_files
    }
} else {
    dc_must_bool "analyze RTL failed" {
        analyze -format sverilog $rtl_files
    }
}
dc_must "elaborate failed" {elaborate ${top_module}}
if {[sizeof_collection [get_designs -quiet ${top_module}]] == 0} {
    dc_fail "top design ${top_module} was not elaborated"
}

dc_must "set current design failed" {current_design ${top_module}}
dc_must_bool "link failed" {link}
dc_must_bool "uniquify failed" {uniquify}
dc_must_bool "post-uniquify link failed" {link}

redirect -file "${rpt_dir}/${top_module}.check_design.pre.rpt" {
    check_design
}

create_clock -name ${clock_name} -period ${clock_period} [get_ports ${clock_port}]
set_clock_uncertainty ${clock_uncertainty} [get_clocks ${clock_name}]
set_clock_latency ${clock_latency} [get_clocks ${clock_name}]
set_ideal_network -no_propagate [get_ports [list ${clock_port} rst_n]]
set_dont_touch_network [get_ports [list ${clock_port} rst_n]]

set inputs [remove_from_collection [all_inputs] [get_ports ${clock_port}]]
set inputs [remove_from_collection $inputs [get_ports rst_n]]
set outputs [all_outputs]

set_input_delay  [expr ${input_delay_max_ratio} * ${clock_period}] -max -clock ${clock_name} $inputs
set_output_delay [expr ${output_delay_max_ratio} * ${clock_period}] -max -clock ${clock_name} $outputs
set_input_delay  [expr ${input_delay_min_ratio} * ${clock_period}] -min -clock ${clock_name} $inputs
set_output_delay [expr ${output_delay_min_ratio} * ${clock_period}] -min -clock ${clock_name} $outputs

set_max_transition ${max_transition} [current_design]
set_max_fanout ${max_fanout} [current_design]
set_load ${output_load} $outputs

if {${driving_cell} ne "" && [sizeof_collection [get_lib_cells -quiet */${driving_cell}]] > 0} {
    set_driving_cell -lib_cell ${driving_cell} \
        -pin ${driving_cell_pin} \
        -input_transition_rise ${driving_cell_input_transition_rise} \
        -input_transition_fall ${driving_cell_input_transition_fall} \
        $inputs
}

if {${use_operating_conditions}} {
    catch {
        set_operating_conditions \
            -max ${operating_conditions_max} \
            -max_lib ${operating_conditions_max_lib} \
            -min ${operating_conditions_min} \
            -min_lib ${operating_conditions_min_lib} \
            -analysis_type ${operating_conditions_analysis_type}
    }
}

set_fix_multiple_port_nets -all -buffer_constants
set_fix_hold [get_clocks ${clock_name}]
set_critical_range [expr ${critical_range_ratio} * ${clock_period}] [current_design]

# This is a pre-synthesis sanity flow. Keep runtime predictable and preserve
# enough hierarchy for RTL debug; use a separate PPA flow for final high-effort
# compile_ultra exploration.
dc_must_bool "compile failed" {compile -map_effort medium -area_effort none}

redirect -file "${rpt_dir}/${top_module}.check_design.rpt" {
    check_design
}
redirect -file "${rpt_dir}/${top_module}.hierarchy.rpt" {
    report_hierarchy
}
redirect -file "${rpt_dir}/${top_module}.qor.rpt" {
    report_qor
}
redirect -file "${rpt_dir}/${top_module}.area.rpt" {
    report_area -hierarchy
}
redirect -file "${rpt_dir}/${top_module}.power.rpt" {
    report_power -hierarchy
}
redirect -file "${rpt_dir}/${top_module}.constraints.rpt" {
    report_constraint -all_violators
}
redirect -file "${rpt_dir}/${top_module}.timing.setup.rpt" {
    report_timing -delay_type max -max_paths 50 -nets -transition -input_pins
}
redirect -file "${rpt_dir}/${top_module}.timing.hold.rpt" {
    report_timing -delay_type min -max_paths 50 -nets -transition -input_pins
}
redirect -file "${rpt_dir}/${top_module}.resources.rpt" {
    report_resources -hierarchy
}
redirect -file "${rpt_dir}/${top_module}.references.rpt" {
    report_reference -hierarchy
}

change_names -rules verilog -hierarchy
remove_ideal_network [get_ports [list ${clock_port} rst_n]]
remove_attribute [get_ports [list ${clock_port} rst_n]] dont_touch_network

dc_must_bool "write DDC failed" {
    write -format ddc -hierarchy -output "${data_dir}/${top_module}.ddc"
}
dc_must_bool "write gate netlist failed" {
    write -format verilog -hierarchy -output "${data_dir}/${top_module}.vg"
}
dc_must_bool "write SDC failed" {
    write_sdc -nosplit "${data_dir}/${top_module}.sdc"
}
dc_must_bool "write SDF failed" {
    write_sdf -version 2.1 "${data_dir}/${top_module}.sdf"
}

set_svf off
exit 0
