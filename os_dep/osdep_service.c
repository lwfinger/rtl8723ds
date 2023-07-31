/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/


#define _OSDEP_SERVICE_C_

#include <drv_types.h>

#define RT_TAG	'1178'

#ifdef DBG_MEMORY_LEAK
atomic_t _malloc_cnt = ATOMIC_INIT(0);
atomic_t _malloc_size = ATOMIC_INIT(0);
#endif /* DBG_MEMORY_LEAK */

/*
* Translate the OS dependent @param error_code to OS independent RTW_STATUS_CODE
* @return: one of RTW_STATUS_CODE
*/
inline int RTW_STATUS_CODE(int error_code)
{
	if (error_code >= 0)
		return _SUCCESS;

	switch (error_code) {
	/* case -ETIMEDOUT: */
	/*	return RTW_STATUS_TIMEDOUT; */
	default:
		return _FAIL;
	}
}

u32 rtw_atoi(u8 *s)
{

	int num = 0, flag = 0;
	int i;
	for (i = 0; i <= strlen(s); i++) {
		if (s[i] >= '0' && s[i] <= '9')
			num = num * 10 + s[i] - '0';
		else if (s[0] == '-' && i == 0)
			flag = 1;
		else
			break;
	}

	if (flag == 1)
		num = num * -1;

	return num;

}

inline u8 *_rtw_vmalloc(u32 sz)
{
	u8	*pbuf;
	pbuf = vmalloc(sz);

#ifdef DBG_MEMORY_LEAK
	if (pbuf != NULL) {
		atomic_inc(&_malloc_cnt);
		atomic_add(sz, &_malloc_size);
	}
#endif /* DBG_MEMORY_LEAK */

	return pbuf;
}

inline u8 *_rtw_zvmalloc(u32 sz)
{
	u8	*pbuf;
	pbuf = _rtw_vmalloc(sz);
	if (pbuf != NULL)
		memset(pbuf, 0, sz);
	return pbuf;
}

inline void _rtw_vmfree(u8 *pbuf, u32 sz)
{
	vfree(pbuf);
#ifdef DBG_MEMORY_LEAK
	atomic_dec(&_malloc_cnt);
	atomic_sub(sz, &_malloc_size);
#endif /* DBG_MEMORY_LEAK */
}

u8 *_rtw_malloc(u32 sz)
{

	u8	*pbuf = NULL;

#ifdef RTK_DMP_PLATFORM
	if (sz > 0x4000)
		pbuf = (u8 *)dvr_malloc(sz);
	else
#endif
		pbuf = kmalloc(sz, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);

#ifdef DBG_MEMORY_LEAK
	if (pbuf != NULL) {
		atomic_inc(&_malloc_cnt);
		atomic_add(sz, &_malloc_size);
	}
#endif /* DBG_MEMORY_LEAK */

	return pbuf;

}


u8 *_rtw_zmalloc(u32 sz)
{
	u8	*pbuf = _rtw_malloc(sz);

	if (pbuf != NULL) {
		memset(pbuf, 0, sz);
	}

	return pbuf;
}

void	_rtw_mfree(u8 *pbuf, u32 sz)
{

#ifdef RTK_DMP_PLATFORM
	if (sz > 0x4000)
		dvr_free(pbuf);
	else
#endif
		kfree(pbuf);

#ifdef DBG_MEMORY_LEAK
	atomic_dec(&_malloc_cnt);
	atomic_sub(sz, &_malloc_size);
#endif /* DBG_MEMORY_LEAK */

}

inline struct sk_buff *_rtw_skb_alloc(u32 sz)
{
	return __dev_alloc_skb(sz, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
}

inline void _rtw_skb_free(struct sk_buff *skb)
{
	dev_kfree_skb_any(skb);
}

inline struct sk_buff *_rtw_skb_copy(const struct sk_buff *skb)
{
	return skb_copy(skb, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
}

inline struct sk_buff *_rtw_skb_clone(struct sk_buff *skb)
{
	return skb_clone(skb, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
}
inline struct sk_buff *_rtw_pskb_copy(struct sk_buff *skb)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36))
	return pskb_copy(skb, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
#else
	return skb_clone(skb, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
#endif
}

inline int _rtw_netif_rx(_nic_hdl ndev, struct sk_buff *skb)
{
	skb->dev = ndev;
	return netif_rx(skb);
}

#ifdef CONFIG_RTW_NAPI
inline int _rtw_netif_receive_skb(_nic_hdl ndev, struct sk_buff *skb)
{
	skb->dev = ndev;
	return netif_receive_skb(skb);
}

#ifdef CONFIG_RTW_GRO
inline gro_result_t _rtw_napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	return napi_gro_receive(napi, skb);
}
#endif /* CONFIG_RTW_GRO */
#endif /* CONFIG_RTW_NAPI */

void _rtw_skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;

	while ((skb = skb_dequeue(list)) != NULL)
		_rtw_skb_free(skb);
}

#ifdef CONFIG_USB_HCI
inline void *_rtw_usb_buffer_alloc(struct usb_device *dev, size_t size, dma_addr_t *dma)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	return usb_alloc_coherent(dev, size, (in_interrupt() ? GFP_ATOMIC : GFP_KERNEL), dma);
#else
	return usb_buffer_alloc(dev, size, (in_interrupt() ? GFP_ATOMIC : GFP_KERNEL), dma);
#endif

inline void _rtw_usb_buffer_free(struct usb_device *dev, size_t size, void *addr, dma_addr_t dma)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	usb_free_coherent(dev, size, addr, dma);
#else
	usb_buffer_free(dev, size, addr, dma);
#endif
}
#endif /* CONFIG_USB_HCI */

#if defined(DBG_MEM_ALLOC)

struct rtw_mem_stat {
	ATOMIC_T alloc; /* the memory bytes we allocate currently */
	ATOMIC_T peak; /* the peak memory bytes we allocate */
	ATOMIC_T alloc_cnt; /* the alloc count for alloc currently */
	ATOMIC_T alloc_err_cnt; /* the error times we fail to allocate memory */
};

struct rtw_mem_stat rtw_mem_type_stat[mstat_tf_idx(MSTAT_TYPE_MAX)];
#ifdef RTW_MEM_FUNC_STAT
struct rtw_mem_stat rtw_mem_func_stat[mstat_ff_idx(MSTAT_FUNC_MAX)];
#endif

char *MSTAT_TYPE_str[] = {
	"VIR",
	"PHY",
	"SKB",
	"USB",
};

#ifdef RTW_MEM_FUNC_STAT
char *MSTAT_FUNC_str[] = {
	"UNSP",
	"IO",
	"TXIO",
	"RXIO",
	"TX",
	"RX",
};
#endif

