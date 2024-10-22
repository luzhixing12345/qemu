import gdb

# 定义4级页表的常量,通常是针对x86_64架构
PAGE_SHIFT = 12        # 页的大小为2^12 = 4KB
PTE_INDEX_SHIFT = PAGE_SHIFT         # 每个页表项的偏移量
PMD_INDEX_SHIFT = PAGE_SHIFT + 9     # 每级页表增加9位偏移
PUD_INDEX_SHIFT = PMD_INDEX_SHIFT + 9
PGD_INDEX_SHIFT = PUD_INDEX_SHIFT + 9

# 页表级别的掩码,用于计算每级表的索引
PAGE_MASK = (1 << PAGE_SHIFT) - 1
PTE_MASK = ((1 << 9) - 1) << PTE_INDEX_SHIFT
PMD_MASK = ((1 << 9) - 1) << PMD_INDEX_SHIFT
PUD_MASK = ((1 << 9) - 1) << PUD_INDEX_SHIFT
PGD_MASK = ((1 << 9) - 1) << PGD_INDEX_SHIFT

VIRUAL_ADDR = 0xffff888000000000 

class PageTableCalculator(gdb.Command):
    """传入虚拟地址,计算其在四级页表中的偏移量"""

    def __init__(self):
        super(PageTableCalculator, self).__init__("pgd", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        try:
            # 将传入的参数转换为虚拟地址
            virt_addr = int(arg, 16)
            print(f"Virtual address: 0x{virt_addr:x}")
            
            # 计算PGD偏移量
            pgd_offset = (virt_addr & PGD_MASK) >> PGD_INDEX_SHIFT
            pgd_index = pgd_offset << 3
            print(f"PGD Offset: 0x{pgd_offset:x}")
            print(f"PGD Index: 0x{pgd_index:x}")
            
            # 计算PUD偏移量
            pud_offset = (virt_addr & PUD_MASK) >> PUD_INDEX_SHIFT
            pud_index = pud_offset << 3
            print(f"PUD Offset: 0x{pud_offset:x}")
            print(f"PUD Index: 0x{pud_index:x}")
            
            # 计算PMD偏移量
            pmd_offset = (virt_addr & PMD_MASK) >> PMD_INDEX_SHIFT
            pmd_index = pmd_offset << 3
            print(f"PMD Offset: 0x{pmd_offset:x}")
            print(f"PMD Index: 0x{pmd_index:x}")
            
            # 计算PTE偏移量
            pte_offset = (virt_addr & PTE_MASK) >> PTE_INDEX_SHIFT
            pte_index = pte_offset << 3
            print(f"PTE Offset: 0x{pte_offset:x}")
            print(f"PTE Index: 0x{pte_index:x}")
            
            # 计算页内偏移量
            page_offset = virt_addr & PAGE_MASK
            print(f"Page Offset: 0x{page_offset:x}")
            
            print(f"Virtual address mapped from kernel address: 0x{VIRUAL_ADDR:x}")
            
        
        except Exception as e:
            print(f"Error: {e}")

# 注册命令
PageTableCalculator()
