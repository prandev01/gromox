#include "msgchg_grouping.h"
#include "system_services.h"
#include "zarafa_server.h"
#include "store_object.h"
#include "exmdb_client.h"
#include "common_util.h"
#include "object_tree.h"
#include "config_file.h"
#include "mail_func.h"
#include "rop_util.h"
#include "util.h"
#include "guid.h"
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define HGROWING_SIZE									0x500

#define PROP_TAG_USERNAME								0x661A001F

#define PROP_TAG_ECSERVERVERSION						0x6716001F

#define PROP_TAG_OOFSTATE								0x67600003
#define PROP_TAG_OOFINTERNALREPLY						0x6761001F
#define PROP_TAG_OOFINTERNALSUBJECT						0x6762001F
#define PROP_TAG_OOFBEGIN								0x67630040
#define PROP_TAG_OOFUNTIL								0x67640040
#define PROP_TAG_OOFALLOWEXTERNAL						0x6765000B
#define PROP_TAG_OOFEXTERNALAUDIENCE					0x6766000B
#define PROP_TAG_OOFEXTERNALREPLY						0x6767001F
#define PROP_TAG_OOFEXTERNALSUBJECT						0x6768001F

#define OOF_STATE_DISABLED								0x00000000
#define OOF_STATE_ENABLED								0x00000001
#define OOF_STATE_SCHEDULED								0x00000002

static BOOL store_object_enlarge_propid_hash(STORE_OBJECT *pstore)
{
	int tmp_id;
	void *ptmp_value;
	INT_HASH_ITER *iter;
	INT_HASH_TABLE *phash;
	
	phash = int_hash_init(pstore->ppropid_hash->capacity
			+ HGROWING_SIZE, sizeof(PROPERTY_NAME), NULL);
	if (NULL == phash) {
		return FALSE;
	}
	iter = int_hash_iter_init(pstore->ppropid_hash);
	for (int_hash_iter_begin(iter); !int_hash_iter_done(iter);
		int_hash_iter_forward(iter)) {
		ptmp_value = int_hash_iter_get_value(iter, &tmp_id);
		int_hash_add(phash, tmp_id, ptmp_value);
	}
	int_hash_iter_free(iter);
	int_hash_free(pstore->ppropid_hash);
	pstore->ppropid_hash = phash;
	return TRUE;
}

static BOOL store_object_enlarge_propname_hash(STORE_OBJECT *pstore)
{
	void *ptmp_value;
	STR_HASH_ITER *iter;
	char tmp_string[256];
	STR_HASH_TABLE *phash;
	
	phash = str_hash_init(pstore->ppropname_hash->capacity
				+ HGROWING_SIZE, sizeof(uint16_t), NULL);
	if (NULL == phash) {
		return FALSE;
	}
	iter = str_hash_iter_init(pstore->ppropname_hash);
	for (str_hash_iter_begin(iter); !str_hash_iter_done(iter);
		str_hash_iter_forward(iter)) {
		ptmp_value = str_hash_iter_get_value(iter, tmp_string);
		str_hash_add(phash, tmp_string, ptmp_value);
	}
	str_hash_iter_free(iter);
	str_hash_free(pstore->ppropname_hash);
	pstore->ppropname_hash = phash;
	return TRUE;
}

