### malloclab

#### Notes

​	DRAM缓存是全相联的，使用了复杂精密的替换算法。DRAM缓存总是使用写回，而不是直写。

​	naive版的mm.c中mm_init函数是空的，也就是没有空闲块的管理功能。

​	显式链表的缺点是空闲块必须足够大。

#### Details

​	使用分离的空闲链表，搭配分离适配策略：分配器需要维护一个空闲链表数组，但pdf中提到不可以使用，故需要在初始化使用mem_sbrk函数时，顺便在prologue里面实现一个数组。

​	指导文档里有提到：指针操作容易出现一些难以察觉的错误，所以实验过程中debugging占了大量时间。

​	mm_init，extend_heap，mm_free，coalesce，mm_malloc这几个函数在教材中的示例基础上修改。

#### Evaluation

Testing mm malloc
Reading tracefile: short1-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   65%      12  0.000001 17143
Total          65%      12  0.000001 17143

Perf index = 39 (util) + 40 (thru) = 79/100



Testing mm malloc
Reading tracefile: short2-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   89%      12  0.000001 12000
Total          89%      12  0.000001 12000

Perf index = 53 (util) + 40 (thru) = 93/100

#### Ps

​	9.9.11最后一段提到边界标记还可以优化，代码中暂时没有实现。