void rtw_mstat_dump(void *sel)
{
	int i;
	int value_t[4][mstat_tf_idx(MSTAT_TYPE_MAX)];
#ifdef RTW_MEM_FUNC_STAT
	int value_f[4][mstat_ff_idx(MSTAT_FUNC_MAX)];
#endif

	int vir_alloc, vir_peak, vir_alloc_err, phy_alloc, phy_peak, phy_alloc_err;
	int tx_alloc, tx_peak, tx_alloc_err, rx_alloc, rx_peak, rx_alloc_err;

	for (i = 0; i < mstat_tf_idx(MSTAT_TYPE_MAX); i++) {
		value_t[0][i] = ATOMIC_READ(&(rtw_mem_type_stat[i].alloc));
		value_t[1][i] = ATOMIC_READ(&(rtw_mem_type_stat[i].peak));
		value_t[2][i] = ATOMIC_READ(&(rtw_mem_type_stat[i].alloc_cnt));
		value_t[3][i] = ATOMIC_READ(&(rtw_mem_type_stat[i].alloc_err_cnt));
	}

#ifdef RTW_MEM_FUNC_STAT
	for (i = 0; i < mstat_ff_idx(MSTAT_FUNC_MAX); i++) {
		value_f[0][i] = ATOMIC_READ(&(rtw_mem_func_stat[i].alloc));
		value_f[1][i] = ATOMIC_READ(&(rtw_mem_func_stat[i].peak));
		value_f[2][i] = ATOMIC_READ(&(rtw_mem_func_stat[i].alloc_cnt));
		value_f[3][i] = ATOMIC_READ(&(rtw_mem_func_stat[i].alloc_err_cnt));
	}
#endif

	RTW_PRINT_SEL(sel, "===================== MSTAT =====================\n");
	RTW_PRINT_SEL(sel, "%4s %10s %10s %10s %10s\n", "TAG", "alloc", "peak", "aloc_cnt", "err_cnt");
	RTW_PRINT_SEL(sel, "-------------------------------------------------\n");
	for (i = 0; i < mstat_tf_idx(MSTAT_TYPE_MAX); i++)
		RTW_PRINT_SEL(sel, "%4s %10d %10d %10d %10d\n", MSTAT_TYPE_str[i], value_t[0][i], value_t[1][i], value_t[2][i], value_t[3][i]);
#ifdef RTW_MEM_FUNC_STAT
	RTW_PRINT_SEL(sel, "-------------------------------------------------\n");
	for (i = 0; i < mstat_ff_idx(MSTAT_FUNC_MAX); i++)
		RTW_PRINT_SEL(sel, "%4s %10d %10d %10d %10d\n", MSTAT_FUNC_str[i], value_f[0][i], value_f[1][i], value_f[2][i], value_f[3][i]);
#endif
}

void rtw_mstat_update(const enum mstat_f flags, const MSTAT_STATUS status, u32 sz)
{
	static u32 update_time = 0;
	int peak, alloc;
	int i;

	/* initialization */
	if (!update_time) {
		for (i = 0; i < mstat_tf_idx(MSTAT_TYPE_MAX); i++) {
			ATOMIC_SET(&(rtw_mem_type_stat[i].alloc), 0);
			ATOMIC_SET(&(rtw_mem_type_stat[i].peak), 0);
			ATOMIC_SET(&(rtw_mem_type_stat[i].alloc_cnt), 0);
			ATOMIC_SET(&(rtw_mem_type_stat[i].alloc_err_cnt), 0);
		}
		#ifdef RTW_MEM_FUNC_STAT
		for (i = 0; i < mstat_ff_idx(MSTAT_FUNC_MAX); i++) {
			ATOMIC_SET(&(rtw_mem_func_stat[i].alloc), 0);
			ATOMIC_SET(&(rtw_mem_func_stat[i].peak), 0);
			ATOMIC_SET(&(rtw_mem_func_stat[i].alloc_cnt), 0);
			ATOMIC_SET(&(rtw_mem_func_stat[i].alloc_err_cnt), 0);
		}
		#endif
	}

	switch (status) {
	case MSTAT_ALLOC_SUCCESS:
		ATOMIC_INC(&(rtw_mem_type_stat[mstat_tf_idx(flags)].alloc_cnt));
		alloc = ATOMIC_ADD_RETURN(&(rtw_mem_type_stat[mstat_tf_idx(flags)].alloc), sz);
		peak = ATOMIC_READ(&(rtw_mem_type_stat[mstat_tf_idx(flags)].peak));
		if (peak < alloc)
			ATOMIC_SET(&(rtw_mem_type_stat[mstat_tf_idx(flags)].peak), alloc);

		#ifdef RTW_MEM_FUNC_STAT
		ATOMIC_INC(&(rtw_mem_func_stat[mstat_ff_idx(flags)].alloc_cnt));
		alloc = ATOMIC_ADD_RETURN(&(rtw_mem_func_stat[mstat_ff_idx(flags)].alloc), sz);
		peak = ATOMIC_READ(&(rtw_mem_func_stat[mstat_ff_idx(flags)].peak));
		if (peak < alloc)
			ATOMIC_SET(&(rtw_mem_func_stat[mstat_ff_idx(flags)].peak), alloc);
		#endif
		break;

	case MSTAT_ALLOC_FAIL:
		ATOMIC_INC(&(rtw_mem_type_stat[mstat_tf_idx(flags)].alloc_err_cnt));
		#ifdef RTW_MEM_FUNC_STAT
		ATOMIC_INC(&(rtw_mem_func_stat[mstat_ff_idx(flags)].alloc_err_cnt));
		#endif
		break;

	case MSTAT_FREE:
		ATOMIC_DEC(&(rtw_mem_type_stat[mstat_tf_idx(flags)].alloc_cnt));
		ATOMIC_SUB(&(rtw_mem_type_stat[mstat_tf_idx(flags)].alloc), sz);
		#ifdef RTW_MEM_FUNC_STAT
		ATOMIC_DEC(&(rtw_mem_func_stat[mstat_ff_idx(flags)].alloc_cnt));
		ATOMIC_SUB(&(rtw_mem_func_stat[mstat_ff_idx(flags)].alloc), sz);
		#endif
		break;
	};

	/* if (rtw_get_passing_time_ms(update_time) > 5000) { */
	/*	rtw_mstat_dump(RTW_DBGDUMP); */
	update_time = rtw_get_current_time();
	/* } */
}

#ifndef SIZE_MAX
	#define SIZE_MAX (~(size_t)0)
#endif

struct mstat_sniff_rule {
	enum mstat_f flags;
	size_t lb;
	size_t hb;
};

struct mstat_sniff_rule mstat_sniff_rules[] = {
	{MSTAT_TYPE_PHY, 4097, SIZE_MAX},
};

int mstat_sniff_rule_num = sizeof(mstat_sniff_rules) / sizeof(struct mstat_sniff_rule);

bool match_mstat_sniff_rules(const enum mstat_f flags, const size_t size)
{
	int i;
	for (i = 0; i < mstat_sniff_rule_num; i++) {
		if (mstat_sniff_rules[i].flags == flags
			&& mstat_sniff_rules[i].lb <= size
			&& mstat_sniff_rules[i].hb >= size)
			return _TRUE;
	}

	return _FALSE;
}

inline u8 *dbg_rtw_vmalloc(u32 sz, const enum mstat_f flags, const char *func, const int line)
{
	u8  *p;

	if (match_mstat_sniff_rules(flags, sz))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%d)\n", func, line, __FUNCTION__, (sz));

	p = _rtw_vmalloc((sz));

	rtw_mstat_update(
		flags
		, p ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, sz
	);

	return p;
}

