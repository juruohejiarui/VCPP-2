# Instructions
To be compatible for both processors of $\texttt{AMD}$ and $\texttt{Intel}$, this project will only use ``syscall/sysret``, excepts ``sysenter/sysexit`` .

## Description
For ``syscall``, before jumping into the entry function, ``rip`` will be stored in ``rcx`` and ``rflags`` will be in ``r11``. Then, the ``cs`` and ``ss`` will be loaded corresponding to the value of msr $\texttt{STAR}$. There is no operation for switching the stack, so, it is our responsibility to manuallly switch the user stack to kernel stack.

For ``sysret``, it does the opposite thing. This instruction will not switch the stack as well.

## Initialization
To enable this function, the $0$-th bit of msr in $\texttt{0xC0000080}$ should be set. Then the segment index of ``cs`` and ``ss`` for user level should be stored in the $48\dots63$-th bits of msr called $\texttt{STAR}$, whose index is $\texttt{0xC0000081}$, which that ``ss`` will be $\texttt{STAR}[48\dots 63]+8$ and ``cs`` will be $\texttt{STAR}[48\dots 63]+16$. Similarly, the ``cs`` and ``ss`` of kernel level are stored in the $32\dots 47$-th bits of $\texttt{STAR}$ . Then, the pointer to the entry function of syscall, should be stored in $\texttt{LSTAR}$ in $\texttt{0xC0000082}$ .

# Task
## Virtual Address Layout
| Description | Range |
| :----: | :-----: |
| Task Structure | Start from $\texttt{0x0000000000000000}$ |
| User Code | Up aligns the end of previous to $8$ |
| User Data | Up aligns the end of previous to $8$ |
| User Rodata | Up aligns the end of previous to $8$ |
| User Stack | End at $\texttt{0x00007fffffffffff}$ |
| Kernel Program <br>(includes data and code) |  Start from $\texttt{0xffff800000000000}$ |
| DMAS Area | Start from $\texttt{0xffff880000000000}$ |
| Kernel Stack | End at $\texttt{0xffffffffffffffff}$ |

## Processes
### Create a Task
