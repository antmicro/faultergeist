proc env_var_equals { env_var value } {
  return [expr { [info exists ::env($env_var)] && $::env($env_var) == $value }]
}

proc env_var_exists_and_non_empty { env_var } {
  return [expr { [info exists ::env($env_var)] && ![string equal $::env($env_var) ""] }]
}

yosys -import

if { [env_var_exists_and_non_empty LIB_FILES] } {
read_liberty -overwrite -setattr liberty_cell -lib {*}$::env(LIB_FILES)
}

if { [env_var_equals SYNTH_HDL_FRONTEND slang] } {
  plugin -i slang
  yosys read_slang --top $::env(DESIGN_TOP) --std latest --single-unit --allow-toplevel-iface-ports -F $::env(DESIGN_FILE_LIST) --no-implicit-memories
} else {
  read_verilog_file_list -F $::env(DESIGN_FILE_LIST)
}
synth -noabc -top $::env(DESIGN_TOP)
# Synthesis done, write output
clean
yosys rename -wire

if { [env_var_exists_and_non_empty LIB_FILES] } {
  set lib_args ""
  foreach lib $::env(LIB_FILES) {
    append lib_args "-liberty $lib "
  }

  dfflibmap {*}$lib_args

  # Consider using custom ABC script
  abc {*}$lib_args

  # Make sure we mapped all the cells to the target library
  write_verilog $::env(VERILOG_NETLIST).pre_check.v
  check -assert -mapped
}

write_verilog $::env(VERILOG_NETLIST)
write_json $::env(JSON_NETLIST)