inline u8 *dbg_rtw_zvmalloc(u32 sz, const enum mstat_f flags, const char *func, const int line)
{
	u8 *p;

	if (match_mstat_sniff_rules(flags, sz))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%d)\n", func, line, __FUNCTION__, (sz));

	p = _rtw_zvmalloc((sz));

	rtw_mstat_update(
		flags
		, p ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, sz
	);

	return p;
}

inline void dbg_rtw_vmfree(u8 *pbuf, u32 sz, const enum mstat_f flags, const char *func, const int line)
{

	if (match_mstat_sniff_rules(flags, sz))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%d)\n", func, line, __FUNCTION__, (sz));

	_rtw_vmfree((pbuf), (sz));

	rtw_mstat_update(
		flags
		, MSTAT_FREE
		, sz
	);
}

inline u8 *dbg_rtw_malloc(u32 sz, const enum mstat_f flags, const char *func, const int line)
{
	u8 *p;

	if (match_mstat_sniff_rules(flags, sz))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%d)\n", func, line, __FUNCTION__, (sz));

	p = _rtw_malloc((sz));

	rtw_mstat_update(
		flags
		, p ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, sz
	);

	return p;
}

inline u8 *dbg_rtw_zmalloc(u32 sz, const enum mstat_f flags, const char *func, const int line)
{
	u8 *p;

	if (match_mstat_sniff_rules(flags, sz))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%d)\n", func, line, __FUNCTION__, (sz));

	p = _rtw_zmalloc((sz));

	rtw_mstat_update(
		flags
		, p ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, sz
	);

	return p;
}

inline void dbg_rtw_mfree(u8 *pbuf, u32 sz, const enum mstat_f flags, const char *func, const int line)
{
	if (match_mstat_sniff_rules(flags, sz))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%d)\n", func, line, __FUNCTION__, (sz));

	_rtw_mfree((pbuf), (sz));

	rtw_mstat_update(
		flags
		, MSTAT_FREE
		, sz
	);
}

inline struct sk_buff *dbg_rtw_skb_alloc(unsigned int size, const enum mstat_f flags, const char *func, int line)
{
	struct sk_buff *skb;
	unsigned int truesize = 0;

	skb = _rtw_skb_alloc(size);

	if (skb)
		truesize = skb->truesize;

	if (!skb || truesize < size || match_mstat_sniff_rules(flags, truesize))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%d), skb:%p, truesize=%u\n", func, line, __FUNCTION__, size, skb, truesize);

	rtw_mstat_update(
		flags
		, skb ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, truesize
	);

	return skb;
}

inline void dbg_rtw_skb_free(struct sk_buff *skb, const enum mstat_f flags, const char *func, int line)
{
	unsigned int truesize = skb->truesize;

	if (match_mstat_sniff_rules(flags, truesize))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s, truesize=%u\n", func, line, __FUNCTION__, truesize);

	_rtw_skb_free(skb);

	rtw_mstat_update(
		flags
		, MSTAT_FREE
		, truesize
	);
}

inline struct sk_buff *dbg_rtw_skb_copy(const struct sk_buff *skb, const enum mstat_f flags, const char *func, const int line)
{
	struct sk_buff *skb_cp;
	unsigned int truesize = skb->truesize;
	unsigned int cp_truesize = 0;

	skb_cp = _rtw_skb_copy(skb);
	if (skb_cp)
		cp_truesize = skb_cp->truesize;

	if (!skb_cp || cp_truesize < truesize || match_mstat_sniff_rules(flags, cp_truesize))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%u), skb_cp:%p, cp_truesize=%u\n", func, line, __FUNCTION__, truesize, skb_cp, cp_truesize);

	rtw_mstat_update(
		flags
		, skb_cp ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, truesize
	);

	return skb_cp;
}

inline struct sk_buff *dbg_rtw_skb_clone(struct sk_buff *skb, const enum mstat_f flags, const char *func, const int line)
{
	struct sk_buff *skb_cl;
	unsigned int truesize = skb->truesize;
	unsigned int cl_truesize = 0;

	skb_cl = _rtw_skb_clone(skb);
	if (skb_cl)
		cl_truesize = skb_cl->truesize;

	if (!skb_cl || cl_truesize < truesize || match_mstat_sniff_rules(flags, cl_truesize))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%u), skb_cl:%p, cl_truesize=%u\n", func, line, __FUNCTION__, truesize, skb_cl, cl_truesize);

	rtw_mstat_update(
		flags
		, skb_cl ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, truesize
	);

	return skb_cl;
}

inline int dbg_rtw_netif_rx(_nic_hdl ndev, struct sk_buff *skb, const enum mstat_f flags, const char *func, int line)
{
	int ret;
	unsigned int truesize = skb->truesize;

	if (match_mstat_sniff_rules(flags, truesize))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s, truesize=%u\n", func, line, __FUNCTION__, truesize);

	ret = _rtw_netif_rx(ndev, skb);

	rtw_mstat_update(
		flags
		, MSTAT_FREE
		, truesize
	);

	return ret;
}

#ifdef CONFIG_RTW_NAPI
inline int dbg_rtw_netif_receive_skb(_nic_hdl ndev, struct sk_buff *skb, const enum mstat_f flags, const char *func, int line)
{
	int ret;
	unsigned int truesize = skb->truesize;

	if (match_mstat_sniff_rules(flags, truesize))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s, truesize=%u\n", func, line, __FUNCTION__, truesize);

	ret = _rtw_netif_receive_skb(ndev, skb);

	rtw_mstat_update(
		flags
		, MSTAT_FREE
		, truesize
	);

	return ret;
}

#ifdef CONFIG_RTW_GRO
inline gro_result_t dbg_rtw_napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb, const enum mstat_f flags, const char *func, int line)
{
	int ret;
	unsigned int truesize = skb->truesize;

	if (match_mstat_sniff_rules(flags, truesize))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s, truesize=%u\n", func, line, __FUNCTION__, truesize);

	ret = _rtw_napi_gro_receive(napi, skb);

	rtw_mstat_update(
		flags
		, MSTAT_FREE
		, truesize
	);

	return ret;
}
#endif /* CONFIG_RTW_GRO */
#endif /* CONFIG_RTW_NAPI */

inline void dbg_rtw_skb_queue_purge(struct sk_buff_head *list, enum mstat_f flags, const char *func, int line)
{
	struct sk_buff *skb;

	while ((skb = skb_dequeue(list)) != NULL)
		dbg_rtw_skb_free(skb, flags, func, line);
}

#ifdef CONFIG_USB_HCI
inline void *dbg_rtw_usb_buffer_alloc(struct usb_device *dev, size_t size, dma_addr_t *dma, const enum mstat_f flags, const char *func, int line)
{
	void *p;

	if (match_mstat_sniff_rules(flags, size))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%zu)\n", func, line, __FUNCTION__, size);

	p = _rtw_usb_buffer_alloc(dev, size, dma);

	rtw_mstat_update(
		flags
		, p ? MSTAT_ALLOC_SUCCESS : MSTAT_ALLOC_FAIL
		, size
	);

	return p;
}

