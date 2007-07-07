#ifndef _ASM_LKL_CHECKSUM_H
#define _ASM_LKL_CHECKSUM_H


static inline __sum16 csum_fold(__wsum csum)
{
	u32 sum = (__force u32)csum;
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return (__force __sum16)~sum;
}


/*
 *	This is a version of ip_compute_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 *
 *	By Jorge Cwik <jorge@laser.satlink.net>, adapted for linux by
 *	Arnt Gulbrandsen.
 */
static inline __sum16 ip_fast_csum(const void *iph, unsigned int ihl)
{
	const unsigned int *word = iph;
	const unsigned int *stop = word + ihl;
	unsigned int csum;
	int carry;

	csum = word[0];
	csum += word[1];
	carry = (csum < word[1]);
	csum += carry;

	csum += word[2];
	carry = (csum < word[2]);
	csum += carry;

	csum += word[3];
	carry = (csum < word[3]);
	csum += carry;

	word += 4;
	do {
		csum += *word;
		carry = (csum < *word);
		csum += carry;
		word++;
	} while (word != stop);

	return csum_fold(csum);
}

__wsum csum_partial(const void *buff, int len, __wsum sum);


/*
 * the same as csum_partial, but copies from src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */
static inline __wsum
csum_partial_copy_nocheck(const void *src, void *dst, int len, __wsum sum)
{
	memcpy(dst, src, len);
	return csum_partial(dst, len, sum);
}


/*
 * the same as csum_partial_copy, but copies from user space.
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */
static inline __wsum
csum_partial_copy_from_user(const void __user *src, void *dst,
			    int len, __wsum sum, int *csum_err)
{
	int rem;

	if (csum_err)
		*csum_err = 0;

	rem = copy_from_user(dst, src, len);
	if (rem != 0) {
		if (csum_err)
			*csum_err = -EFAULT;
		memset(dst + len - rem, 0, rem);
		len = rem;
	}

	return csum_partial(dst, len, sum);
}


static inline 
__wsum csum_tcpudp_nofold(__be32 saddr, __be32 daddr,
				   unsigned short len,
				   unsigned short proto,
				   __wsum sum)
{
	u64 result;

	result = (__force u64)saddr + (__force u64)daddr +
		 (__force u64)sum + ((len + proto) << 8);

	/* Fold down to 32-bits so we don't lose in the typedef-less 
	   network stack.  */
	/* 64 to 33 */
	result = (result & 0xffffffff) + (result >> 32);
	/* 33 to 32 */
	result = (result & 0xffffffff) + (result >> 32);
	return (__force __wsum)result;
}


static inline __sum16
csum_tcpudp_magic(__be32 saddr, __be32 daddr, unsigned short len,
		  unsigned short proto, __wsum sum)
{
	return csum_fold(csum_tcpudp_nofold(saddr,daddr,len,proto,sum));
}




/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */
extern __sum16 ip_compute_csum(const void *buff, int len);



#endif
