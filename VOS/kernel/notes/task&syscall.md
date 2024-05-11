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
| Task Structure | Start from $\texttt{0x0000000000100000}$ |
| User Code | Up aligns the end of previous to $8$ |
| User Data | Up aligns the end of previous to $8$ |
| User Rodata | Up aligns the end of previous to $8$ |
| User Stack | End at $\texttt{0x00007fffffffffff}$ |
| Kernel Program <br>(includes data and code) |  Start from $\texttt{0xffff800000000000}$ |
| DMAS Area | Start from $\texttt{0xffff880000000000}$ |
| Interrupt Stack | End at $\texttt{0xffffffffff800000}$ |
| Kernel Stack | End at $\texttt{0xfffffffffffffff0}$ |

## Processes
### Switch Task
Initially, we can switch task when time interrupt was triggered. Since the registers and other state are stored in the stack of the current task, it can be used directly to restore this task. Additionally, since the task can only be switched out when the interrupt was trigered, so the next task we need to conduct can be also loaded using the save function. Additionally, the instruction ``iretq`` use the ``cs`` stored in the stack to recognize which ring should switch to, and in this project, all the interrupt handler will run in Ring $0$, so, it is not necessary to write different handler function to switch from Ring $3\rightarrow$ Ring $1$, Ring $1\rightarrow$ Ring $1$ ...

The state storing function and state restoring function are the entry and exit functions of interrupt & exception handlers respectively.

Furthermore, to reduce the probability of causing bugs, the screen drawing function ``printStr`` used in ``printk`` can only run in Ring $0$ and all the interrupts will be masked during the process of ``printStr`` .

When a task is in interrupt or trapped state and can not restore itself, for example, accessing an address that is not represented, then because that we masked all the interrupts by default, then we need to enable the interrupts at this time.   