inline void dbg_rtw_usb_buffer_free(struct usb_device *dev, size_t size, void *addr, dma_addr_t dma, const enum mstat_f flags, const char *func, int line)
{

	if (match_mstat_sniff_rules(flags, size))
		RTW_INFO("DBG_MEM_ALLOC %s:%d %s(%zu)\n", func, line, __FUNCTION__, size);

	_rtw_usb_buffer_free(dev, size, addr, dma);

	rtw_mstat_update(
		flags
		, MSTAT_FREE
		, size
	);
}
#endif /* CONFIG_USB_HCI */

#endif /* defined(DBG_MEM_ALLOC) */

void *rtw_malloc2d(int h, int w, size_t size)
{
	int j;

	void **a = (void **) rtw_zmalloc(h * sizeof(void *) + h * w * size);
	if (a == NULL) {
		RTW_INFO("%s: alloc memory fail!\n", __FUNCTION__);
		return NULL;
	}

	for (j = 0; j < h; j++)
		a[j] = ((char *)(a + h)) + j * w * size;

	return a;
}

void rtw_mfree2d(void *pbuf, int h, int w, int size)
{
	rtw_mfree((u8 *)pbuf, h * sizeof(void *) + w * h * size);
}

void _rtw_memcpy(void *dst, const void *src, u32 sz)
{
	memcpy(dst, src, sz);
}

inline void _rtw_memmove(void *dst, const void *src, u32 sz)
{
	memmove(dst, src, sz);
}

int	_rtw_memcmp(const void *dst, const void *src, u32 sz)
{

	/* under Linux/GNU/GLibc, the return value of memcmp for two same mem. chunk is 0 */

	if (!(memcmp(dst, src, sz)))
		return _TRUE;
	else
		return _FALSE;
}

void _rtw_memset(void *pbuf, int c, u32 sz)
{
	memset(pbuf, c, sz);
}

void _rtw_init_listhead(_list *list)
{
	INIT_LIST_HEAD(list);
}

/*
For the following list_xxx operations,
caller must guarantee the atomic context.
Otherwise, there will be racing condition.
*/
u32	rtw_is_list_empty(_list *phead)
{
	if (list_empty(phead))
		return _TRUE;
	else
		return _FALSE;
}

void rtw_list_insert_head(_list *plist, _list *phead)
{
	list_add(plist, phead);
}

void rtw_list_insert_tail(_list *plist, _list *phead)
{
	list_add_tail(plist, phead);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
void rtw_init_timer(_timer *ptimer, void *padapter, void *pfunc)
{
	_adapter *adapter = (_adapter *)padapter;

	_init_timer(ptimer, adapter->pnetdev, pfunc, adapter);
}
#endif

/*

Caller must check if the list is empty before calling rtw_list_delete

*/


void _rtw_init_sema(_sema	*sema, int init_val)
{
	sema_init(sema, init_val);
}

void _rtw_free_sema(_sema	*sema)
{
}

void _rtw_up_sema(_sema	*sema)
{
	up(sema);
}

u32 _rtw_down_sema(_sema *sema)
{
	if (down_interruptible(sema))
		return _FAIL;
	else
		return _SUCCESS;
}



void	_rtw_mutex_init(_mutex *pmutex)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	mutex_init(pmutex);
#else
	init_MUTEX(pmutex);
#endif
}

void	_rtw_mutex_free(_mutex *pmutex);
void	_rtw_mutex_free(_mutex *pmutex)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	mutex_destroy(pmutex);
#endif
}

void	_rtw_spinlock_init(_lock *plock)
{
	spin_lock_init(plock);
}

void	_rtw_spinlock_free(_lock *plock)
{
}

void	_rtw_spinlock(_lock	*plock)
{
	spin_lock(plock);
}

void	_rtw_spinunlock(_lock *plock)
{
	spin_unlock(plock);
}

void	_rtw_spinlock_ex(_lock	*plock)
{
	spin_lock(plock);
}

void	_rtw_spinunlock_ex(_lock *plock)
{
	spin_unlock(plock);
}

void _rtw_init_queue(_queue *pqueue)
{
	_rtw_init_listhead(&(pqueue->queue));
	_rtw_spinlock_init(&(pqueue->lock));
}

void _rtw_deinit_queue(_queue *pqueue)
{
	_rtw_spinlock_free(&(pqueue->lock));
}

u32	  _rtw_queue_empty(_queue	*pqueue)
{
	return rtw_is_list_empty(&(pqueue->queue));
}


u32 rtw_end_of_queue_search(_list *head, _list *plist)
{
	if (head == plist)
		return _TRUE;
	else
		return _FALSE;
}


u32	rtw_get_current_time(void)
{
	return jiffies;
}

inline u32 rtw_systime_to_ms(u32 systime)
{
	return systime * 1000 / HZ;
}

inline u32 rtw_ms_to_systime(u32 ms)
{
	return ms * HZ / 1000;
}

/* the input parameter start use the same unit as returned by rtw_get_current_time */
inline s32 rtw_get_passing_time_ms(u32 start)
{
	return rtw_systime_to_ms(jiffies - start);
}

inline s32 rtw_get_time_interval_ms(u32 start, u32 end)
{
	return rtw_systime_to_ms(end - start);
}

void rtw_sleep_schedulable(int ms)
{
	u32 delta;

	delta = (ms * HZ) / 1000; /* (ms) */
	if (delta == 0) {
		delta = 1;/* 1 ms */
	}
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(delta);
}


void rtw_msleep_os(int ms)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36))
	if (ms < 20) {
		unsigned long us = ms * 1000UL;
		usleep_range(us, us + 1000UL);
	} else
#endif
		msleep((unsigned int)ms);
}

void rtw_usleep_os(int us)
{
	/* msleep((unsigned int)us); */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36))
	usleep_range(us, us + 1);
#else
	if (1 < (us / 1000))
		msleep(1);
	else
		msleep((us / 1000) + 1);
#endif
}

#ifdef DBG_DELAY_OS
void _rtw_mdelay_os(int ms, const char *func, const int line)
{
	RTW_INFO("%s:%d %s(%d)\n", func, line, __FUNCTION__, ms);
	mdelay((unsigned long)ms);
}

void _rtw_udelay_os(int us, const char *func, const int line)
{
	RTW_INFO("%s:%d %s(%d)\n", func, line, __FUNCTION__, us);
	udelay((unsigned long)us);
}
#else
void rtw_mdelay_os(int ms)
{
	mdelay((unsigned long)ms);
}

void rtw_udelay_os(int us)
{
	udelay((unsigned long)us);
}
#endif

void rtw_yield_os(void)
{
	yield();
}

