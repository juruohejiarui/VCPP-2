# JIT Implementation
An instruction in ``TCommand table``(These instructions will be called ``VInstruction``) relates to a list of platform-related assembly codes. ``instruction.h\c`` contains the codes for generation of needed assembly codes. And ``tmpl.h\c`` contains the templates for every ``VInstruction``. Then I implements some functions and global variables in ``vm.h\c`` for assembly codes to interact with ``VGVM``, including the ``callStack`` and the runtime block management, etc. 

There are some more details on how to execuate the assembly code in memory.
- For Unix-like operating system, for example, Linux and MacOS, ``mmap`` function in ``sys\mman.h`` was used to allocate an executable memory block.

Because of the stack-based expression evaluation, the used of registers has a higher degree of freedom. (Though code efficiency becomes lower. $\mathrm{TAT}$) . I specified the following rules of using register.

- ``RAX, RBX, RCX, RDX, R8, R9, R10, R11, R12`` can be used to evaluation.
- ``R13`` always stores the address of the top of ``callStack``, except the the preparation phase for calling the ``VGVM`` function.
- ``R14`` always stores the address of the bottom of the calculation stack of the current ``callFrame``, except the the preparation phase for calling the ``VGVM`` function.
- In the assembly code list for ``getarg`` and ``setarg``, ``R15`` stores the address of the first param. In the list for ``vret`` and ``ret``, ``R15`` stores the results of function.
- Process of the preparation phase for calling the ``VGVM`` function:
    - ``R13, R14, R15`` will be pushed into stack. (the native stack)
    - Then prepare the params for the calling.
- Process of the end phase for calling the ``VGVM`` function:
    - Handle the result from the function.
    - Restore the values of ``R13, R14, R15`` from the stack.

Another important point is the code jumping. In assembly code, we can just simply use ``jmp``, ``jz``, ``jnz``, etc to jump to other address. However, if I used the native ``jmp`` family directly, I need to calculate the address of the code again after a the executable memory is created. In other word, I need to scan the assembly code again with complex condition codes. Since that I have no enough time to do time, I just store table of address of the first assembly code (called 'entry') for every ``VInstruction``, then the assembly program just need to get the address from the table and then jump. Additionally, this table can be also used for function call.