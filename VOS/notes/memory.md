# Memory Structure

```
>> 0xffffffffffffffff 
Kernel Program 
.->4kb 
Global memory descriptor 
.->4kb 
Buddy memory descriptor 
.->4kb 
Slab cache space
...
>> 0x00007fffffffffff 
Stack Bottom 
Thread Info
    threadMemoryManageStruct 
        pgd_t : vaddress of page table 
        page : the first head page for this thread
...
Basic Page Table
Heap Bottom 
User Program 
>> 0x0000000000000000 
```
# Basic Functions and API

## Allocate memory

for kernel: ``kmalloc()->slab_alloc()``

for user: ``malloc(virtAddr)---->buddy_alloc(virtAddr, pages)->mapPage(phyAddr, virtAddr)``

``buddy_alloc()``中调用``mapPage(phyAddr, virtAddr)``完成页表更新

## Free memory

for kernel: ``kfree()->slab_free()``

for user: ``free(virtAddr)----->buddy_free(virtAddr, pages)->unmapPage(virtAddr)``

# Page Table

## Structure

`orginal page table` 保存了一个最初使用的页表，用于，其 `virtAddr` 和 `phyAddr` 的关系满足：

$$
\mathrm{virtAddr} = \mathrm{phyAddr} + \texttt{0xffff000000000000}
$$

可以用于页表操作  
**PDG** 实际存储结构:
这一级页表为最低级页表，因此不需要记录下一层的虚拟地址。
```
pageItem 0
pageItem 1
...
pageItem 511
```

**PML4E** 和 **PDPTE** 实际存储结构:
```
pageItem 0
pageItem 1
...
pageItem 511
--------------
virtAddr 0
virtAddr 1
...
virtAddr 511
```
每一项 $\text{virtAddr}_i$ 均保存下一级页表的虚拟地址。
当某一项没有映射，即存在 $\text{pageItem}_i = 0$，这时候 $\text{virtAddr}_i=0$ 。

## Operation

### `PageTable_init(phyAddr, virtAddr)`
假设现在需要建立页表的物理地址和虚拟地址分别为`phyAddr`和`virtAddr`，一般而言，$\mathrm{virtAddr} = \texttt{0x0}$, 接下来使用初始页表将phyAddr映射到某个虚拟地址中，假设为`tmpVirtAddr`，一般为内存空洞的开始位置 ，这时候操作`tmpVirtAddr`，可以将`virtAddr`成功映射到`phyAddr`, 接着将 `kernel` 的页表复制到这个新的页表中，然后更改 $\texttt{CR3}$，然后可以使用 `virtAddr` 完成对其余内存的初始化。

### ``PageTable_map(phyAddr, virtAddrSt, virtAddrEd)``
可以用于将物理地址 ``phyAddr`` 映射到虚拟地址 ``virtAddr`` ，这里规定 ``virtAddr`` 一定是用户地址，也就是 $\mathrm{virtAddr}\leq \texttt{0x7fffffffffff}$，因为内核的映射不会改变。

- 如果需要映射的虚拟内存均在已建立页表的范围内，那么可以直接修改相关的页表。

- 如果需要建立新的页表，那么将执行如下程序：
	- 首先检测当前页表所处的物理页是否被占满，如果被占满，那么使用 ``PageTable_extend(phyAddr, virtAddr)`` 将这个页表复制到更大的区域，（一般而言，增加的物理页数量和需要更加的新页表个数相关，新建之后页表项的物理地址可能不再连续），然后将这个进程的页表更换为这个更大的页表。
	- 接着在当前进程的页表中完成映射修改。

修改完成后的页表结构和上述的描述保持一致。

### ``PageTable_unmap(virtAddrSt, virtAddrEd)``
将相关的 ``pdg`` 页表项均设为 $0$ ，然后如果出现某个``pdg`` 均为空，那么将其设为空洞，如果 ``pdpte`` 中的项也均为空，也将这个 ``pdpte`` 设置为空洞，更新空洞的总大小。
如果空洞大小大于当前页表大小的一半，那么重新建立这个页表。

具体而言，就是申请一个新的内存，将原有页表复制进去，然后释放原页表的物理页。

# General Process
实际运行过程中，可以选择当程序使用某个内存时，再分配对应的物理页。
比如，每个进程的栈空间的基地址均为 $\texttt{0x7fffffffffff}$ ，但是堆空间以及程序数据的起始地址是 $\texttt{0x0}$ ，中间的大部分空间一般是不会被使用的，因此：
- 初始的时候，可以首先申请一个页面给栈空间，以及若干个页面给程序数据和堆空间。
- 堆空间：
    - 使用 ``malloc()`` 分配内存的时候，会分配物理页，也就是堆空间是即时分配的。
    - 使用 ``free()`` 释放内存的时候，会产生空洞，因为虚拟地址 $\mathrm{virtAddr}$ 和及其对应的物理地址 $\mathrm{phyAddr}$ 满足如下关系：
    $$
    \mathrm{virtAddr} - 2\texttt{MB}\times \left\lfloor\frac{\mathrm{virtAddr}}{2\texttt{MB}}\right\rfloor
     =
    \mathrm{phyAddr} - 2\texttt{MB}\times \left\lfloor\frac{\mathrm{phyAddr}}{2\texttt{MB}}\right\rfloor
    $$
    因此可以观察空洞大小，如果存在整个物理页都是空洞，那么可以用 ``Buddy_free`` 回收这个页表。
    - PS：回收物理页 $p$ 之前，需要调整 $p$ 所在的物理页块中其他物理页的结构，使其满足 ``Buddy`` 系统的结构，然后再将 $p$ 抽出。
- 当栈生长到某个没有映射物理页的虚拟地址，会触发 ``doPageFault`` ，这时候可以使用 $\texttt{CR2}$ 获得出错的地址，这时候只需要分配一个物理页给这个地址对 $2\texttt{MB}$ 下取整的虚拟地址即可。
- 值得注意的是，当栈空间缩小后，分配到栈的物理页并不会回收。