#define RTW_SUSPEND_LOCK_NAME "rtw_wifi"
#define RTW_SUSPEND_EXT_LOCK_NAME "rtw_wifi_ext"
#define RTW_SUSPEND_RX_LOCK_NAME "rtw_wifi_rx"
#define RTW_SUSPEND_TRAFFIC_LOCK_NAME "rtw_wifi_traffic"
#define RTW_SUSPEND_RESUME_LOCK_NAME "rtw_wifi_resume"
#define RTW_RESUME_SCAN_LOCK_NAME "rtw_wifi_scan"
#ifdef CONFIG_WAKELOCK
static struct wake_lock rtw_suspend_lock;
static struct wake_lock rtw_suspend_ext_lock;
static struct wake_lock rtw_suspend_rx_lock;
static struct wake_lock rtw_suspend_traffic_lock;
static struct wake_lock rtw_suspend_resume_lock;
static struct wake_lock rtw_resume_scan_lock;
#elif defined(CONFIG_ANDROID_POWER)
static android_suspend_lock_t rtw_suspend_lock = {
	.name = RTW_SUSPEND_LOCK_NAME
};
static android_suspend_lock_t rtw_suspend_ext_lock = {
	.name = RTW_SUSPEND_EXT_LOCK_NAME
};
static android_suspend_lock_t rtw_suspend_rx_lock = {
	.name = RTW_SUSPEND_RX_LOCK_NAME
};
static android_suspend_lock_t rtw_suspend_traffic_lock = {
	.name = RTW_SUSPEND_TRAFFIC_LOCK_NAME
};
static android_suspend_lock_t rtw_suspend_resume_lock = {
	.name = RTW_SUSPEND_RESUME_LOCK_NAME
};
static android_suspend_lock_t rtw_resume_scan_lock = {
	.name = RTW_RESUME_SCAN_LOCK_NAME
};
#endif

inline void rtw_suspend_lock_init(void)
{
#ifdef CONFIG_WAKELOCK
	wake_lock_init(&rtw_suspend_lock, WAKE_LOCK_SUSPEND, RTW_SUSPEND_LOCK_NAME);
	wake_lock_init(&rtw_suspend_ext_lock, WAKE_LOCK_SUSPEND, RTW_SUSPEND_EXT_LOCK_NAME);
	wake_lock_init(&rtw_suspend_rx_lock, WAKE_LOCK_SUSPEND, RTW_SUSPEND_RX_LOCK_NAME);
	wake_lock_init(&rtw_suspend_traffic_lock, WAKE_LOCK_SUSPEND, RTW_SUSPEND_TRAFFIC_LOCK_NAME);
	wake_lock_init(&rtw_suspend_resume_lock, WAKE_LOCK_SUSPEND, RTW_SUSPEND_RESUME_LOCK_NAME);
	wake_lock_init(&rtw_resume_scan_lock, WAKE_LOCK_SUSPEND, RTW_RESUME_SCAN_LOCK_NAME);
#elif defined(CONFIG_ANDROID_POWER)
	android_init_suspend_lock(&rtw_suspend_lock);
	android_init_suspend_lock(&rtw_suspend_ext_lock);
	android_init_suspend_lock(&rtw_suspend_rx_lock);
	android_init_suspend_lock(&rtw_suspend_traffic_lock);
	android_init_suspend_lock(&rtw_suspend_resume_lock);
	android_init_suspend_lock(&rtw_resume_scan_lock);
#endif
}

inline void rtw_suspend_lock_uninit(void)
{
#ifdef CONFIG_WAKELOCK
	wake_lock_destroy(&rtw_suspend_lock);
	wake_lock_destroy(&rtw_suspend_ext_lock);
	wake_lock_destroy(&rtw_suspend_rx_lock);
	wake_lock_destroy(&rtw_suspend_traffic_lock);
	wake_lock_destroy(&rtw_suspend_resume_lock);
	wake_lock_destroy(&rtw_resume_scan_lock);
#elif defined(CONFIG_ANDROID_POWER)
	android_uninit_suspend_lock(&rtw_suspend_lock);
	android_uninit_suspend_lock(&rtw_suspend_ext_lock);
	android_uninit_suspend_lock(&rtw_suspend_rx_lock);
	android_uninit_suspend_lock(&rtw_suspend_traffic_lock);
	android_uninit_suspend_lock(&rtw_suspend_resume_lock);
	android_uninit_suspend_lock(&rtw_resume_scan_lock);
#endif
}

inline void rtw_lock_suspend(void)
{
#ifdef CONFIG_WAKELOCK
	wake_lock(&rtw_suspend_lock);
#elif defined(CONFIG_ANDROID_POWER)
	android_lock_suspend(&rtw_suspend_lock);
#endif

#if  defined(CONFIG_WAKELOCK) || defined(CONFIG_ANDROID_POWER)
	/* RTW_INFO("####%s: suspend_lock_count:%d####\n", __FUNCTION__, rtw_suspend_lock.stat.count); */
#endif
}

inline void rtw_unlock_suspend(void)
{
#ifdef CONFIG_WAKELOCK
	wake_unlock(&rtw_suspend_lock);
#elif defined(CONFIG_ANDROID_POWER)
	android_unlock_suspend(&rtw_suspend_lock);
#endif

#if  defined(CONFIG_WAKELOCK) || defined(CONFIG_ANDROID_POWER)
	/* RTW_INFO("####%s: suspend_lock_count:%d####\n", __FUNCTION__, rtw_suspend_lock.stat.count); */
#endif
}

inline void rtw_resume_lock_suspend(void)
{
#ifdef CONFIG_WAKELOCK
	wake_lock(&rtw_suspend_resume_lock);
#elif defined(CONFIG_ANDROID_POWER)
	android_lock_suspend(&rtw_suspend_resume_lock);
#endif

#if  defined(CONFIG_WAKELOCK) || defined(CONFIG_ANDROID_POWER)
	/* RTW_INFO("####%s: suspend_lock_count:%d####\n", __FUNCTION__, rtw_suspend_lock.stat.count); */
#endif
}

inline void rtw_resume_unlock_suspend(void)
{
#ifdef CONFIG_WAKELOCK
	wake_unlock(&rtw_suspend_resume_lock);
#elif defined(CONFIG_ANDROID_POWER)
	android_unlock_suspend(&rtw_suspend_resume_lock);
#endif

#if  defined(CONFIG_WAKELOCK) || defined(CONFIG_ANDROID_POWER)
	/* RTW_INFO("####%s: suspend_lock_count:%d####\n", __FUNCTION__, rtw_suspend_lock.stat.count); */
#endif
}

inline void rtw_lock_suspend_timeout(u32 timeout_ms)
{
#ifdef CONFIG_WAKELOCK
	wake_lock_timeout(&rtw_suspend_lock, rtw_ms_to_systime(timeout_ms));
#elif defined(CONFIG_ANDROID_POWER)
	android_lock_suspend_auto_expire(&rtw_suspend_lock, rtw_ms_to_systime(timeout_ms));
#endif
}

inline void rtw_lock_ext_suspend_timeout(u32 timeout_ms)
{
#ifdef CONFIG_WAKELOCK
	wake_lock_timeout(&rtw_suspend_ext_lock, rtw_ms_to_systime(timeout_ms));
#elif defined(CONFIG_ANDROID_POWER)
	android_lock_suspend_auto_expire(&rtw_suspend_ext_lock, rtw_ms_to_systime(timeout_ms));
#endif
	/* RTW_INFO("EXT lock timeout:%d\n", timeout_ms); */
}

