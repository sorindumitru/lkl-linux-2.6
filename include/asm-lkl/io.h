#ifndef _ASM_LKL_IO_H
#define _ASM_LKL_IO_H


static inline unsigned char readb(const volatile void __iomem *addr)
{
        BUG();
	return *(volatile unsigned char __force *) addr;
}
static inline unsigned short readw(const volatile void __iomem *addr)
{
        BUG();
	return *(volatile unsigned short __force *) addr;
}
static inline unsigned int readl(const volatile void __iomem *addr)
{
        BUG();
	return *(volatile unsigned int __force *) addr;
}

static inline unsigned long long readq(void __iomem *addr)
{
        BUG();
        return *(volatile unsigned long long __force *) addr;
}

#define inb(x) readb((void*)x)

static inline void writeb(unsigned char b, volatile void __iomem *addr)
{
        BUG();
	*(volatile unsigned char __force *) addr = b;
}

#define outb(c, i) writeb(c, (void*)addr)

static inline void writew(unsigned short b, volatile void __iomem *addr)
{
        BUG();
	*(volatile unsigned short __force *) addr = b;
}
static inline void writel(unsigned int b, volatile void __iomem *addr)
{
        BUG();
	*(volatile unsigned int __force *) addr = b;
}
static inline void writeq(unsigned int b, volatile void __iomem *addr)
{
        BUG();
	*(volatile unsigned long long __force *) addr = b;
}

#define readq_relaxed readq
#define readl_relaxed readl
#define readw_relaxed readw
#define readb_relaxed readb

#define __raw_writeb writeb
#define __raw_writew writew
#define __raw_writel writel
#define __raw_writeq writeq


#define IO_SPACE_LIMIT 0

#endif
