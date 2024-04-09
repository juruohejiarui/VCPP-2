# Segment Registers
- ``ds``: Data Segment
- ``es``: Extend Segment
- ``cs``: Code Segment
- ``ss``: Stack Segment
- ``fs`` and ``gs``: General-purpose Segment 

In long-mode, ``ds, es, ss, cs`` are treated as their **base** was $0$ no matter what the descriptor is. The limit check is disabled.

# Paging and Page Table
In long-mode, the size of one address is $64$ bits, but the highest 8 bits are just the copy of the $47$ bit (counting from $0$). Therefore the effective number of digits in a memory address is $48$. 

While the segment memory management is nearly disabled, paging system is used to convert virtual address to physics address, allowing the isolation of processes and giving convenience for implementation of non contiguous memory occupation of one process. Paging divides an virtual address into $5$ parts from higher bits to lower bits. The first $4$ parts are all 9-bit long, and is used as the index of one level of page table. And the last part is $12$-bit long and used as the offset inside page.

In my project, $4$ levels of page table are named from highest level to lowest level as Page Global Directory (PGD), Page Upper Directory (PUD), Page Middle Directory (PMD), Page Lower Directory (PLD).

For one page table, base physics address should be aligned to $4\texttt{KB}$. There are $512$ items, which called "entries", in one pages, each item is a $64$-bit integer. Thus, the size of one page table is $512 \times 8\texttt{B} = 4\texttt{KB}$ .

The structure of items in each level is different. For items of PGD, PUD and PMD, one item contains the pointer (a physics address) to the next level page table, along with some attributes of the memory that managed by this item. While PLD contains the base physics address managed by this page and some attributes. The details of these structures will be shown below.

The address of PGD is stored in register ``CR3``, and ``movq`` can be used to modify and access it. Because of the effect of TLB (a cache for speeding up the convertion), the adjustment of page table may not make effects immediately, but ``movq xxx, %cr3`` can compulsorily flush the TLB.

In some scenarios, PLD would disappear (when the **PS** bit of PMD is set to $0$, which will be introduced later) and the $0...21$-th bits will be directly used as the offset inside page, which means that we use $2\texttt{MB}$ pages instead of $4\texttt{KB}$ pages. In other scenarios, both PLD and PMD would disapper and the $0...30$-th bits will be directly used as the offset inside page

In the initialization period, due to the limitation of space usage, I disable the PLD and use $2\texttt{MB}$ pages instead.

## Description of Items
Let me first introduce the names and meaning of the attribute bits.
- **P**, Present. it should be $1$.
- **R/W**, Read/Write. $1$: Allows to write this memory $0$: Not allowed to write. and this page is read-only
- **U/S**, User/Supervisor. $1$: This page can be accessed by all privilege level. $0$: user is not allowed to use this page.
- **PWT**, Page-level Write-Through. $1$: Write-though caching is enabled. $0$: Write-back is enabled.
- **PCD**, Cache Disable. $1$: This page will not be cached. $0$: will be cached.
- **A**, Accessed. This bit is to shown whether a this item was used during converting, which will be adjust by MMU (the convertion tool of CPU) automatically.
- **D**, Dirty. This bit is used to shown whether a page has been written to.
- **PS**, Page Size. $1$: This item directly maps to a page. $0$: This item leads to a page table of the next level.
- **G**, Global. This bit tells the processor not to invalidate the TLB entry corresponding to the page upon a instruction ``mov`` to ``CR3`` instruction. And the **PGE** bit of ``CR4`` (the $7$-th bit) should be set. In this project, this bit should be $0$. 

PS: I ignore some useless attribute bit.

### PGD
A PGD can manage the whole virtual address space, while one entry of PGD can manage $512\texttt{GB}$ .
- $0$: **P**
- $1$: **R/W**
- $2$: **U/S**
- $3$: **PWT**
- $4$: **PCD**
- $5$: **A**
- $6$: Ignored.
- $7$: **PS**, Reserved and must be $0$.
- $8...10$: Ignored.
- $11$: **R**, something not so important and can will be set to $0$ .
- $12...47$: The $12...47$-th of the $4\texttt{KB}$-aligned address of the PUD referenced by this item.
- $48...62$: From $48...51$-th bits, which are reserved, the values should be $0$. And the $52...62$-th bits are ignored.
- $63$: In this project, this bit should be $0$.

### PUD
- $0...5$: These bits are the same as those of PGD
- $6$: **D**
- $7$: **PS**. $1$: this entry directly maps to a $1\texttt{GB}$ page and then $12$-th bit of this entry is **PAT** bit. $0$: this entry refers to a PMD. During this project, this bit is always $0$.
- $8$: **G**. If **PS** is $0$, then this bit will be ignore. But as what I said before, throughout this project, this bit will be always $0$.
- $9...10$: Ignored.
- $11$: **R**.
- if **PS**=$1$:
  - $12$: **PAT**
  - $13...29$: Reserved and should be $0$.
  - $30...47$: The $30...47$-th bits of the $1\texttt{GB}$-aligned physics page referenced by this entry.
- if **PS**=$0$:
  - $12...47$: The $12...47$-th of the $4\texttt{KB}$-aligned address of the PMD referenced by this item.
- The rest bits are the same as those of PGD.

### PMD
The description of PMD is similar to PUD, so I will not show it again. The only difference is that when **PS**=1, the $21...48$-th bits reference to the $2\texttt{MB}$-aligned physics page.

### PLD
- $0...11$: These bits are the same as those in PGD. But the **PS** bit should be zero.
- $12...47$: The $12...47$-th of the $4\texttt{KB}$-aligned physics page referenced by this entry.
- $47...63$: These bits are the same as those in PGD.

# Memory Management
## Initialzation Process
During the initialization period, the kernel program will only use the mapped address space in ``head.S``, whose size is below $512\texttt{MB}$. Then the buddy system is initalized. Then a 'directly mapping address space' (DMAS) is initalized (this idea is copy from one concept of linux kernel), whose virtual address space is $[\texttt{0xffff880000000000}, \texttt{0xffffC80000000000}]$, whose size is $64\texttt{TB}$ and the relationship of the physics address and the virtual address of this space is
$$
\text{physics address}=\text{virtual address} - \texttt{0xffff880000000000}
$$
The page table of this DMAS is $2\texttt{MB}$ page table or maybe a $1\texttt{GB}$ page table, whose size relates to the actual size of the machine memory space. and it is stored in the space following the kernel pragram. Then the SLAB system is finally initialized, since it will use the management APIs of page table management and buddy system.

## APIs and Algorithms behind Them