inline void rtw_lock_rx_suspend_timeout(u32 timeout_ms)
{
#ifdef CONFIG_WAKELOCK
	wake_lock_timeout(&rtw_suspend_rx_lock, rtw_ms_to_systime(timeout_ms));
#elif defined(CONFIG_ANDROID_POWER)
	android_lock_suspend_auto_expire(&rtw_suspend_rx_lock, rtw_ms_to_systime(timeout_ms));
#endif
	/* RTW_INFO("RX lock timeout:%d\n", timeout_ms); */
}


inline void rtw_lock_traffic_suspend_timeout(u32 timeout_ms)
{
#ifdef CONFIG_WAKELOCK
	wake_lock_timeout(&rtw_suspend_traffic_lock, rtw_ms_to_systime(timeout_ms));
#elif defined(CONFIG_ANDROID_POWER)
	android_lock_suspend_auto_expire(&rtw_suspend_traffic_lock, rtw_ms_to_systime(timeout_ms));
#endif
	/* RTW_INFO("traffic lock timeout:%d\n", timeout_ms); */
}

inline void rtw_lock_resume_scan_timeout(u32 timeout_ms)
{
#ifdef CONFIG_WAKELOCK
	wake_lock_timeout(&rtw_resume_scan_lock, rtw_ms_to_systime(timeout_ms));
#elif defined(CONFIG_ANDROID_POWER)
	android_lock_suspend_auto_expire(&rtw_resume_scan_lock, rtw_ms_to_systime(timeout_ms));
#endif
	/* RTW_INFO("resume scan lock:%d\n", timeout_ms); */
}

inline void ATOMIC_SET(ATOMIC_T *v, int i)
{
	atomic_set(v, i);
}

inline int ATOMIC_READ(ATOMIC_T *v)
{
	return atomic_read(v);
}

inline void ATOMIC_ADD(ATOMIC_T *v, int i)
{
	atomic_add(i, v);
}
inline void ATOMIC_SUB(ATOMIC_T *v, int i)
{
	atomic_sub(i, v);
}

inline void ATOMIC_INC(ATOMIC_T *v)
{
	atomic_inc(v);
}

inline void ATOMIC_DEC(ATOMIC_T *v)
{
	atomic_dec(v);
}

inline int ATOMIC_ADD_RETURN(ATOMIC_T *v, int i)
{
	return atomic_add_return(i, v);
}

inline int ATOMIC_SUB_RETURN(ATOMIC_T *v, int i)
{
	return atomic_sub_return(i, v);
}

inline int ATOMIC_INC_RETURN(ATOMIC_T *v)
{
	return atomic_inc_return(v);
}

inline int ATOMIC_DEC_RETURN(ATOMIC_T *v)
{
	return atomic_dec_return(v);
}


/*
* Open a file with the specific @param path, @param flag, @param mode
* @param fpp the pointer of struct file pointer to get struct file pointer while file opening is success
* @param path the path of the file to open
* @param flag file operation flags, please refer to linux document
* @param mode please refer to linux document
* @return Linux specific error code
*/
static int openFile(struct file **fpp, const char *path, int flag, int mode)
{
	struct file *fp;

	fp = filp_open(path, flag, mode);
	if (IS_ERR(fp)) {
		*fpp = NULL;
		return PTR_ERR(fp);
	} else {
		*fpp = fp;
		return 0;
	}
}

/*
* Close the file with the specific @param fp
* @param fp the pointer of struct file to close
* @return always 0
*/
static int closeFile(struct file *fp)
{
	filp_close(fp, NULL);
	return 0;
}

static int readFile(struct file *fp, char *buf, int len)
{
	int rlen = 0, sum = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	if (!(fp->f_mode & FMODE_CAN_READ))
#else
	if (!fp->f_op || !fp->f_op->read)
#endif
		return -EPERM;

	while (sum < len) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
		rlen = kernel_read(fp, buf + sum, len - sum, &fp->f_pos);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
		rlen = __vfs_read(fp, buf + sum, len - sum, &fp->f_pos);
#else
		rlen = fp->f_op->read(fp, buf + sum, len - sum, &fp->f_pos);
#endif
		if (rlen > 0)
			sum += rlen;
		else if (0 != rlen)
			return rlen;
		else
			break;
	}

	return  sum;

}

static int writeFile(struct file *fp, char *buf, int len)
{
	int wlen = 0, sum = 0;

	if (!fp->f_op || !fp->f_op->write)
		return -EPERM;

	while (sum < len) {
		wlen = fp->f_op->write(fp, buf + sum, len - sum, &fp->f_pos);
		if (wlen > 0)
			sum += wlen;
		else if (0 != wlen)
			return wlen;
		else
			break;
	}

	return sum;

}

/*
* Test if the specifi @param path is a file and readable
* If readable, @param sz is got
* @param path the path of the file to test
* @return Linux specific error code
*/
static int isFileReadable(const char *path, u32 *sz)
{
	struct file *fp;
	int ret = 0;
#ifdef set_fs
	mm_segment_t oldfs;
#endif
	char buf;

	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(fp))
		ret = PTR_ERR(fp);
	else {
#ifdef set_fs
		oldfs = get_fs();
		set_fs(KERNEL_DS);
#endif

		if (1 != readFile(fp, &buf, 1))
			ret = PTR_ERR(fp);

		if (ret == 0 && sz) {
			#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0))
			*sz = i_size_read(fp->f_path.dentry->d_inode);
			#else
			*sz = i_size_read(fp->f_dentry->d_inode);
			#endif
		}

#ifdef set_fs
		set_fs(oldfs);
#endif
		filp_close(fp, NULL);
	}
	return ret;
}

/*
* Open the file with @param path and retrive the file content into memory starting from @param buf for @param sz at most
* @param path the path of the file to open and read
* @param buf the starting address of the buffer to store file content
* @param sz how many bytes to read at most
* @return the byte we've read, or Linux specific error code
*/
static int retriveFromFile(const char *path, u8 *buf, u32 sz)
{
	int ret = -1;
#ifdef set_fs
	mm_segment_t oldfs;
#endif
	struct file *fp;

	if (path && buf) {
		ret = openFile(&fp, path, O_RDONLY, 0);
		if (0 == ret) {
			RTW_INFO("%s openFile path:%s fp=%p\n", __FUNCTION__, path , fp);

#ifdef set_fs
			oldfs = get_fs();
			set_fs(KERNEL_DS);
#endif
			ret = readFile(fp, buf, sz);
#ifdef set_fs
			set_fs(oldfs);
#endif
			closeFile(fp);

			RTW_INFO("%s readFile, ret:%d\n", __FUNCTION__, ret);

		} else
			RTW_INFO("%s openFile path:%s Fail, ret:%d\n", __FUNCTION__, path, ret);
	} else {
		RTW_INFO("%s NULL pointer\n", __FUNCTION__);
		ret =  -EINVAL;
	}
	return ret;
}

