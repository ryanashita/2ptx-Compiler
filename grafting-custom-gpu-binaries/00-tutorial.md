---
layout: default
title: Tutorial 00
---
# 00 Grafting Custom GPU sections (.cubin files) into a fatbin file & Executing fatbins with the Nvidia Driver API
This is a tutorial on how to graft custom GPU sections into a fatbin file, and execute them directly with the Nvidia Driver API (not using Nvidia's Runtime API). The motivation behind this tutorial and its contents is part of my independent study in building a GPU compiler. To build the backend of the GPU compiler, specifically for an NVIDIA GPU and driver, I have to learn how to modify ```.cubin``` files and graft/modify them into a ```.fatbin``` container. This is to learn how to generate SASS -*Streaming ASSembler is the assembly format for programs running on NVIDIA GPUs*- manually (not from Nvidia tools), produce correct ELF -*Executable and Linkable Format (ELF) is the file format for object files in Linux*- files, and execute these instructions on the NVIDIA driver. Since I'm ultimately building a compiler to "replace" NVCC, I have to learn to get around NVCC and the Runtime API.

Getting into the more nitty-gritty of the tutorial, by custom GPU sections, I am talking about ```.cubin``` files -*target-specific ELF-formatted CUDA binary files*. These ```.cubin``` files are target-machine-specific, and in this tutorial they are specific to sm_75 (Nvidia GPU architecture with compute capability 7.5). 
Grafting in this context means taking a existing ```.fatbin``` and ```.cubin```, modifying the ```.cubin```, and rebuilding the fat binary with the modified ```.cubin``` file. *A .fatbin file is a "fat binary" container that holds one or more .cubin files*. The SASS assembly code inside the .cubin can be modified to perform a computation that differs from the computation of the kernel -*Kernels are special units of code (also called functions) that are designed to run in parallel in a GPU*- that was complied with NVCC, NVIDIA's compiler. 

For this tutorial, I will compile a CUDA file ```add.cu``` with the command ```$ nvcc --keep -arch=sm_75 -c add.cu```. The ```--keep``` option is to keep all 
intermediate files used in NVCC's full compilation process. The ```add.sm_75.cubin```, ```add.fatbin```, and ```add.o``` files will be particularly useful. 

## Understanding ```.cubin```, ```.fatbin```, and SASS
1. Extract ```nv_fatbin``` binary content from the ```add.o``` object file using this command:
```
$ objdump -s -j nv_fatbin add.o
```
2. View the hexadecimal representation of the ```add.fatbin``` contents with this command:
```
$ hexdump -C add.fatbin
```
Comparing the outputs from steps (1) and (2), it is clear that they are identical, meaning the fat binary portion in the final object file is unchanged from the ```.fatbin``` file created intermediately by the compiler. This is because the fat binary contains only information and data relevant to the GPU computation, nothing executed by the CPU.
This means everything we need to modify the GPU's computation / create custom GPU sections are contained in the fatbin. It's type is also PROGBITs (more on this later). 

3. Either open the existing ```add.sm_75.cubin``` file with:
```
$ nvdisasm add.sm_75.cubin
```
or run the command:
```
$ cuobjdump -xelf add1.sm_75.cubin add.fatbin // the cubin file's name can be anything, I added the '1' to differentiate between the two otherwise identical files
```
Looking at the contents of the disassembled ```.cubin``` file (run the ```nvdisasm``` command in this step), it can be seen that the ```FADD``` instruction (which we want to modify), is at a specific address. Running the command:
```
$ readelf -S add.sm_75.cubin
```
shows where the SASS instructions for the ```add``` kernel reside in the ```.cubin``` file. In my case, the section ```.text._Z3add[...]``` resides is at the specific offset of 0x700 bytes. Adding the two addresses gets us the specific address of the ```FADD``` instruction: 
```
address = (offset of .text section) + (address of FADD within the .text section)
```
4. Find an instruction you want to replace the ```FADD``` instruction with. For this tutorial I want to replace it with a ```FMUL``` instruction. Since our ```add``` kernel doesn't multiply anything, I made a very simple kernel in file ```mult.cu``` that multiplies values ```(*a) * (*b)``` instead of adding them. I compiled this into a ```.cubin``` file using the command:
```
$ nvcc -arch=sm_75 -cubin mult.cu
```
The ```FMUL``` instruction exists inside the ```mult.cubin``` file at a specific offset. Follow commands in step 3 to find the instruction's address. Use the offset address in this next command:
```
$ xxd -g 8 -l 16 -s 0x7e0 mult.cubin
```
The hexademical value ```0x7e0``` is the address I calculated of the ```FMUL``` instruction I found. Save the hexadecimal value that is outputted. For example, the bytes of the ```FMUL``` instruction I found and will use is ```20 72 00 00 03 00 00 00 00 00 40 00 00 d0 4f 00```. We will print this to the ```.cubin``` file and overwrite the ```FMUL``` instruction in the next section.

## Modifying the ```.cubin``` file by overwriting ```FADD``` instruction with a ```NOP``` instruction
5. Run the command:
```
$ printf 'HEXADECIMAL INSTRUCTION SEQUENCE HERE' | dd of=add.sm_75.cubin bs=1 seek=$((0x760)) conv=notrunc
```
where ```0x760``` is the address of the ```FADD``` instruction I want to overwrite with a ```FMUL```. Make sure that the instruction sequence
is in the format ```'\xHH\xHH\xHH'```, where ```'HH'``` is a byte. This is because we are using the hexidecimal representation of the sequence of bytes that make up the instruction we want to use. ```printf '\xHH...'``` prints raw bytes. ```dd``` -*a low-level unix command to manipulate raw data*- receives the byte stream. The ```bs=1``` option specifies that ```seek``` is measured in bytes and ```dd``` writes byte-by-byte. ```dd``` overwrites the bytes at the offset specified by the ```seek``` option in the file specified by the ```of``` option. Finally, ```conv=notrunc``` keeps the rest of the file intact, which is necessary because we are doing an in-place overwrite.

6. If you want to check that the instruction was indeed overwritten, run command:
```
$ nvdisasm --print-line-info add.sm_75.cubin
```
If you get errors running this command, it means that the bytes used to overwrite ```FADD``` were not representing a valid instruction. If there are no errors, move on. At this point the ```add.sm_75.cubin``` file could be renamed to ```mult.sm_75.cubin``` to decrease confusion. 

7. Rebuild the ```.fatbin``` file with the ```.cubin``` we just modified using the command:
```
$ nvcc --fatbin mult.sm_75.cubin -arch=sm_75 -o patched.fatbin // renamed the fatbin to decrease confusion.
```

## Executing the modified .fatbin file using the Nvidia Driver API and not using NVCC and Nvidia Runtime API
8. Now that we have a modified ```.fatbin``` file, we can execute the kernel with arguments we give it in a C++ script. We will not use NVCC and the Nvidia Runtime API to execute this! We will build the C++ script in a file, I'll call it ```main.cpp```:
```
$ vim main.cpp
```
All the driver API functions I used are found in the documentation: https://docs.nvidia.com/cuda/cuda-driver-api/modules.html#modules. 
Although the documentation is quite scant, reading the name and simple definitions of functions is enough to understand what to do (read: RTFM). 

Specifically, the functions I use are ```cuInit```, ```cuDeviceGet```,```cuCtxCreate```, ```cuModuleLoadFatBinary```, ```cuModuleGetFunction```, ```cuFuncLoad```, ```cuFuncIsLoaded```, ```cuMemAlloc```, ```cuMemcpyHtoD```, ```cuLaunchKernel```, and ```cuMemFree```. Each function returns a ```CUresult```. This is useful to test if a function did what the developer wanted it to do. Use a simple conditional to check: 
```
if (res != 0) {
  std::cerr << "failed in some way: " << res << std::endl;
  return 1;
}
```

Something that this tutorial does different from other resources on the internet is the use of ```cuModuleLoadFatBinary``` and the explicit use of a modified fatbin as the module. We can load the module inside the fat binary object into the current context. The code to do this is in the script. 

Take a look at the script file, ```main.cpp```, for more details and execution order of functions.

9. Finally, we can reap the benefits of our hard work. Run this command to compile the script:
```
$ clang++ main.cpp -I /usr/local/cuda/include -L /usr/local/cuda/lib64 -lcuda -o main
```
This command compiles the script using ```clang++```, -*a high-performance compiler driver for C++, based on LLVM*-. The ```-I /usr/local/cuda/include``` option adds a directory to the search path for include files, telling the compiler that when it sees ```#include <cuda.h>```, look in ```/usr/local/cuda/include```. Similarly, the ```-L /usr/local/cuda/lib64``` option adds a directory to the library search path at link time. That is where ```libcuda``` typically resides. Lastly, ```-lcuda``` links against the CUDA driver library.

Then, execute the executable: 
```
$ ./main
```

The outputs with ```std::cout``` may be different depending on your specific output strings, but the kernel that originally computed addition should now be computing multiplication! For me, the original kernel added 2.5 + 4.5 = 7.0. After the modification, the kernel computes 2.5 * 4.5 = 11.25. 

## Conclusion

This tutorial demonstrated how to modify a CUDA binary, package it into a fat binary, and execute the patched kernel using the CUDA Driver API. In our script, we've effectively replicated what ```libcudart``` -*The CUDA Runtime API*- does under the hood.

The next tutorial in this series walks through building a parser and AST, which can be read here: 
[tutorial 01](../2ptx-compiler/01-tutorial.html)

