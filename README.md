# 2ptx-Compiler

This compiler compiles my custom domain-specific language (DSL) to PTX, Nvidia's virtual ISA for their GPUs. This project was what I worked for my Independent Study (CSCI 498 Ind Reading: Computer Science) spring semester 2026, which focused on learning compiler design and development in C++. My independent study was titled "Developing a domain-specific language and compiler for numerically intense kernels". Here is an explanation of my studies, from the start of the spring 2026 semester: 

"This semester, I will develop my own domain-specific programming language (DSL) specifically designed for programmers to efficiently design programs that run numerically intense functions like dense matrix multiplications and discrete fourier transforms on graphic processing units (GPUs). I will also build a custom compiler for this programming language (PL) that will lower my DSL to an intermediate representation (IR) and from IR to GPU machine assembly and ultimately to GPU machine code. The motivation for this independent study is that I love compilers, a passion I discovered after taking CSCI 334: Programming Languages, taught by Daniel Barowy. In CSCI 334 I dipped my toes in programming language theory and development, but I wanted to learn more about PL and the compilers that compile PLs. Over Summer 2025, I worked on my own to gain more knowledge of PLs and compilers, communicating my interests, progress, and experiences with Daniel Barowy. Over the summer I also developed an interest in GPUs and high-performance computing. Compilers and GPUs intertwine greatly, and this area is currently very relevant in academia and industry. Daniel Barowy and I have planned to do this independent study since last spring, and for Winter Study 2026 I did CSCI 32 - Research in Computing in preparation for this independent study."

The repository is split between two directories. The grafting-custom-gpu-binaries holds all the work I did for CSCI 32, during which I learned to reverse engineer GPU binaries (Nvidia ```.fatbin``` files) and interpret/write SASS, CUDA (C++), and PTX. Ultimately, I was able to bypass the CUDA compiler and I realized it was possible to The tutorial for how to achieve what I accomplished is titled "Grafting Custom GPU sections (.cubin files) into a fatbin file & Executing fatbins with the Nvidia Driver API".

The second directory, 2ptx-compiler, is the actual compiler I implemented for CSCI 498. There is a tutorial in the directory that explains how I developed the software, and another file explaining how to run the compiler on some programs. 

CSCI 498 and 32 were both amazing learning experiences for me as an aspiring compiler engineer. I've been diving head first into compilers since last spring, and it has been so cool to continue building in this space. I set a pretty ambitious goal for myself this spring, and although I didn't achieve exactly what I set out to do, I've learned so much in the past few months. 

I hope that anyone who finds this compiler and the "tutorials" I write will benefit from my work! 

GitHub Pages link: [https://ryanashita.github.io/2ptx-Compiler](https://ryanashita.github.io/2ptx-Compiler)

## How to run the 2ptx-compiler
1. Write a program in the DSL. An example is ```2;3;5;x=(3+5);6;7;y=(6+2);```. Some notes for the program:
- semi-colons separate statements
- arithmetic must be in parentheses if the result of the arithmetic is the rvalue for an assignment
2. Navigate to ```~/2ptx-Compiler/2ptx-compiler```
3. Run the command ```cmake --build build```

## Tutorials

### Grafting GPU Binaries
- [tutorial 00: Grafting GPU Binaries](https://ryanashita.github.io/2ptx-Compiler/grafting-custom-gpu-binaries/00-tutorial.html)

### Compiler
- [tutorial 01: Parse Expression Grammar & AST](https://ryanashita.github.io/2ptx-Compiler/2ptx-compiler/01-tutorial.html)
- [tutorial 02: Three-Address Code & Liveness Analysis](https://ryanashita.github.io/2ptx-Compiler/2ptx-compiler/02-tutorial.html)
- [tutorial 03: Register Allocation](https://ryanashita.github.io/2ptx-Compiler/2ptx-compiler/03-tutorial.html)