/*
* Open the file with @param path and wirte @param sz byte of data starting from @param buf into the file
* @param path the path of the file to open and write
* @param buf the starting address of the data to write into file
* @param sz how many bytes to write at most
* @return the byte we've written, or Linux specific error code
*/
static int storeToFile(const char *path, u8 *buf, u32 sz)
{
	int ret = 0;
#ifdef set_fs
	mm_segment_t oldfs;
#endif
	struct file *fp;

	if (path && buf) {
		ret = openFile(&fp, path, O_CREAT | O_WRONLY, 0666);
		if (0 == ret) {
			RTW_INFO("%s openFile path:%s fp=%p\n", __FUNCTION__, path , fp);

#ifdef set_fs
			oldfs = get_fs();
			set_fs(KERNEL_DS);
#endif
			ret = writeFile(fp, buf, sz);
#ifdef set_fs
			set_fs(oldfs);
#endif
			closeFile(fp);

			RTW_INFO("%s writeFile, ret:%d\n", __FUNCTION__, ret);

		} else
			RTW_INFO("%s openFile path:%s Fail, ret:%d\n", __FUNCTION__, path, ret);
	} else {
		RTW_INFO("%s NULL pointer\n", __FUNCTION__);
		ret =  -EINVAL;
	}
	return ret;
}

/*
* Test if the specifi @param path is a file and readable
* @param path the path of the file to test
* @return _TRUE or _FALSE
*/
int rtw_is_file_readable(const char *path)
{
	if (isFileReadable(path, NULL) == 0)
		return _TRUE;
	else
		return _FALSE;
}

/*
* Test if the specifi @param path is a file and readable.
* If readable, @param sz is got
* @param path the path of the file to test
* @return _TRUE or _FALSE
*/
int rtw_is_file_readable_with_size(const char *path, u32 *sz)
{
	if (isFileReadable(path, sz) == 0)
		return _TRUE;
	else
		return _FALSE;
}

/*
* Open the file with @param path and retrive the file content into memory starting from @param buf for @param sz at most
* @param path the path of the file to open and read
* @param buf the starting address of the buffer to store file content
* @param sz how many bytes to read at most
* @return the byte we've read
*/
int rtw_retrieve_from_file(const char *path, u8 *buf, u32 sz)
{
	int ret = retriveFromFile(path, buf, sz);
	return ret >= 0 ? ret : 0;
}

/*
* Open the file with @param path and wirte @param sz byte of data starting from @param buf into the file
* @param path the path of the file to open and write
* @param buf the starting address of the data to write into file
* @param sz how many bytes to write at most
* @return the byte we've written
*/
int rtw_store_to_file(const char *path, u8 *buf, u32 sz)
{
	int ret = storeToFile(path, buf, sz);
	return ret >= 0 ? ret : 0;
}

struct net_device *rtw_alloc_etherdev_with_old_priv(int sizeof_priv, void *old_priv)
{
	struct net_device *pnetdev;
	struct rtw_netdev_priv_indicator *pnpi;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	pnetdev = alloc_etherdev_mq(sizeof(struct rtw_netdev_priv_indicator), 4);
#else
	pnetdev = alloc_etherdev(sizeof(struct rtw_netdev_priv_indicator));
#endif
	if (!pnetdev)
		goto RETURN;

	pnpi = netdev_priv(pnetdev);
	pnpi->priv = old_priv;
	pnpi->sizeof_priv = sizeof_priv;

RETURN:
	return pnetdev;
}

struct net_device *rtw_alloc_etherdev(int sizeof_priv)
{
	struct net_device *pnetdev;
	struct rtw_netdev_priv_indicator *pnpi;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	pnetdev = alloc_etherdev_mq(sizeof(struct rtw_netdev_priv_indicator), 4);
#else
	pnetdev = alloc_etherdev(sizeof(struct rtw_netdev_priv_indicator));
#endif
	if (!pnetdev)
		goto RETURN;

	pnpi = netdev_priv(pnetdev);

	pnpi->priv = rtw_zvmalloc(sizeof_priv);
	if (!pnpi->priv) {
		free_netdev(pnetdev);
		pnetdev = NULL;
		goto RETURN;
	}

	pnpi->sizeof_priv = sizeof_priv;
RETURN:
	return pnetdev;
}

void rtw_free_netdev(struct net_device *netdev)
{
	struct rtw_netdev_priv_indicator *pnpi;

	if (!netdev)
		goto RETURN;

	pnpi = netdev_priv(netdev);

	if (!pnpi->priv)
		goto RETURN;

	free_netdev(netdev);

RETURN:
	return;
}

int rtw_change_ifname(_adapter *padapter, const char *ifname)
{
	struct net_device *pnetdev;
	struct net_device *cur_pnetdev;
	struct rereg_nd_name_data *rereg_priv;
	int ret;

	if (!padapter)
		goto error;

	cur_pnetdev = padapter->pnetdev;
	rereg_priv = &padapter->rereg_nd_name_priv;

	/* free the old_pnetdev */
	if (rereg_priv->old_pnetdev) {
		free_netdev(rereg_priv->old_pnetdev);
		rereg_priv->old_pnetdev = NULL;
	}

	unregister_netdevice(cur_pnetdev);

	rereg_priv->old_pnetdev = cur_pnetdev;

	pnetdev = rtw_init_netdev(padapter);
	if (!pnetdev)  {
		ret = -1;
		goto error;
	}

	SET_NETDEV_DEV(pnetdev, dvobj_to_dev(adapter_to_dvobj(padapter)));

	rtw_init_netdev_name(pnetdev, ifname);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	_rtw_memcpy(pnetdev->dev_addr, adapter_mac_addr(padapter), ETH_ALEN);
#else
	dev_addr_set(pnetdev, adapter_mac_addr(padapter));
#endif
	pr_info("****** vendor driver: MAC address %pM\n", adapter_mac_addr(padapter));
	ret = register_netdevice(pnetdev);

	if (ret != 0) {
		goto error;
	}

	return 0;

error:

	return -1;

}

#ifdef CONFIG_PLATFORM_SPRD
	#ifdef do_div
		#undef do_div
	#endif
	#include <asm-generic/div64.h>
#endif

u64 rtw_modular64(u64 x, u64 y)
{
	return do_div(x, y);
}

u64 rtw_division64(u64 x, u64 y)
{
	do_div(x, y);
	return x;
}

inline u32 rtw_random32(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	return get_random_u32();
#else
	return prandom_u32();
#endif
#elif (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 18))
	u32 random_int;
	get_random_bytes(&random_int , 4);
	return random_int;
#else
	return random32();
#endif
}

void rtw_buf_free(u8 **buf, u32 *buf_len)
{
	u32 ori_len;

	if (!buf || !buf_len)
		return;

	ori_len = *buf_len;

	if (*buf) {
		u32 tmp_buf_len = *buf_len;
		*buf_len = 0;
		rtw_mfree(*buf, tmp_buf_len);
		*buf = NULL;
	}
}

void rtw_buf_update(u8 **buf, u32 *buf_len, u8 *src, u32 src_len)
{
	u32 ori_len = 0, dup_len = 0;
	u8 *ori = NULL;
	u8 *dup = NULL;

	if (!buf || !buf_len)
		return;

	if (!src || !src_len)
		goto keep_ori;

	/* duplicate src */
	dup = rtw_malloc(src_len);
	if (dup) {
		dup_len = src_len;
		_rtw_memcpy(dup, src, dup_len);
	}

keep_ori:
	ori = *buf;
	ori_len = *buf_len;

	/* replace buf with dup */
	*buf_len = 0;
	*buf = dup;
	*buf_len = dup_len;

	/* free ori */
	if (ori && ori_len > 0)
		rtw_mfree(ori, ori_len);
}