static BOOL store_object_cache_propname(STORE_OBJECT *pstore,
	uint16_t propid, const PROPERTY_NAME *ppropname)
{
	char tmp_guid[64];
	char tmp_string[256];
	PROPERTY_NAME tmp_name;
	
	if (NULL == pstore->ppropid_hash) {
		pstore->ppropid_hash = int_hash_init(
			HGROWING_SIZE, sizeof(PROPERTY_NAME), NULL);
		if (NULL == pstore->ppropid_hash) {
			return FALSE;
		}
	}
	if (NULL == pstore->ppropname_hash) {
		pstore->ppropname_hash = str_hash_init(
			HGROWING_SIZE, sizeof(uint16_t), NULL);
		if (NULL == pstore->ppropname_hash) {
			int_hash_free(pstore->ppropid_hash);
			return FALSE;
		}
	}
	tmp_name.kind = ppropname->kind;
	tmp_name.guid = ppropname->guid;
	guid_to_string(&ppropname->guid, tmp_guid, 64);
	switch (ppropname->kind) {
	case KIND_LID:
		tmp_name.plid = malloc(sizeof(uint32_t));
		if (NULL == tmp_name.plid) {
			return FALSE;
		}
		*tmp_name.plid = *ppropname->plid;
		tmp_name.pname = NULL;
		snprintf(tmp_string, 256, "%s:lid:%u", tmp_guid, *ppropname->plid);
		break;
	case KIND_NAME:
		tmp_name.plid = NULL;
		tmp_name.pname = strdup(ppropname->pname);
		if (NULL == tmp_name.pname) {
			return FALSE;
		}
		snprintf(tmp_string, 256, "%s:name:%s", tmp_guid, ppropname->pname);
		break;
	default:
		return FALSE;
	}
	if (NULL == int_hash_query(pstore->ppropid_hash, propid)) {
		if (1 != int_hash_add(pstore->ppropid_hash, propid, &tmp_name)) {
			if (FALSE == store_object_enlarge_propid_hash(pstore) ||
				1 != int_hash_add(pstore->ppropid_hash, propid, &tmp_name)) {
				if (NULL != tmp_name.plid) {
					free(tmp_name.plid);
				}
				if (NULL != tmp_name.pname) {
					free(tmp_name.pname);
				}
				return FALSE;
			}
		}
	} else {
		if (NULL != tmp_name.plid) {
			free(tmp_name.plid);
		}
		if (NULL != tmp_name.pname) {
			free(tmp_name.pname);
		}
	}
	lower_string(tmp_string);
	if (NULL == str_hash_query(pstore->ppropname_hash, tmp_string)) {
		if (1 != str_hash_add(pstore->ppropname_hash, tmp_string, &propid)) {
			if (FALSE == store_object_enlarge_propname_hash(pstore)
				|| 1 != str_hash_add(pstore->ppropname_hash,
				tmp_string, &propid)) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

STORE_OBJECT* store_object_create(BOOL b_private,
	int account_id, const char *account, const char *dir)
{
	void *pvalue;
	uint32_t proptag;
	STORE_OBJECT *pstore;
	PROPTAG_ARRAY proptags;
	TPROPVAL_ARRAY propvals;
	
	proptags.count = 1;
	proptags.pproptag = &proptag;
	proptag = PROP_TAG_STORERECORDKEY;
	if (FALSE == exmdb_client_get_store_properties(
		dir, 0, &proptags, &propvals)) {
		return NULL;	
	}
	pvalue = common_util_get_propvals(
		&propvals, PROP_TAG_STORERECORDKEY);
	if (NULL == pvalue) {
		return NULL;
	}
	pstore = malloc(sizeof(STORE_OBJECT));
	if (NULL == pstore) {
		return NULL;
	}
	pstore->b_private = b_private;
	pstore->account_id = account_id;
	strncpy(pstore->account, account, 256);
	strncpy(pstore->dir, dir, 256);
	pstore->mailbox_guid = rop_util_binary_to_guid(pvalue);
	pstore->pgpinfo = NULL;
	pstore->ppropid_hash = NULL;
	pstore->ppropname_hash = NULL;
	double_list_init(&pstore->group_list);
	return pstore;
}

void store_object_free(STORE_OBJECT *pstore)
{
	INT_HASH_ITER *piter;
	DOUBLE_LIST_NODE *pnode;
	PROPERTY_NAME *ppropname;
	
	if (NULL != pstore->pgpinfo) {
		property_groupinfo_free(pstore->pgpinfo);
	}
	while (pnode = double_list_get_from_head(&pstore->group_list)) {
		property_groupinfo_free(pnode->pdata);
		free(pnode);
	}
	double_list_free(&pstore->group_list);
	if (NULL != pstore->ppropid_hash) {
		piter = int_hash_iter_init(pstore->ppropid_hash);
		for (int_hash_iter_begin(piter); !int_hash_iter_done(piter);
			int_hash_iter_forward(piter)) {
			ppropname = int_hash_iter_get_value(piter, NULL);
			switch( ppropname->kind) {
			case KIND_LID:
				free(ppropname->plid);
				break;
			case KIND_NAME:
				free(ppropname->pname);
				break;
			}
		}
		int_hash_iter_free(piter);
		int_hash_free(pstore->ppropid_hash);
	}
	if (NULL != pstore->ppropname_hash) {
		str_hash_free(pstore->ppropname_hash);
	}
	free(pstore);
}

BOOL store_object_check_private(STORE_OBJECT *pstore)
{
	return pstore->b_private;
}

store_object_check_owner_mode(STORE_OBJECT *pstore)
{
	USER_INFO *pinfo;
	
	if (FALSE == pstore->b_private) {
		return FALSE;
	}
	pinfo = zarafa_server_get_info();
	if (pinfo->user_id == pstore->account_id) {
		return TRUE;
	}
	return FALSE;
}

int store_object_get_account_id(STORE_OBJECT *pstore)
{
	return pstore->account_id;
}

const char* store_object_get_account(STORE_OBJECT *pstore)
{
	return pstore->account;
}

const char* store_object_get_dir(STORE_OBJECT *pstore)
{
	return pstore->dir;
}

GUID store_object_get_mailbox_guid(STORE_OBJECT *pstore)
{
	return pstore->mailbox_guid;
}

BOOL store_object_get_named_propname(STORE_OBJECT *pstore,
	uint16_t propid, PROPERTY_NAME *ppropname)
{
	PROPERTY_NAME *pname;
	
	if (propid < 0x8000) {
		rop_util_get_common_pset(PS_MAPI, &ppropname->guid);
		ppropname->kind = KIND_LID;
		ppropname->plid = common_util_alloc(sizeof(uint32_t));
		if (NULL == ppropname->plid) {
			return FALSE;
		}
		*ppropname->plid = propid;
	}
	if (NULL != pstore->ppropid_hash) {
		pname = int_hash_query(pstore->ppropid_hash, propid);
		if (NULL != pname) {
			*ppropname = *pname;
			return TRUE;
		}
	}
	if (FALSE == exmdb_client_get_named_propname(
		pstore->dir, propid, ppropname)) {
		return FALSE;	
	}
	if (KIND_LID == ppropname->kind ||
		KIND_NAME == ppropname->kind) {
		store_object_cache_propname(pstore, propid, ppropname);
	}
	return TRUE;
}

BOOL store_object_get_named_propnames(STORE_OBJECT *pstore,
	const PROPID_ARRAY *ppropids, PROPNAME_ARRAY *ppropnames)
{
	int i;
	int *pindex_map;
	PROPERTY_NAME *pname;
	PROPID_ARRAY tmp_propids;
	PROPNAME_ARRAY tmp_propnames;
	
	if (0 == ppropids->count) {
		ppropnames->count = 0;
		return TRUE;
	}
	pindex_map = common_util_alloc(ppropids->count*sizeof(int));
	if (NULL == pindex_map) {
		return FALSE;
	}
	ppropnames->ppropname = common_util_alloc(
		sizeof(PROPERTY_NAME)*ppropids->count);
	if (NULL == ppropnames->ppropname) {
		return FALSE;
	}
	ppropnames->count = ppropids->count;
	tmp_propids.count = 0;
	tmp_propids.ppropid = common_util_alloc(
			sizeof(uint16_t)*ppropids->count);
	if (NULL == tmp_propids.ppropid) {
		return FALSE;
	}
	for (i=0; i<ppropids->count; i++) {
		if (ppropids->ppropid[i] < 0x8000) {
			rop_util_get_common_pset(PS_MAPI,
				&ppropnames->ppropname[i].guid);
			ppropnames->ppropname[i].kind = KIND_LID;
			ppropnames->ppropname[i].plid =
				common_util_alloc(sizeof(uint32_t));
			if (NULL == ppropnames->ppropname[i].plid) {
				return FALSE;
			}
			*ppropnames->ppropname[i].plid = ppropids->ppropid[i];
			pindex_map[i] = i;
			continue;
		}
		if (NULL != pstore->ppropid_hash) {
			pname = int_hash_query(pstore->ppropid_hash,
								ppropids->ppropid[i]);
		} else {
			pname = NULL;
		}
		if (NULL != pname) {
			pindex_map[i] = i;
			ppropnames->ppropname[i] = *pname;
		} else {
			tmp_propids.ppropid[tmp_propids.count] =
								ppropids->ppropid[i];
			tmp_propids.count ++;
			pindex_map[i] = tmp_propids.count * (-1);
		}
	}
	if (0 == tmp_propids.count) {
		return TRUE;
	}
	if (FALSE == exmdb_client_get_named_propnames(
		pstore->dir, &tmp_propids, &tmp_propnames)) {
		return FALSE;	
	}
	for (i=0; i<ppropids->count; i++) {
		if (pindex_map[i] < 0) {
			ppropnames->ppropname[i] =
				tmp_propnames.ppropname[(-1)*pindex_map[i] - 1];
			if (KIND_LID == ppropnames->ppropname[i].kind ||
				KIND_NAME == ppropnames->ppropname[i].kind) {
				store_object_cache_propname(pstore,
					ppropids->ppropid[i], ppropnames->ppropname + i);
			}
		}
	}
	return TRUE;
}

BOOL store_object_get_named_propid(STORE_OBJECT *pstore,
	BOOL b_create, const PROPERTY_NAME *ppropname,
	uint16_t *ppropid)
{
	GUID guid;
	uint16_t *pid;
	char tmp_guid[64];
	char tmp_string[256];
	
	rop_util_get_common_pset(PS_MAPI, &guid);
	if (0 == guid_compare(&ppropname->guid, &guid)) {
		if (KIND_LID == ppropname->kind) {
			*ppropid = *ppropname->plid;
		} else {
			*ppropid = 0;
		}
		return TRUE;
	}
	guid_to_string(&ppropname->guid, tmp_guid, 64);
	switch (ppropname->kind) {
	case KIND_LID:
		snprintf(tmp_string, 256, "%s:lid:%u", tmp_guid, *ppropname->plid);
		break;
	case KIND_NAME:
		snprintf(tmp_string, 256, "%s:name:%s", tmp_guid, ppropname->pname);
		lower_string(tmp_string);
		break;
	default:
		*ppropid = 0;
		return TRUE;
	}
	if (NULL != pstore->ppropname_hash) {
		pid = str_hash_query(pstore->ppropname_hash, tmp_string);
		if (NULL != pid) {
			*ppropid = *pid;
			return TRUE;
		}
	}
	if (FALSE == exmdb_client_get_named_propid(
		pstore->dir, b_create, ppropname, ppropid)) {
		return FALSE;
	}
	if (0 == *ppropid) {
		return TRUE;
	}
	store_object_cache_propname(pstore, *ppropid, ppropname);
	return TRUE;
}

BOOL store_object_get_named_propids(STORE_OBJECT *pstore,
	BOOL b_create, const PROPNAME_ARRAY *ppropnames,
	PROPID_ARRAY *ppropids)
{
	int i;
	GUID guid;
	uint16_t *pid;
	int *pindex_map;
	char tmp_guid[64];
	char tmp_string[256];
	PROPID_ARRAY tmp_propids;
	PROPNAME_ARRAY tmp_propnames;
	
	if (0 == ppropnames->count) {
		ppropids->count = 0;
		return TRUE;
	}
	rop_util_get_common_pset(PS_MAPI, &guid);
	pindex_map = common_util_alloc(ppropnames->count*sizeof(int));
	if (NULL == pindex_map) {
		return FALSE;
	}
	ppropids->count = ppropnames->count;
	ppropids->ppropid = common_util_alloc(
		sizeof(uint16_t)*ppropnames->count);
	if (NULL == ppropids->ppropid) {
		return FALSE;
	}
	tmp_propnames.count = 0;
	tmp_propnames.ppropname = common_util_alloc(
		sizeof(PROPERTY_NAME)*ppropnames->count);
	if (NULL == tmp_propnames.ppropname) {
		return FALSE;
	}
	for (i=0; i<ppropnames->count; i++) {
		if (0 == guid_compare(&ppropnames->ppropname[i].guid, &guid)) {
			if (KIND_LID == ppropnames->ppropname[i].kind) {
				ppropids->ppropid[i] = *ppropnames->ppropname[i].plid;
			} else {
				ppropids->ppropid[i] = 0;
			}
			pindex_map[i] = i;
			continue;
		}
		guid_to_string(&ppropnames->ppropname[i].guid, tmp_guid, 64);
		switch (ppropnames->ppropname[i].kind) {
		case KIND_LID:
			snprintf(tmp_string, 256, "%s:lid:%u",
				tmp_guid, *ppropnames->ppropname[i].plid);
			break;
		case KIND_NAME:
			snprintf(tmp_string, 256, "%s:name:%s",
				tmp_guid, ppropnames->ppropname[i].pname);
			lower_string(tmp_string);
			break;
		default:
			ppropids->ppropid[i] = 0;
			pindex_map[i] = i;
			continue;
		}
		if (NULL != pstore->ppropname_hash) {
			pid = str_hash_query(pstore->ppropname_hash, tmp_string);
		} else {
			pid = NULL;
		}
		if (NULL != pid) {
			pindex_map[i] = i;
			ppropids->ppropid[i] = *pid;
		} else {
			tmp_propnames.ppropname[tmp_propnames.count] =
									ppropnames->ppropname[i];
			tmp_propnames.count ++;
			pindex_map[i] = tmp_propnames.count * (-1);
		}
	}
	if (0 == tmp_propnames.count) {
		return TRUE;
	}
	if (FALSE == exmdb_client_get_named_propids(pstore->dir,
		b_create, &tmp_propnames, &tmp_propids)) {
		return FALSE;	
	}
	for (i=0; i<ppropnames->count; i++) {
		if (pindex_map[i] < 0) {
			ppropids->ppropid[i] =
				tmp_propids.ppropid[(-1)*pindex_map[i] - 1];
			if (0 != ppropids->ppropid[i]) {
				store_object_cache_propname(pstore,
					ppropids->ppropid[i], ppropnames->ppropname + i);
			}
		}
	}
	return TRUE;
}

PROPERTY_GROUPINFO* store_object_get_last_property_groupinfo(
	STORE_OBJECT *pstore)
{
	if (NULL == pstore->pgpinfo) {
		pstore->pgpinfo = msgchg_grouping_get_groupinfo(
			pstore, msgchg_grouping_get_last_group_id());
	}
	return pstore->pgpinfo;
}

PROPERTY_GROUPINFO* store_object_get_property_groupinfo(
	STORE_OBJECT *pstore, uint32_t group_id)
{
	DOUBLE_LIST_NODE *pnode;
	PROPERTY_GROUPINFO *pgpinfo;
	
	if (group_id == msgchg_grouping_get_last_group_id()) {
		return store_object_get_last_property_groupinfo(pstore);
	}
	for (pnode=double_list_get_head(&pstore->group_list); NULL!=pnode;
		pnode=double_list_get_after(&pstore->group_list, pnode)) {
		pgpinfo = (PROPERTY_GROUPINFO*)pnode->pdata;
		if (pgpinfo->group_id == group_id) {
			return pgpinfo;
		}
	}
	pnode = malloc(sizeof(DOUBLE_LIST_NODE));
	if (NULL == pnode) {
		return NULL;
	}
	pgpinfo = msgchg_grouping_get_groupinfo(pstore, group_id);
	if (NULL == pgpinfo) {
		free(pnode);
		return NULL;
	}
	pnode->pdata = pgpinfo;
	double_list_append_as_tail(&pstore->group_list, pnode);
	return pgpinfo;
}

static BOOL store_object_check_readonly_property(
	STORE_OBJECT *pstore, uint32_t proptag)
{
	if (PROPVAL_TYPE_OBJECT == (proptag & 0xFFFF)) {
		return TRUE;
	}
	switch (proptag) {
	case PROP_TAG_ACCESSLEVEL:
	case PROP_TAG_ADDRESSBOOKDISPLAYNAMEPRINTABLE:
	case PROP_TAG_CODEPAGEID:
	case PROP_TAG_CONTENTCOUNT:
	case PROP_TAG_DEFAULTSTORE:
	case PROP_TAG_DELETEAFTERSUBMIT:
	case PROP_TAG_DELETEDASSOCMESSAGESIZE:
	case PROP_TAG_DELETEDASSOCMESSAGESIZEEXTENDED:
	case PROP_TAG_DELETEDASSOCMSGCOUNT:
	case PROP_TAG_DELETEDMESSAGESIZE:
	case PROP_TAG_DELETEDMESSAGESIZEEXTENDED:
	case PROP_TAG_DELETEDMSGCOUNT:
	case PROP_TAG_DELETEDNORMALMESSAGESIZE:
	case PROP_TAG_DELETEDNORMALMESSAGESIZEEXTENDED:
	case PROP_TAG_EMAILADDRESS:
	case PROP_TAG_ENTRYID:
	case PROP_TAG_EXTENDEDRULESIZELIMIT:
	case PROP_TAG_INTERNETARTICLENUMBER:
	case PROP_TAG_LOCALEID:
	case PROP_TAG_MAXIMUMSUBMITMESSAGESIZE:
	case PROP_TAG_MAILBOXOWNERENTRYID:
	case PROP_TAG_MAILBOXOWNERNAME:
	case PROP_TAG_MESSAGESIZE:
	case PROP_TAG_MESSAGESIZEEXTENDED:
	case PROP_TAG_ASSOCMESSAGESIZE:
	case PROP_TAG_ASSOCMESSAGESIZEEXTENDED:
	case PROP_TAG_NORMALMESSAGESIZE:
	case PROP_TAG_NORMALMESSAGESIZEEXTENDED:
	case PROP_TAG_OBJECTTYPE:
	case PROP_TAG_OUTOFOFFICESTATE:
	case PROP_TAG_PROHIBITRECEIVEQUOTA:
	case PROP_TAG_PROHIBITSENDQUOTA:
	case PROP_TAG_INSTANCEKEY:
	case PROP_TAG_RECORDKEY:
	case PROP_TAG_SEARCHKEY:
	case PROP_TAG_SORTLOCALEID:
	case PROP_TAG_STORAGEQUOTALIMIT:
	case PROP_TAG_STOREENTRYID:
	case PROP_TAG_STOREOFFLINE:
	case PROP_TAG_STOREPROVIDER:
	case PROP_TAG_STORERECORDKEY:
	case PROP_TAG_STORESTATE:
	case PROP_TAG_STORESUPPORTMASK:
	case PROP_TAG_TESTLINESPEED:
	case PROP_TAG_USERENTRYID:
	case PROP_TAG_VALIDFOLDERMASK:
	case PROP_TAG_HIERARCHYSERVER:
	case PROP_TAG_FINDERENTRYID:
	case PROP_TAG_IPMFAVORITESENTRYID:
	case PROP_TAG_IPMSUBTREEENTRYID:
	case PROP_TAG_IPMOUTBOXENTRYID:
	case PROP_TAG_IPMSENTMAILENTRYID:
	case PROP_TAG_IPMWASTEBASKETENTRYID:
	case PROP_TGA_SCHEDULEFOLDERENTRYID:
	case PROP_TAG_IPMPUBLICFOLDERSENTRYID:
	case PROP_TAG_NONIPMSUBTREEENTRYID:
	case PROP_TAG_EFORMSREGISTRYENTRYID:
		return TRUE;
	}
	return FALSE;
}

BOOL store_object_get_all_proptags(STORE_OBJECT *pstore,
	PROPTAG_ARRAY *pproptags)
{
	PROPTAG_ARRAY tmp_proptags;
	
	if (FALSE == exmdb_client_get_store_all_proptags(
		pstore->dir, &tmp_proptags)) {
		return FALSE;	
	}
	pproptags->pproptag = common_util_alloc(
		sizeof(uint32_t)*(tmp_proptags.count + 50));
	if (NULL == pproptags->pproptag) {
		return FALSE;
	}
	memcpy(pproptags->pproptag, tmp_proptags.pproptag,
				sizeof(uint32_t)*tmp_proptags.count);
	pproptags->count = tmp_proptags.count;
	if (TRUE == store_object_check_private(pstore)) {
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_MAILBOXOWNERNAME;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
				PROP_TAG_MAILBOXOWNERENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
				PROP_TAG_MAXIMUMSUBMITMESSAGESIZE;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
						PROP_TAG_EMAILADDRESS;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
		PROP_TAG_ADDRESSBOOKDISPLAYNAMEPRINTABLE;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
						PROP_TAG_FINDERENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_IPMOUTBOXENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_IPMSENTMAILENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
				PROP_TAG_IPMWASTEBASKETENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
				PROP_TGA_SCHEDULEFOLDERENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
							PROP_TAG_OOFSTATE;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_OOFINTERNALREPLY;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_OOFINTERNALSUBJECT;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
							PROP_TAG_OOFBEGIN;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
							PROP_TAG_OOFUNTIL;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_OOFALLOWEXTERNAL;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_OOFEXTERNALAUDIENCE;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
						PROP_TAG_OOFEXTERNALREPLY;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
					PROP_TAG_OOFEXTERNALSUBJECT;
		pproptags->count ++;
		
	} else {
		pproptags->pproptag[pproptags->count] =
						PROP_TAG_HIERARCHYSERVER;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
				PROP_TAG_IPMPUBLICFOLDERSENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
				PROP_TAG_NONIPMSUBTREEENTRYID;
		pproptags->count ++;
		pproptags->pproptag[pproptags->count] =
				PROP_TAG_EFORMSREGISTRYENTRYID;
		pproptags->count ++;
		/* TODO!!! for PROP_TAG_EMAILADDRESS
			check if mail address of public folder exits! */
	}
	pproptags->pproptag[pproptags->count] =
				PROP_TAG_IPMFAVORITESENTRYID;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
				PROP_TAG_IPMSUBTREEENTRYID;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_STOREPROVIDER;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_DEFAULTSTORE;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
						PROP_TAG_DISPLAYNAME;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
			PROP_TAG_EXTENDEDRULESIZELIMIT;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
						PROP_TAG_USERENTRYID;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_CONTENTCOUNT;
	pproptags->count ++;	
	pproptags->pproptag[pproptags->count] =
						PROP_TAG_OBJECTTYPE;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_PROVIDERDISPLAY;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_RESOURCEFLAGS;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_RESOURCETYPE;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
						PROP_TAG_RECORDKEY;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
						PROP_TAG_INSTANCEKEY;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_STORERECORDKEY;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
							PROP_TAG_ENTRYID;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_STOREENTRYID;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
				PROP_TAG_STORESUPPORTMASK;
	pproptags->count ++;
	pproptags->pproptag[pproptags->count] =
					PROP_TAG_ECSERVERVERSION;
	pproptags->count ++;
	return TRUE;
}

static void* store_object_get_oof_property(
	const char *maildir, uint32_t proptag)
{
	int fd;
	int offset;
	char *pbuff;
	char *ptoken;
	int buff_len;
	void *pvalue;
	struct tm dtm;
	struct tm ttm;
	char *str_value;
	struct tm unix_tm;
	int parsed_length;
	char time_buff[64];
	char date_buff[64];
	char subject[1024];
	char temp_path[256];
	CONFIG_FILE *pconfig;
	MIME_FIELD mime_field;
	struct stat node_stat;
	
	pvalue = NULL;
	sprintf(temp_path, "%s/config/autoreply.cfg", maildir);
	pconfig = config_file_init(temp_path);
	switch (proptag) {
	case PROP_TAG_OOFSTATE:
		pvalue = common_util_alloc(sizeof(uint32_t));
		if (NULL == pvalue) {
			break;
		}
		if (NULL == pconfig) {
			*(uint32_t*)pvalue = OOF_STATE_DISABLED;
		} else {
			str_value = config_file_get_value(pconfig, "REPLY_SWITCH");
			if (NULL == str_value) {
				*(uint32_t*)pvalue = OOF_STATE_DISABLED;
			} else {
				if (0 == atoi(str_value)) {
					*(uint32_t*)pvalue = OOF_STATE_DISABLED;
				} else {
					str_value = config_file_get_value(
							pconfig, "DURATION_SWITCH");
					if (NULL == str_value || 0 == atoi(str_value)) {
						*(uint32_t*)pvalue = OOF_STATE_ENABLED;
					} else {
						*(uint32_t*)pvalue = OOF_STATE_SCHEDULED;
					}
				}
			}
		}
		break;
	case PROP_TAG_OOFINTERNALREPLY:
	case PROP_TAG_OOFEXTERNALREPLY:
		if (PROP_TAG_OOFINTERNALREPLY == proptag) {
			sprintf(temp_path, "%s/config/internal-reply", maildir);
		} else {
			sprintf(temp_path, "%s/config/external-reply", maildir);
		}
		if (0 != stat(temp_path, &node_stat)) {
			break;
		}
		buff_len = node_stat.st_size;
		fd = open(temp_path, O_RDONLY);
		if (-1 == fd) {
			break;
		}
		pbuff = common_util_alloc(buff_len + 1);
		if (NULL == pbuff) {
			close(fd);
			break;
		}
		if (buff_len != read(fd, pbuff, buff_len)) {
			close(fd);
			break;
		}
		close(fd);
		pbuff[buff_len] = '\0';
		pvalue = strstr(pbuff, "\r\n\r\n");
		break;
	case PROP_TAG_OOFINTERNALSUBJECT:
	case PROP_TAG_OOFEXTERNALSUBJECT:
		if (PROP_TAG_OOFINTERNALSUBJECT == proptag) {
			sprintf(temp_path, "%s/config/internal-reply", maildir);
		} else {
			sprintf(temp_path, "%s/config/external-reply", maildir);
		}
		if (0 != stat(temp_path, &node_stat)) {
			break;
		}
		buff_len = node_stat.st_size;
		fd = open(temp_path, O_RDONLY);
		if (-1 == fd) {
			break;
		}
		pbuff = common_util_alloc(buff_len);
		if (NULL == pbuff) {
			close(fd);
			break;
		}
		if (buff_len != read(fd, pbuff, buff_len)) {
			close(fd);
			break;
		}
		close(fd);
		offset = 0;
		while (parsed_length = parse_mime_field(pbuff +
			offset, buff_len - offset, &mime_field)) {
			offset += parsed_length;
			if (0 == strncasecmp("Subject", mime_field.field_name, 7)
				&& mime_field.field_value_len < sizeof(subject)) {
				mime_field.field_value[mime_field.field_value_len] = '\0';
				if (TRUE == mime_string_to_utf8("utf-8",
					mime_field.field_value, subject)) {
					pvalue = common_util_dup(subject);
					break;
				}
			}
			if ('\r' == pbuff[offset] && '\n' == pbuff[offset + 1]) {
				break;
			}
		}
		break;
	case PROP_TAG_OOFBEGIN:
		if (NULL == pconfig) {
			break;
		}
		str_value = config_file_get_value(pconfig, "DURATION_DATE");
		if (NULL == str_value) {
			break;
		}
		strncpy(date_buff, str_value, 64);
		str_value = config_file_get_value(pconfig, "DURATION_TIME");
		if (NULL == str_value) {
			break;
		}
		strncpy(time_buff, str_value, 64);
		ptoken = strchr(date_buff, '~');
		if (NULL == ptoken) {
			break;
		}
		*ptoken = '\0';
		memset(&dtm, 0, sizeof(dtm));
		if (NULL == strptime(date_buff, "%Y-%m-%d", &dtm)) {
			break;
		}
		ptoken = strchr(time_buff, '~');
		if (NULL == ptoken) {
			break;
		}
		*ptoken = '\0';
		memset(&ttm, 0, sizeof(ttm));
		if (NULL == strptime(time_buff, "%H:%M", &ttm)) {
			break;
		}
		pvalue = common_util_alloc(sizeof(uint64_t));
		if (NULL == pvalue) {
			break;
		}
		memset(&unix_tm, 0, sizeof(struct tm));
		unix_tm.tm_sec = ttm.tm_sec;
		unix_tm.tm_min = ttm.tm_min;
		unix_tm.tm_hour = ttm.tm_hour;
		unix_tm.tm_mday = dtm.tm_mday;
		unix_tm.tm_mon = dtm.tm_mon;
		unix_tm.tm_year = dtm.tm_year;
		unix_tm.tm_wday = dtm.tm_wday;
		unix_tm.tm_yday = dtm.tm_yday;
		*(uint64_t*)pvalue = common_util_tm_to_nttime(unix_tm);
		break;
	case PROP_TAG_OOFUNTIL:
		if (NULL == pconfig) {
			break;
		}
		str_value = config_file_get_value(pconfig, "DURATION_DATE");
		if (NULL == str_value) {
			break;
		}
		strncpy(date_buff, str_value, 64);
		str_value = config_file_get_value(pconfig, "DURATION_TIME");
		if (NULL == str_value) {
			break;
		}
		strncpy(time_buff, str_value, 64);
		ptoken = strchr(date_buff, '~');
		if (NULL == ptoken) {
			break;
		}
		memset(&dtm, 0, sizeof(dtm));
		if (NULL == strptime(ptoken + 1, "%Y-%m-%d", &dtm)) {
			break;
		}
		ptoken = strchr(time_buff, '~');
		if (NULL == ptoken) {
			break;
		}
		memset(&ttm, 0, sizeof(ttm));
		if (NULL == strptime(ptoken + 1, "%H:%M", &ttm)) {
			break;
		}
		pvalue = common_util_alloc(sizeof(uint64_t));
		if (NULL == pvalue) {
			break;
		}
		memset(&unix_tm, 0, sizeof(struct tm));
		unix_tm.tm_sec = ttm.tm_sec;
		unix_tm.tm_min = ttm.tm_min;
		unix_tm.tm_hour = ttm.tm_hour;
		unix_tm.tm_mday = dtm.tm_mday;
		unix_tm.tm_mon = dtm.tm_mon;
		unix_tm.tm_year = dtm.tm_year;
		unix_tm.tm_wday = dtm.tm_wday;
		unix_tm.tm_yday = dtm.tm_yday;
		*(uint64_t*)pvalue = common_util_tm_to_nttime(unix_tm);
		break;
	case PROP_TAG_OOFALLOWEXTERNAL:
		pvalue = common_util_alloc(sizeof(uint8_t));
		if (NULL == pvalue) {
			break;
		}
		if (NULL == pconfig) {
			*(uint8_t*)pvalue = 0;
			break;
		}
		str_value = config_file_get_value(pconfig, "EXTERNAL_SWITCH");
		if (NULL == str_value || 0 == atoi(str_value)) {
			*(uint8_t*)pvalue = 0;
		} else {
			*(uint8_t*)pvalue = 1;
		}
		break;
	case PROP_TAG_OOFEXTERNALAUDIENCE:
		pvalue = common_util_alloc(sizeof(uint8_t));
		if (NULL == pvalue) {
			break;
		}
		if (NULL == pconfig) {
			*(uint8_t*)pvalue = 0;
			break;
		}
		str_value = config_file_get_value(pconfig, "EXTERNAL_CHECK");
		if (NULL == str_value || 0 == atoi(str_value)) {
			*(uint8_t*)pvalue = 0;
		} else {
			*(uint8_t*)pvalue = 1;
		}
		break;
	}
	if (NULL != pconfig) {
		config_file_free(pconfig);
	}
	return pvalue;
}

static BOOL store_object_get_calculated_property(
	STORE_OBJECT *pstore, uint32_t proptag, void **ppvalue)
{
	int i;
	int temp_len;
	void *pvalue;
	USER_INFO *pinfo;
	char temp_buff[1024];
	static uint64_t tmp_ll = 0;
	static uint8_t private_uid[] = {
		0x54, 0x94, 0xA1, 0xC0, 0x29, 0x7F, 0x10, 0x1B,
		0xA5, 0x87, 0x08, 0x00, 0x2B, 0x2A, 0x25, 0x17
	};
	static uint8_t public_uid[] = {
		0x78, 0xB2, 0xFA, 0x70, 0xAF, 0xF7, 0x11, 0xCD,
		0x9B, 0xC8, 0x00, 0xAA, 0x00, 0x2F, 0xC4, 0x5A
	};
	static uint8_t share_uid[] = {
		0x9E, 0xB4, 0x77, 0x00, 0x74, 0xE4, 0x11, 0xCE,
		0x8C, 0x5E, 0x00, 0xAA, 0x00, 0x42, 0x54, 0xE2
	};
	
	switch (proptag) {
	case PROP_TAG_STOREPROVIDER:
		*ppvalue = common_util_alloc(sizeof(BINARY));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		((BINARY*)*ppvalue)->cb = 16;
		if (TRUE == pstore->b_private) {
			if (TRUE == store_object_check_owner_mode(pstore)) {
				((BINARY*)*ppvalue)->pb = private_uid;
			} else {
				((BINARY*)*ppvalue)->pb = share_uid;
			}
		} else {
			((BINARY*)*ppvalue)->pb = public_uid;
		}
		return TRUE;
	case PROP_TAG_DISPLAYNAME:
		*ppvalue = common_util_alloc(256);
		if (NULL == *ppvalue) {
			return FALSE;
		}
		if (TRUE == pstore->b_private) {
			if (FALSE == system_services_get_user_displayname(
				pstore->account, *ppvalue)) {
				strcpy(*ppvalue, pstore->account);
			}
		} else {
			sprintf(*ppvalue, "Public Folders - %s", pstore->account);
		}
		return TRUE;
	case PROP_TAG_ADDRESSBOOKDISPLAYNAMEPRINTABLE:
		if (FALSE == pstore->b_private) {
			return FALSE;
		}
		*ppvalue = common_util_alloc(256);
		if (NULL == *ppvalue) {
			return FALSE;
		}
		if (FALSE == system_services_get_user_displayname(
			pstore->account, *ppvalue)) {
			return FALSE;	
		}
		temp_len = strlen(*ppvalue);
		for (i=0; i<temp_len; i++) {
			if (0 == isascii(((char*)(*ppvalue))[i])) {
				strcpy(*ppvalue, pstore->account);
				pvalue = strchr(*ppvalue, '@');
				*(char*)pvalue = '\0';
				break;
			}
		}
		return TRUE;
	case PROP_TAG_DEFAULTSTORE:
		*ppvalue = common_util_alloc(sizeof(uint8_t));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		if (TRUE == store_object_check_owner_mode(pstore)) {
			*(uint8_t*)(*ppvalue) = 1;
		} else {
			*(uint8_t*)(*ppvalue) = 0;
		}
		return TRUE;
	case PROP_TAG_EMAILADDRESS:
		if (TRUE == pstore->b_private) {
			if (FALSE == common_util_username_to_essdn(
				pstore->account, temp_buff)) {
				return FALSE;	
			}
		} else {
			if (FALSE == common_util_public_to_essdn(
				pstore->account, temp_buff)) {
				return FALSE;	
			}
		}
		*ppvalue = common_util_alloc(strlen(temp_buff) + 1);
		if (NULL == *ppvalue) {
			return FALSE;
		}
		strcpy(*ppvalue, temp_buff);
		return TRUE;
	case PROP_TAG_EXTENDEDRULESIZELIMIT:
		*ppvalue = common_util_alloc(sizeof(uint32_t));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		*(uint32_t*)(*ppvalue) = common_util_get_param(
						COMMON_UTIL_MAX_EXTRULE_LENGTH);
		return TRUE;
	case PROP_TAG_MAILBOXOWNERENTRYID:
		if (FALSE == pstore->b_private) {
			return FALSE;
		}
		*ppvalue = common_util_username_to_addressbook_entryid(
												pstore->account);
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_MAILBOXOWNERNAME:
		if (FALSE == pstore->b_private) {
			return FALSE;
		}
		if (FALSE == system_services_get_user_displayname(
			pstore->account, temp_buff)) {
			return FALSE;	
		}
		if ('\0' == temp_buff[0]) {
			*ppvalue = common_util_alloc(strlen(pstore->account) + 1);
			if (NULL == *ppvalue) {
				return FALSE;
			}
			strcpy(*ppvalue, pstore->account);
		} else {
			*ppvalue = common_util_alloc(strlen(temp_buff) + 1);
			if (NULL == *ppvalue) {
				return FALSE;
			}
			strcpy(*ppvalue, temp_buff);
		}
		return TRUE;
	case PROP_TAG_MAXIMUMSUBMITMESSAGESIZE:
		*ppvalue = common_util_alloc(sizeof(uint32_t));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		*(uint32_t*)(*ppvalue) = common_util_get_param(
							COMMON_UTIL_MAX_MAIL_LENGTH);
		return TRUE;
	case PROP_TAG_OBJECTTYPE:
		*ppvalue = common_util_alloc(sizeof(uint32_t));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		*(uint32_t*)(*ppvalue) = OBJECT_STORE;
		return TRUE;
	case PROP_TAG_PROVIDERDISPLAY:
		*ppvalue = "Exchange Message Store";
		return TRUE;
	case PROP_TAG_RESOURCEFLAGS:
		*ppvalue = common_util_alloc(sizeof(uint32_t));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		if (TRUE == store_object_check_owner_mode(pstore)) {
			*(uint32_t*)(*ppvalue) = STATUS_PRIMARY_IDENTITY|
					STATUS_DEFAULT_STORE|STATUS_PRIMARY_STORE;
		} else {
			*(uint32_t*)(*ppvalue) = STATUS_NO_DEFAULT_STORE;
		}
		return TRUE;
	case PROP_TAG_RESOURCETYPE:
		*ppvalue = common_util_alloc(sizeof(uint32_t));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		*(uint32_t*)(*ppvalue) = MAPI_STORE_PROVIDER;
		return TRUE;
	case PROP_TAG_STORESUPPORTMASK:
		*ppvalue = common_util_alloc(sizeof(uint32_t));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		if (TRUE == store_object_check_private(pstore)) {
			if (TRUE == store_object_check_owner_mode(pstore)) {
				*(uint32_t*)(*ppvalue) = EC_SUPPORTMASK_OWNER;
			} else {
				*(uint32_t*)(*ppvalue) = EC_SUPPORTMASK_OTHER;
				pinfo = zarafa_server_get_info();
				if (TRUE == common_util_check_delegate_permission(
					pinfo->username, pstore->dir)) {
					*(uint32_t*)(*ppvalue) |= STORE_SUBMIT_OK;
				}
			}
		} else {
			*(uint32_t*)(*ppvalue) = EC_SUPPORTMASK_PUBLIC;
		}
		return TRUE;
	case PROP_TAG_RECORDKEY:
	case PROP_TAG_INSTANCEKEY:
	case PROP_TAG_STORERECORDKEY:
		*ppvalue = common_util_guid_to_binary(pstore->mailbox_guid);
		return TRUE;
	case PROP_TAG_ENTRYID:
	case PROP_TAG_STOREENTRYID:
		*ppvalue = common_util_to_store_entryid(pstore);
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_USERNAME:
		*ppvalue = pinfo->username;
		return TRUE;
	case PROP_TAG_USERENTRYID:
		pinfo = zarafa_server_get_info();
		*ppvalue = common_util_username_to_addressbook_entryid(
												pinfo->username);
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_FINDERENTRYID:
		if (FALSE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PRIVATE_FID_FINDER));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_IPMFAVORITESENTRYID:
		if (TRUE == store_object_check_private(pstore)) {
			*ppvalue = common_util_to_folder_entryid(pstore,
				rop_util_make_eid_ex(1, PRIVATE_FID_SHORTCUTS));
		} else {
			*ppvalue = common_util_to_folder_entryid(pstore,
				rop_util_make_eid_ex(1, PUBLIC_FID_IPMSUBTREE));
		}
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_IPMSUBTREEENTRYID:
		if (TRUE == store_object_check_private(pstore)) {
			*ppvalue = common_util_to_folder_entryid(pstore,
				rop_util_make_eid_ex(1, PRIVATE_FID_IPMSUBTREE));
		} else {
			*ppvalue = common_util_to_folder_entryid(pstore,
				rop_util_make_eid_ex(1, PUBLIC_FID_ROOT));
		}
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_IPMOUTBOXENTRYID:
		if (FALSE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PRIVATE_FID_OUTBOX));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_IPMSENTMAILENTRYID:
		if (FALSE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PRIVATE_FID_SENT_ITEMS));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_IPMWASTEBASKETENTRYID:
		if (FALSE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PRIVATE_FID_DELETED_ITEMS));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TGA_SCHEDULEFOLDERENTRYID:
		if (FALSE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PRIVATE_FID_SCHEDULE));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_COMMONVIEWSENTRYID:
		if (FALSE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PRIVATE_FID_COMMON_VIEWS));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_IPMPUBLICFOLDERSENTRYID:
	case PROP_TAG_NONIPMSUBTREEENTRYID:
		if (TRUE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PUBLIC_FID_NONIPMSUBTREE));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_EFORMSREGISTRYENTRYID:
		if (TRUE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = common_util_to_folder_entryid(pstore,
			rop_util_make_eid_ex(1, PUBLIC_FID_EFORMSREGISTRY));
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	case PROP_TAG_ECSERVERVERSION:
		*ppvalue = ZCORE_VERSION;
		return TRUE;
	case PROP_TAG_OOFSTATE:
	case PROP_TAG_OOFINTERNALREPLY:
	case PROP_TAG_OOFINTERNALSUBJECT:
	case PROP_TAG_OOFBEGIN:
	case PROP_TAG_OOFUNTIL:
	case PROP_TAG_OOFALLOWEXTERNAL:
	case PROP_TAG_OOFEXTERNALAUDIENCE:
	case PROP_TAG_OOFEXTERNALREPLY:
	case PROP_TAG_OOFEXTERNALSUBJECT:
		if (FALSE == store_object_check_private(pstore)) {
			return FALSE;
		}
		*ppvalue = store_object_get_oof_property(
			store_object_get_dir(pstore), proptag);
		if (NULL == *ppvalue) {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL store_object_get_properties(STORE_OBJECT *pstore,
	const PROPTAG_ARRAY *pproptags, TPROPVAL_ARRAY *ppropvals)
{
	int i;
	void *pvalue;
	USER_INFO *pinfo;
	PROPTAG_ARRAY tmp_proptags;
	TPROPVAL_ARRAY tmp_propvals;
	
	ppropvals->ppropval = common_util_alloc(
		sizeof(TAGGED_PROPVAL)*pproptags->count);
	if (NULL == ppropvals->ppropval) {
		return FALSE;
	}
	tmp_proptags.count = 0;
	tmp_proptags.pproptag = common_util_alloc(
			sizeof(uint32_t)*pproptags->count);
	if (NULL == tmp_proptags.pproptag) {
		return FALSE;
	}
	ppropvals->count = 0;
	for (i=0; i<pproptags->count; i++) {
		if (TRUE == store_object_get_calculated_property(
			pstore, pproptags->pproptag[i], &pvalue)) {
			if (NULL == pvalue) {
				return FALSE;
			}
			ppropvals->ppropval[ppropvals->count].proptag =
										pproptags->pproptag[i];
			ppropvals->ppropval[ppropvals->count].pvalue = pvalue;
			ppropvals->count ++;
		} else {
			tmp_proptags.pproptag[tmp_proptags.count] =
								pproptags->pproptag[i];
			tmp_proptags.count ++;
		}
	}
	if (0 == tmp_proptags.count) {
		return TRUE;
	}
	pinfo = zarafa_server_get_info();
	if (TRUE == pstore->b_private &&
		pinfo->user_id == pstore->account_id) {
		for (i=0; i<tmp_proptags.count; i++) {
			pvalue = object_tree_get_zarafa_store_propval(
					pinfo->ptree, tmp_proptags.pproptag[i]);
			if (NULL != pvalue) {
				ppropvals->ppropval[ppropvals->count].proptag =
										tmp_proptags.pproptag[i];
				ppropvals->ppropval[ppropvals->count].pvalue = pvalue;
				ppropvals->count ++;
				tmp_proptags.count --;
				if (i < tmp_proptags.count) {
					memmove(tmp_proptags.pproptag + i,
						tmp_proptags.pproptag + i + 1,
						sizeof(uint32_t)*(tmp_proptags.count - i));
				}
			}
		}	
		if (0 == tmp_proptags.count) {
			return TRUE;
		}	
	}
	if (FALSE == exmdb_client_get_store_properties(
		pstore->dir, pinfo->cpid, &tmp_proptags,
		&tmp_propvals)) {
		return FALSE;	
	}
	if (0 == tmp_propvals.count) {
		return TRUE;
	}
	memcpy(ppropvals->ppropval +
		ppropvals->count, tmp_propvals.ppropval,
		sizeof(TAGGED_PROPVAL)*tmp_propvals.count);
	ppropvals->count += tmp_propvals.count;
	return TRUE;	
}

static BOOL store_object_set_oof_property(const char *maildir,
	uint32_t proptag, const void *pvalue)
{
	int fd;
	char *pbuff;
	int buff_len;
	char *ptoken;
	char *str_value;
	char temp_path[256];
	CONFIG_FILE *pconfig;
	struct stat node_stat;
	
	sprintf(temp_path, "%s/config/autoreply.cfg", maildir);
	if (0 != stat(temp_path, &node_stat)) {
		fd = open(temp_path, O_CREAT|O_TRUNC|O_WRONLY, 0666);
		if (-1 == fd) {
			return FALSE;
		}
		close(fd);
	}
	switch (proptag) {
	case PROP_TAG_OOFSTATE:
		pconfig = config_file_init(temp_path);
		if (NULL == pconfig) {
			return FALSE;
		}
		switch (*(uint32_t*)pvalue) {
		case OOF_STATE_DISABLED:
			config_file_set_value(pconfig, "REPLY_SWITCH", "0");
			break;
		case OOF_STATE_ENABLED:
			config_file_set_value(pconfig, "REPLY_SWITCH", "1");
			config_file_set_value(pconfig, "DURATION_SWITCH", "0");
			break;
		case OOF_STATE_SCHEDULED:
			config_file_set_value(pconfig, "REPLY_SWITCH", "1");
			config_file_set_value(pconfig, "DURATION_SWITCH", "1");
			break;
		}
		if (FALSE == config_file_save(pconfig)) {
			config_file_free(pconfig);
			return FALSE;
		}
		config_file_free(pconfig);
		return TRUE;
	case PROP_TAG_OOFINTERNALREPLY:
	case PROP_TAG_OOFEXTERNALREPLY:
		if (PROP_TAG_OOFINTERNALREPLY == proptag) {
			sprintf(temp_path, "%s/config/internal-reply", maildir);
		} else {
			sprintf(temp_path, "%s/config/external-reply", maildir);
		}
		if (0 != stat(temp_path, &node_stat)) {
			buff_len = node_stat.st_size;
			fd = open(temp_path, O_RDONLY);
			if (-1 == fd) {
				return FALSE;
			}
			pbuff = common_util_alloc(buff_len + strlen(pvalue) + 1);
			if (NULL == pbuff) {
				close(fd);
				return FALSE;
			}
			if (buff_len != read(fd, pbuff, buff_len)) {
				close(fd);
				return FALSE;
			}
			close(fd);
			pbuff[buff_len] = '\0';
			ptoken = strstr(pbuff, "\r\n\r\n");
			if (NULL != ptoken) {
				strcpy(ptoken + 4, pvalue);
				buff_len = strlen(pbuff);
			} else {
				buff_len = sprintf(pbuff, "Content-Type: text/html;\r\n"
								"\tcharset=\"utf-8\"\r\n\r\n%s", pvalue);
			}
		} else {
			pbuff = common_util_alloc(buff_len + 256);
			if (NULL == pbuff) {
				return FALSE;
			}
			buff_len = sprintf(pbuff, "Content-Type: text/html;\r\n"
							"\tcharset=\"utf-8\"\r\n\r\n%s", pvalue);
		}
		fd = open(temp_path, O_CREAT|O_TRUNC|O_WRONLY, 0666);
		if (-1 == fd) {
			return FALSE;
		}
		if (buff_len != write(fd, pbuff, buff_len)) {
			close(fd);
			return FALSE;
		}
		close(fd);
		return TRUE;
	case PROP_TAG_OOFINTERNALSUBJECT:
	case PROP_TAG_OOFEXTERNALSUBJECT:
		if (PROP_TAG_OOFINTERNALSUBJECT == proptag) {
			sprintf(temp_path, "%s/config/internal-reply", maildir);
		} else {
			sprintf(temp_path, "%s/config/external-reply", maildir);
		}
		if (0 != stat(temp_path, &node_stat)) {
			pbuff = common_util_alloc(buff_len + 256);
			if (NULL == pbuff) {
				return FALSE;
			}
			buff_len = sprintf(pbuff, "Content-Type: text/html;\r\n\t"
					"charset=\"utf-8\"\r\nSubject: %s\r\n\r\n", pvalue);
		} else {
			buff_len = node_stat.st_size;
			pbuff = common_util_alloc(buff_len + strlen(pvalue) + 16);
			if (NULL == pbuff) {
				return FALSE;
			}
			ptoken = common_util_alloc(buff_len + 1);
			if (NULL == ptoken) {
				return FALSE;
			}
			fd = open(temp_path, O_RDONLY);
			if (-1 == fd) {
				return FALSE;
			}
			if (buff_len != read(fd, ptoken, buff_len)) {
				close(fd);
				return FALSE;
			}
			close(fd);
			ptoken[buff_len] = '\0';
			ptoken = strstr(ptoken, "\r\n\r\n");
			if (NULL == ptoken) {
				buff_len = sprintf(pbuff, "Content-Type: text/html;\r\n\t"
						"charset=\"utf-8\"\r\nSubject: %s\r\n\r\n", pvalue);
			} else {
				buff_len = sprintf(pbuff, "Content-Type: text/html;\r\n\t"
					"charset=\"utf-8\"\r\nSubject: %s%s", pvalue, ptoken);
			}
		}
		fd = open(temp_path, O_CREAT|O_TRUNC|O_WRONLY, 0666);
		if (-1 == fd) {
			return FALSE;
		}
		if (buff_len != write(fd, pbuff, buff_len)) {
			close(fd);
			return FALSE;
		}
		close(fd);
		return TRUE;
	case PROP_TAG_OOFALLOWEXTERNAL:
		pconfig = config_file_init(temp_path);
		if (NULL == pconfig) {
			return FALSE;
		}
		if (0 == *(uint8_t*)pvalue) {
			config_file_set_value(pconfig, "EXTERNAL_SWITCH", "0");
		} else {
			config_file_set_value(pconfig, "EXTERNAL_SWITCH", "1");
		}
		if (FALSE == config_file_save(pconfig)) {
			config_file_free(pconfig);
			return FALSE;
		}
		config_file_free(pconfig);
		return TRUE;
	case PROP_TAG_OOFEXTERNALAUDIENCE:
		pconfig = config_file_init(temp_path);
		if (NULL == pconfig) {
			return FALSE;
		}
		if (0 == *(uint8_t*)pvalue) {
			config_file_set_value(pconfig, "EXTERNAL_CHECK", "0");
		} else {
			config_file_set_value(pconfig, "EXTERNAL_CHECK", "1");
		}
		if (FALSE == config_file_save(pconfig)) {
			config_file_free(pconfig);
			return FALSE;
		}
		config_file_free(pconfig);
		return TRUE;
	}
	return FALSE;
}

static BOOL store_object_set_oof_schedule(const char *maildir,
	uint64_t begin_time, uint64_t until_time)
{
	int fd;
	struct tm begin_tm;
	struct tm until_tm;
	char temp_path[256];
	char temp_buff[256];
	CONFIG_FILE *pconfig;
	struct stat node_stat;
	
	if (FALSE == common_util_nttime_to_tm(begin_time, &begin_tm) ||
		FALSE == common_util_nttime_to_tm(until_time, &until_tm)) {
		return FALSE;	
	}
	sprintf(temp_path, "%s/config/autoreply.cfg", maildir);
	if (0 != stat(temp_path, &node_stat)) {
		fd = open(temp_path, O_CREAT|O_TRUNC|O_WRONLY, 0666);
		if (-1 == fd) {
			return FALSE;
		}
		close(fd);
	}
	pconfig = config_file_init(temp_path);
	if (NULL == pconfig) {
		return FALSE;
	}
	sprintf(temp_buff, "%d-%d-%d~%d-%d-%d",
		begin_tm.tm_year + 1900, begin_tm.tm_mon + 1,
		begin_tm.tm_mday, until_tm.tm_year + 1900,
		until_tm.tm_mon + 1, until_tm.tm_mday);
	config_file_set_value(pconfig, "DURATION_DATE", temp_buff);
	sprintf(temp_buff, "%02d:%02d~%02d:%02d",
			begin_tm.tm_hour, begin_tm.tm_min,
			until_tm.tm_hour, until_tm.tm_min);
	config_file_set_value(pconfig, "DURATION_TIME", temp_buff);
	if (FALSE == config_file_save(pconfig)) {
		config_file_free(pconfig);
		return FALSE;
	}
	config_file_free(pconfig);
	return TRUE;
}

BOOL store_object_set_properties(STORE_OBJECT *pstore,
	const TPROPVAL_ARRAY *ppropvals)
{
	int i;
	USER_INFO *pinfo;
	uint64_t *poof_until;
	uint64_t *poof_begin;
	
	pinfo = zarafa_server_get_info();
	if (FALSE == pstore->b_private ||
		pinfo->user_id != pstore->account_id) {
		return TRUE;
	}
	poof_begin = common_util_get_propvals(ppropvals, PROP_TAG_OOFBEGIN);
	poof_until = common_util_get_propvals(ppropvals, PROP_TAG_OOFUNTIL);
	if (NULL != poof_begin && NULL != poof_until) {
		if (FALSE == store_object_set_oof_schedule(
			store_object_get_dir(pstore), *poof_begin,
			*poof_until)) {
			return FALSE;	
		}
	}
	for (i=0; i<ppropvals->count; i++) {
		if (TRUE == store_object_check_readonly_property(
			pstore, ppropvals->ppropval[i].proptag)) {
			continue;
		}
		switch (ppropvals->ppropval[i].proptag) {
		case PROP_TAG_OOFBEGIN:
		case PROP_TAG_OOFUNTIL:
			continue;
		case PROP_TAG_OOFSTATE:
		case PROP_TAG_OOFINTERNALREPLY:
		case PROP_TAG_OOFINTERNALSUBJECT:
		case PROP_TAG_OOFALLOWEXTERNAL:
		case PROP_TAG_OOFEXTERNALAUDIENCE:
		case PROP_TAG_OOFEXTERNALREPLY:
			if (FALSE == store_object_set_oof_property(
				store_object_get_dir(pstore),
				ppropvals->ppropval[i].proptag,
				ppropvals->ppropval[i].pvalue)) {
				return FALSE;	
			}
			break;
		default:
			if (FALSE == object_tree_set_zarafa_store_propval(
				pinfo->ptree, &ppropvals->ppropval[i])) {
				return FALSE;	
			}
			break;
		}
	}
	return TRUE;
}

BOOL store_object_remove_properties(STORE_OBJECT *pstore,
	const PROPTAG_ARRAY *pproptags)
{
	int i;
	USER_INFO *pinfo;
	
	pinfo = zarafa_server_get_info();
	if (TRUE == pstore->b_private &&
		pinfo->user_id == pstore->account_id) {
		return TRUE;
	}
	for (i=0; i<pproptags->count; i++) {
		if (TRUE == store_object_check_readonly_property(
			pstore, pproptags->pproptag[i])) {
			continue;
		}
		object_tree_remove_zarafa_store_propval(
			pinfo->ptree, pproptags->pproptag[i]);
	}
}

static BOOL store_object_get_folder_permissions(
	STORE_OBJECT *pstore, uint64_t folder_id,
	PERMISSION_SET *pperm_set)
{
	int i, j;
	const char *dir;
	BINARY *pentryid;
	uint32_t row_num;
	uint32_t table_id;
	uint32_t *prights;
	uint32_t max_count;
	PROPTAG_ARRAY proptags;
	TARRAY_SET permission_set;
	PERMISSION_ROW *pperm_row;
	static uint32_t proptag_buff[] = {
		PROP_TAG_ENTRYID,
		PROP_TAG_MEMBERRIGHTS
	};
	
	if (FALSE == exmdb_client_load_permission_table(
		pstore->dir, folder_id, 0, &table_id, &row_num)) {
		return FALSE;
	}
	proptags.count = 2;
	proptags.pproptag = proptag_buff;
	if (FALSE == exmdb_client_query_table(dir, NULL, 0,
		table_id, &proptags, 0, row_num, &permission_set)) {
		exmdb_client_unload_table(dir, table_id);
		return FALSE;
	}
	exmdb_client_unload_table(pstore->dir, table_id);
	max_count = (pperm_set->count/100)*100;
	for (i=0; i<permission_set.count; i++) {
		if (max_count == pperm_set->count) {
			max_count += 100;
			pperm_row = common_util_alloc(
				sizeof(PERMISSION_ROW)*max_count);
			if (NULL == pperm_row) {
				return FALSE;
			}
			if (0 != pperm_set->count) {
				memcpy(pperm_row, pperm_set->prows,
					sizeof(PERMISSION_ROW)*pperm_set->count);
			}
			pperm_set->prows = pperm_row;
		}
		pentryid = common_util_get_propvals(
				permission_set.pparray[i],
				PROP_TAG_ENTRYID);
		/* ignore the default and anonymous user */
		if (NULL == pentryid || 0 == pentryid->cb) {
			continue;
		}
		for (j=0; j<pperm_set->count; j++) {
			if (pperm_set->prows[j].entryid.cb ==
				pentryid->cb && 0 == memcmp(
				pperm_set->prows[j].entryid.pb,
				pentryid->pb, pentryid->cb)) {
				break;	
			}
		}
		prights = common_util_get_propvals(
				permission_set.pparray[i],
				PROP_TAG_MEMBERRIGHTS);
		if (NULL == prights) {
			continue;
		}
		if (j < pperm_set->count) {
			pperm_set->prows[j].member_rights |= *prights;
			continue;
		}
		pperm_set->prows[pperm_set->count].flags = RIGHT_NORMAL;
		pperm_set->prows[pperm_set->count].entryid = *pentryid;
		pperm_set->prows[pperm_set->count].member_rights = *prights;
		pperm_set->count ++;
	}
	return TRUE;
}

BOOL store_object_get_permissions(STORE_OBJECT *pstore,
	PERMISSION_SET *pperm_set)
{
	int i;
	uint32_t row_num;
	uint32_t table_id;
	TARRAY_SET tmp_set;
	uint64_t folder_id;
	uint32_t tmp_proptag;
	PROPTAG_ARRAY proptags;
	
	if (TRUE == pstore->b_private) {
		folder_id = rop_util_make_eid_ex(1, PRIVATE_FID_IPMSUBTREE);
	} else {
		folder_id = rop_util_make_eid_ex(1, PUBLIC_FID_IPMSUBTREE);
	}
	if (FALSE == exmdb_client_load_hierarchy_table(
		pstore->dir, folder_id, NULL, TABLE_FLAG_DEPTH,
		NULL, &table_id, &row_num)) {
		return FALSE;
	}
	proptags.count = 1;
	proptags.pproptag = &tmp_proptag;
	tmp_proptag = PROP_TAG_FOLDERID;
	if (FALSE == exmdb_client_query_table(pstore->dir, NULL,
		0, table_id, &proptags, 0, row_num, &tmp_set)) {
		return FALSE;
	}
	pperm_set->count = 0;
	pperm_set->prows = NULL;
	for (i=0; i<tmp_set.count; i++) {
		if (0 == tmp_set.pparray[i]->count) {
			continue;
		}
		if (FALSE == store_object_get_folder_permissions(pstore,
			*(uint64_t*)tmp_set.pparray[i]->ppropval[0].pvalue,
			pperm_set)) {
			return FALSE;	
		}
	}
	return TRUE;
}
