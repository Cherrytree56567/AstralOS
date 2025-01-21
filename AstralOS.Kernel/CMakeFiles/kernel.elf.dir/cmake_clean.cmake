file(REMOVE_RECURSE
  "bin/kernel.elf"
  "bin/kernel.elf.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/kernel.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