/**
 * rtw_cbuf_full - test if cbuf is full
 * @cbuf: pointer of struct rtw_cbuf
 *
 * Returns: _TRUE if cbuf is full
 */
inline bool rtw_cbuf_full(struct rtw_cbuf *cbuf)
{
	return (cbuf->write == cbuf->read - 1) ? _TRUE : _FALSE;
}

/**
 * rtw_cbuf_empty - test if cbuf is empty
 * @cbuf: pointer of struct rtw_cbuf
 *
 * Returns: _TRUE if cbuf is empty
 */
inline bool rtw_cbuf_empty(struct rtw_cbuf *cbuf)
{
	return (cbuf->write == cbuf->read) ? _TRUE : _FALSE;
}

/**
 * rtw_cbuf_push - push a pointer into cbuf
 * @cbuf: pointer of struct rtw_cbuf
 * @buf: pointer to push in
 *
 * Lock free operation, be careful of the use scheme
 * Returns: _TRUE push success
 */
bool rtw_cbuf_push(struct rtw_cbuf *cbuf, void *buf)
{
	if (rtw_cbuf_full(cbuf))
		return _FAIL;

	if (0)
		RTW_INFO("%s on %u\n", __func__, cbuf->write);
	cbuf->bufs[cbuf->write] = buf;
	cbuf->write = (cbuf->write + 1) % cbuf->size;

	return _SUCCESS;
}

/**
 * rtw_cbuf_pop - pop a pointer from cbuf
 * @cbuf: pointer of struct rtw_cbuf
 *
 * Lock free operation, be careful of the use scheme
 * Returns: pointer popped out
 */
void *rtw_cbuf_pop(struct rtw_cbuf *cbuf)
{
	void *buf;
	if (rtw_cbuf_empty(cbuf))
		return NULL;

	if (0)
		RTW_INFO("%s on %u\n", __func__, cbuf->read);
	buf = cbuf->bufs[cbuf->read];
	cbuf->read = (cbuf->read + 1) % cbuf->size;

	return buf;
}

/**
 * rtw_cbuf_alloc - allocte a rtw_cbuf with given size and do initialization
 * @size: size of pointer
 *
 * Returns: pointer of srtuct rtw_cbuf, NULL for allocation failure
 */
struct rtw_cbuf *rtw_cbuf_alloc(u32 size)
{
	struct rtw_cbuf *cbuf;

	cbuf = (struct rtw_cbuf *)rtw_malloc(sizeof(*cbuf) + sizeof(void *) * size);

	if (cbuf) {
		cbuf->write = cbuf->read = 0;
		cbuf->size = size;
	}

	return cbuf;
}

/**
 * rtw_cbuf_free - free the given rtw_cbuf
 * @cbuf: pointer of struct rtw_cbuf to free
 */
void rtw_cbuf_free(struct rtw_cbuf *cbuf)
{
	rtw_mfree((u8 *)cbuf, sizeof(*cbuf) + sizeof(void *) * cbuf->size);
}

/**
 * map_readN - read a range of map data
 * @map: map to read
 * @offset: start address to read
 * @len: length to read
 * @buf: pointer of buffer to store data read
 *
 * Returns: _SUCCESS or _FAIL
 */
int map_readN(const struct map_t *map, u16 offset, u16 len, u8 *buf)
{
	const struct map_seg_t *seg;
	int ret = _FAIL;
	int i;

	if (len == 0) {
		rtw_warn_on(1);
		goto exit;
	}

	if (offset + len > map->len) {
		rtw_warn_on(1);
		goto exit;
	}

	_rtw_memset(buf, map->init_value, len);

	for (i = 0; i < map->seg_num; i++) {
		u8 *c_dst, *c_src;
		u16 c_len;

		seg = map->segs + i;
		if (seg->sa + seg->len <= offset || seg->sa >= offset + len)
			continue;

		if (seg->sa >= offset) {
			c_dst = buf + (seg->sa - offset);
			c_src = seg->c;
			if (seg->sa + seg->len <= offset + len)
				c_len = seg->len;
			else
				c_len = offset + len - seg->sa;
		} else {
			c_dst = buf;
			c_src = seg->c + (offset - seg->sa);
			if (seg->sa + seg->len >= offset + len)
				c_len = len;
			else
				c_len = seg->sa + seg->len - offset;
		}

		_rtw_memcpy(c_dst, c_src, c_len);
	}

exit:
	return ret;
}

/**
 * map_read8 - read 1 byte of map data
 * @map: map to read
 * @offset: address to read
 *
 * Returns: value of data of specified offset. map.init_value if offset is out of range
 */
u8 map_read8(const struct map_t *map, u16 offset)
{
	const struct map_seg_t *seg;
	u8 val = map->init_value;
	int i;

	if (offset + 1 > map->len) {
		rtw_warn_on(1);
		goto exit;
	}

	for (i = 0; i < map->seg_num; i++) {
		seg = map->segs + i;
		if (seg->sa + seg->len <= offset || seg->sa >= offset + 1)
			continue;

		val = *(seg->c + offset - seg->sa);
		break;
	}

exit:
	return val;
}

/**
* is_null -
*
* Return	TRUE if c is null character
*		FALSE otherwise.
*/
inline BOOLEAN is_null(char c)
{
	if (c == '\0')
		return _TRUE;
	else
		return _FALSE;
}

/**
* is_eol -
*
* Return	TRUE if c is represent for EOL (end of line)
*		FALSE otherwise.
*/
inline BOOLEAN is_eol(char c)
{
	if (c == '\r' || c == '\n')
		return _TRUE;
	else
		return _FALSE;
}

/**
* is_space -
*
* Return	TRUE if c is represent for space
*		FALSE otherwise.
*/
inline BOOLEAN is_space(char c)
{
	if (c == ' ' || c == '\t')
		return _TRUE;
	else
		return _FALSE;
}

/**
* IsHexDigit -
*
* Return	TRUE if chTmp is represent for hex digit
*		FALSE otherwise.
*/
inline BOOLEAN IsHexDigit(char chTmp)
{
	if ((chTmp >= '0' && chTmp <= '9') ||
		(chTmp >= 'a' && chTmp <= 'f') ||
		(chTmp >= 'A' && chTmp <= 'F'))
		return _TRUE;
	else
		return _FALSE;
}

/**
* is_alpha -
*
* Return	TRUE if chTmp is represent for alphabet
*		FALSE otherwise.
*/
inline BOOLEAN is_alpha(char chTmp)
{
	if ((chTmp >= 'a' && chTmp <= 'z') ||
		(chTmp >= 'A' && chTmp <= 'Z'))
		return _TRUE;
	else
		return _FALSE;
}

inline char alpha_to_upper(char c)
{
	if ((c >= 'a' && c <= 'z'))
		c = 'A' + (c - 'a');
	return c;
}
