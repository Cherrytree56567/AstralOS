file(REMOVE_RECURSE
  "CMakeFiles/build"
  "obj/BasicConsole.o"
  "obj/Bitmap.o"
  "obj/EFIMemoryMap.o"
  "obj/GDT.o"
  "obj/IDT.o"
  "obj/KernelServices.o"
  "obj/KernelUtils.o"
  "obj/PageFrameAllocator.o"
  "obj/PageMapIndexer.o"
  "obj/PageTableManager.o"
  "obj/Paging.o"
  "obj/cstr.o"
  "obj/interrupts.o"
  "obj/main.o"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/build.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
