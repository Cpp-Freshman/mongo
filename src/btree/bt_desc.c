/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2011 WiredTiger, Inc.
 *	All rights reserved.
 */

#include "wt_internal.h"

static int __wt_desc_io(SESSION *, void *, int);

/*
 * __wt_desc_stat --
 *	Fill in the statistics from the file's description.
 */
int
__wt_desc_stat(SESSION *session)
{
	WT_PAGE_DESC desc;

	WT_RET(__wt_desc_io(session, &desc, 1));

	WT_STAT_SET(session->btree->fstats, file_magic, desc.magic);
	WT_STAT_SET(session->btree->fstats, file_major, desc.majorv);
	WT_STAT_SET(session->btree->fstats, file_minor, desc.minorv);
	WT_STAT_SET(session->btree->fstats, file_allocsize, desc.allocsize);
	WT_STAT_SET(session->btree->fstats, file_intlmax, desc.intlmax);
	WT_STAT_SET(session->btree->fstats, file_intlmin, desc.intlmin);
	WT_STAT_SET(session->btree->fstats, file_leafmax, desc.leafmax);
	WT_STAT_SET(session->btree->fstats, file_leafmin, desc.leafmin);
	WT_STAT_SET(session->btree->fstats, file_base_recno, desc.recno_offset);
	WT_STAT_SET(session->btree->fstats, file_fixed_len, desc.fixed_len);

	return (0);
}

/*
 * __wt_desc_read --
 *	Read the descriptor structure from page 0.
 */
int
__wt_desc_read(SESSION *session)
{
	BTREE *btree;
	WT_PAGE_DESC desc;

	btree = session->btree;

	WT_RET(__wt_desc_io(session, &desc, 1));

	btree->allocsize = desc.allocsize;		/* Update DB handle */
	btree->intlmax = desc.intlmax;
	btree->intlmin = desc.intlmin;
	btree->leafmax = desc.leafmax;
	btree->leafmin = desc.leafmin;
	btree->root_page.addr = desc.root_addr;
	btree->root_page.size = desc.root_size;
	btree->free_addr = desc.free_addr;
	btree->free_size = desc.free_size;
	btree->fixed_len = desc.fixed_len;

	/*
	 * XXX
	 * This is the wrong place to do this -- need to think about how
	 * to update open/configuration information in a reasonable way.
	 */
	if (F_ISSET(&desc, WT_PAGE_DESC_COLUMN))
		F_SET(btree, WT_COLUMN);
	if (F_ISSET(&desc, WT_PAGE_DESC_RLE))
		F_SET(btree, WT_RLE);

	return (0);
}

/*
 * __wt_desc_write --
 *	Update the description page.
 */
int
__wt_desc_write(SESSION *session)
{
	BTREE *btree;
	WT_PAGE_DESC desc;
	int ret;

	btree = session->btree;
	ret = 0;

	WT_CLEAR(desc);
	desc.magic = WT_BTREE_MAGIC;
	desc.majorv = WT_BTREE_MAJOR_VERSION;
	desc.minorv = WT_BTREE_MINOR_VERSION;
	desc.allocsize = btree->allocsize;
	desc.intlmax = btree->intlmax;
	desc.intlmin = btree->intlmin;
	desc.leafmax = btree->leafmax;
	desc.leafmin = btree->leafmin;
	desc.recno_offset = 0;
	desc.root_addr = btree->root_page.addr;
	desc.root_size = btree->root_page.size;
	desc.free_addr = btree->free_addr;
	desc.free_size = btree->free_size;
	desc.fixed_len = (uint8_t)btree->fixed_len;
	desc.flags = 0;
	if (F_ISSET(btree, WT_COLUMN))
		F_SET(&desc, WT_PAGE_DESC_COLUMN);
	if (F_ISSET(btree, WT_RLE))
		F_SET(&desc, WT_PAGE_DESC_RLE);

	WT_RET(__wt_desc_io(session, &desc, 0));

	return (ret);
}

/*
 * __wt_desc_io --
 *	Read/write the WT_DESC sector.
 */
static int
__wt_desc_io(SESSION *session, void *p, int is_read)
{
	WT_FH *fh;

	fh = session->btree->fh;

	return (is_read ?
	    __wt_read(session, fh, (off_t)0, 512, p) :
	    __wt_write(session, fh, (off_t)0, 512, p));
}
