local module = tup.file(tup.getrelativedir(tup.getcwd())):upper()

if module ~= 'MODULES' and tup.getconfig(module) == 'y' then
  local inputs = tup.glob '*.c'
  inputs.extra_inputs = '$(ROOT)/src/resources/*.h'
  tup.foreach_rule(inputs, '!compile')
end
