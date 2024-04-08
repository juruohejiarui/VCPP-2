This page aims to record the structure of Global Descriptor Table(GDT), Interrupt Descriptor Table(IDT) and Task State Segment(TSS) in long-mode.

# GDT
## Description
One item of GDT can be described using the following List:

- $0...15$: $0...15$-th bits of **limit**
- $16...31$: $0...15$-th bits of **base**
- $32...39$: $16...23$-th bits of **base**
- $40...47$: **Access Byte**
  - $40$: **A**, accessable. recommended to be $1$
  - $41$: **RW**, readable/writeable.
    - for code segment: readable($1$), unreadable($0$), write access is never allowed for code segments
    - for data segment: writeable($1$), unwriteable($0$), data segments are always readable.
  - $42$: **DC**, direction/conforming.
    - for code segment: $0$: code in this segment can only be executed from the ring set in **DPL**. $1$, code int this segment can be executed from an equal or lower ($\leq$) privilege level.
    - for data segment: $0$: grows up; $1$ grows down.
  - $43$: **E**, executable. $0$: this segment is a data segment; $1$: this segment is a code segment.
  - $44$: **Descriptor type**. $0$: defines a system segment (e.g. a Task State Segment); $1$: defines a code or data segment
  - $45...46$: **privilege level**. $0~\mathrm{(highest)}...3~\mathrm{(lowest)}$
  - $47$: **present**. must be $1$.
- $48...51$: $16...19$-th bits of **limit**.
- $52...55$: **Flags**
  - $52$: reserved. must be $0$
  - $53$: **L**, long-mode code flag. $1$: descriptor defines a 64-bit code segment, then the DV should be $0$ ; $1$, otherwise. 
  - $54$: **DB**, size. $0$: defines a 16-bit protected mode segment; $1$: defines a 32-bit protected mode segment.
  - $55$: **G**, Granularity. $0$: the **limit** is in 1 byte blocks; $1$: the **limit** is in $4\texttt{KB}$ blocks.
- $56...63$: $24...31$-th bits for **base**.

There are something different for the system segment. Specifically, the descriptor is 128-bit long. And the $64...95$-th bits stores the $32...63$-th bits of **base**, while the $96...127$-th bits are reserved and the meaning of other bits are the same as those in descriptor for code and data segment.
  
a GDT is a block of memory following the format that shown below:
|Address|Description|
|:---:|:---:|
|GDT Base Address $+ 0$ |must be $0$|
|GDT Base Address $+ 8$ |entry $1$|
|GDT Base Address $+ 16$ |entry $2$|
|$\vdots$|$\vdots$|

The address of GDT is stored in the register ``gdtr``(GDT register).
The instruction ``lgdt`` can be applied to update ``gdtr``, which accepts a value as the pointer to the GDT base address.

## Practice
From the forms above, we can write the segment descriptor for 64-bit flag mode.

For the 64-bit kernel code segment, we have:
```
P(47th) = DescriptorType(44th) = E(43th) = 1
L(53th) = 1
```
then the value of this descriptor should be $\texttt{0x0020980000000000}$

For the 64-bit kernel data segment, we have:
```
P(47th) = DescriptorType(44th) = RW(42th) = 1
```
Then the value should be $\texttt{0x0000920000000000}$

The following code shown the initial GDT, which is used in the initialization period of kernel program:

```cpp
ENTRY(gdtTable)
    .quad 0x0000000000000000    /* NULL */
    .quad 0x0020980000000000    /* kernel code 64-bit */
    .quad 0x0000920000000000    /* kernel data 64-bit */
    .quad 0x0000000000000000 
    .quad 0x0000000000000000
    .quad 0x0020f80000000000    /* user code 64-bit */
    .quad 0x0000f20000000000    /* user data 64-bit */
    .quad 0x00cf9a000000ffff    /* kernel code 32-bit */
    .quad 0x00cf92000000ffff    /* kernel data 32-bit */
    .fill 10, 8, 0				/* TSS (jmp one segment <9>) in long-mode 128-bit 50*/
gdtEnd:
```

# TSS
## Description
The structure and application of TSS in 32-bit mode and 64-bit mode are quite different, and we will specifically focus on 64-bit mode.

$\texttt{rsp}0...\texttt{rsp}2$ are stack pointer used to load the stack when a privilege level changes from lower level to higher level.

$\texttt{IST}1...\texttt{IST}7$ are stack pointer used to load the stack when an entry in IDT has an **IST** value other than $0$.


|Item Name|Offset             |Length       |
|---|---|---|
|Reserved|$\texttt{0x00}$    |$4\texttt{B}$|
|$\texttt{rsp}0$|$\texttt{0x04}$    |$8\texttt{B}$|
|$\texttt{rsp}1$|$\texttt{0x0C}$    |$8\texttt{B}$|
|$\texttt{rsp}2$|$\texttt{0x14}$    |$8\texttt{B}$|
|Reserved|$\texttt{0x1C}$    |$8\texttt{B}$|
|$\texttt{IST}1...\texttt{IST}7$    |$\texttt{0x24}$ |$8\texttt{B}\times 7$|
|Reserved|$\texttt{0x5C}$    |$12\texttt{B}$|
|IOPB|$\texttt{0x66}$ |$2\texttt{B}$|

Instruction ``ltr`` can be used to load the TSS, which accepts an offset based on GDT as the description of TSS, then, modifies the register ``tr``.

For example, 
```as
// assume that the offset of TSS descriptor is on GDT base address + 0x10
mov $0x10, %ax
ltr $ax
```
can load the TSS in ``gdtr + 0x50``.

## Practice
The first step is to set up the descriptor of TSS in GDT.
Following the description above, we can write down the value of each bit of the descirptor.

Let $A$ be the address of TSS Table.
Then the descriptor can be written like:
```cpp
((A>>32)<<64) | (((A>>24)&((1<<8)-1))<<56) | (Flag=0) | present(47th) | E(43th) | A(40th) | ((A&0xffffff)<<16) | (0x103)
```
Then comes to applying this TSS to CPU, since that we have modified the descriptor in the GDT, then we can simply use assembly code to load it.
```as
mov 0x50, %rax
ltr %rax
```
# IDT
## Description
There are at most $256$ items in one IDT, and every item is a 128-bit integer. Once an interrupt, trap or error is triggered, CPU will search for the description on the IDT, then jump to the corresponded program, which aims to handle the interrupt/trap/error. If there is no description for this interrupt/trap/error, then "general protection" will be triggered.

The item of IDT is called **Interrupt Vector** or **Gate Descriptor**, which can be descripted using the following list:

- $0...15$: $0...15$-th bits of **Offset**
- $16...31$: the segment selector $\in[0, 2^{15}-1]$
- $32...34$: the IST index. if $=0$, then the IST will not be used.
- $35...39$: reserved.
- $40...43$: Gate Type. $\texttt{0xE}$: This is a 64-bit interrupt gate. $\texttt{0xF}$: This is a 64-bit Trap gate.
- $44$: should be $0$.
- $45...46$: **DPL**, Processor privilege level. Used to Defined which are allowed to access this interrupt via ``int`` instruction. Hardware interrupt ignores this mechanism.
- $47$: Must be $1$
- $48...63$: the $16...31$-th bits of **Offset**
- $64...95$: the $32...63$-th bits of **Offset**
- $96...127$: reserved.