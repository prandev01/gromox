#pragma once
#include <cstdint>
#include <gromox/mapi_types.hpp>
#include <gromox/ext_buffer.hpp>

struct LOGMAP;

extern uint32_t rop_logon_pmb(uint8_t logon_flags, uint32_t open_flags, uint32_t store_stat, char *essdn, size_t dnmax, uint64_t *folder_id, uint8_t *response_flags, GUID *mailbox_guid, uint16_t *replica_id, GUID *preplica_guid, LOGON_TIME *logon_time, uint64_t *pgwart_time, uint32_t *store_stat_out, LOGMAP *, uint8_t logon_id, uint32_t *hout);
extern uint32_t rop_logon_pf(uint8_t logon_flags, uint32_t open_flags, uint32_t store_stat, char *essdn, uint64_t *folder_id, uint16_t *replica_id, GUID *replica_guid, GUID *per_user_guid, LOGMAP *, uint8_t logon_id, uint32_t *hout);
extern uint32_t rop_getreceivefolder(const char *str_class, uint64_t *folder_id, char **str_explicit, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setreceivefolder(uint64_t folder_id, const char *str_class, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getreceivefoldertable(PROPROW_SET *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getstorestat(uint32_t *pstat, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getowningservers(uint64_t folder_id, GHOST_SERVER *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_publicfolderisghosted(uint64_t folder_id, GHOST_SERVER **, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_longtermidfromid(uint64_t id, LONG_TERM_ID *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_idfromlongtermid(const LONG_TERM_ID *, uint64_t *pid, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getperuserlongtermids(const GUID *, LONG_TERM_ID_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getperuserguid(const LONG_TERM_ID *, GUID *, LOGMAP *, uint8_t logon_id,  uint32_t hin);
extern uint32_t rop_readperuserinformation(const LONG_TERM_ID *, uint8_t reserved, uint32_t data_offset, uint16_t max_data_size, uint8_t *has_finished, BINARY *data, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_writeperuserinformation(const LONG_TERM_ID *, uint8_t has_finished, uint32_t offset, const BINARY *, const GUID *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_openfolder(uint64_t folder_id, uint8_t open_flags, uint8_t *has_rules, GHOST_SERVER **, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_createfolder(uint8_t folder_type, uint8_t use_unicode, uint8_t open_existing, uint8_t reserved, const char *fld_name, const char *fld_comment, uint64_t *fld_id, uint8_t *is_existing, uint8_t *has_rules, GHOST_SERVER **, LOGMAP *, uint8_t logon_id,  uint32_t hin, uint32_t *hout);
extern uint32_t rop_deletefolder(uint8_t flags, uint64_t folder_id, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setsearchcriteria(const RESTRICTION *, const LONGLONG_ARRAY *folder_ids, uint32_t search_flags, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getsearchcriteria(uint8_t use_unicode, uint8_t include_restriction, uint8_t include_folders, RESTRICTION **, LONGLONG_ARRAY *folder_ids, uint32_t *search_flags, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_movecopymessages(const LONGLONG_ARRAY *msg_ids, uint8_t want_async, uint8_t want_copy, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hsrc, uint32_t hdst);
extern uint32_t rop_movefolder(uint8_t want_async, uint8_t use_unicode, uint64_t folder_id, const char *new_name, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hsrc, uint32_t hdst);
extern uint32_t rop_copyfolder(uint8_t want_async, uint8_t want_recursive, uint8_t use_unicode, uint64_t folder_id, const char *new_name, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hsrc, uint32_t hdst);
extern uint32_t rop_emptyfolder(uint8_t want_async, uint8_t want_delete_associated, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_harddeletemessagesandsubfolders(uint8_t want_async, uint8_t want_delete_associated, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_deletemessages(uint8_t want_async, uint8_t notify_non_read, const LONGLONG_ARRAY *msg_ids, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_harddeletemessages(uint8_t want_async, uint8_t notify_non_read, const LONGLONG_ARRAY *msg_ids, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_gethierarchytable(uint8_t tbl_flags, uint32_t *row_count, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_getcontentstable(uint8_t tbl_flags, uint32_t *row_count, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_setcolumns(uint8_t tbl_flags, const PROPTAG_ARRAY *, uint8_t *tbl_status, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_sorttable(uint8_t tbl_flags, const SORTORDER_SET *criteria, uint8_t *tbl_status, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_restrict(uint8_t res_flags, const RESTRICTION *, uint8_t *tbl_status, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_queryrows(uint8_t flags, uint8_t forward_read, uint16_t row_count, uint8_t *seek_pos, uint16_t *count, EXT_PUSH *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_abort(uint8_t *tbl_status, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getstatus(uint8_t *tbl_status, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_queryposition(uint32_t *numerator, uint32_t *denominator, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_seekrow(uint8_t seek_pos, int32_t offset, uint8_t want_moved_count, uint8_t *has_soughtless, int32_t *offset_sought, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_seekrowbookmark(const BINARY *bookmark, int32_t offset, uint8_t want_moved_count, uint8_t *row_invisible, uint8_t *has_soughtless, uint32_t *offset_sought, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_seekrowfractional(uint32_t numerator, uint32_t denominator, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_createbookmark(BINARY *bookmark, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_querycolumnsall(PROPTAG_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_findrow(uint8_t flags, const RESTRICTION *, uint8_t seek_pos, const BINARY *bookmark, uint8_t *bookmark_invisible, PROPERTY_ROW **row, PROPTAG_ARRAY **cols, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_freebookmark(const BINARY *bookmark, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_resettable(LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_expandrow(uint16_t max_count, uint64_t category_id, uint32_t *expanded_count, uint16_t *count, EXT_PUSH *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_collapserow(uint64_t category_id, uint32_t *collapsed_count, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getcollapsestate(uint64_t row_id, uint32_t row_instance, BINARY *collapse_state, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setcollapsestate(const BINARY *collapse_state, BINARY *bookmark, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_openmessage(uint16_t cpid, uint64_t folder_id, uint8_t open_mode_flags, uint64_t msg_id, uint8_t *has_named_properties, TYPED_STRING *subject_prefix, TYPED_STRING *normalized_subject, uint16_t *rcpt_count, PROPTAG_ARRAY *rcpt_cols, uint8_t *prow_count, OPENRECIPIENT_ROW **rcpt_row, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_createmessage(uint16_t cpid, uint64_t folder_id, uint8_t associated_flag, uint64_t **msg_id, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_savechangesmessage(uint8_t save_flags, uint64_t *msg_id, LOGMAP *, uint8_t logon_id, uint32_t hresponse, uint32_t hin);
extern uint32_t rop_removeallrecipients(uint32_t reserved, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_modifyrecipients(const PROPTAG_ARRAY *, uint16_t count, const MODIFYRECIPIENT_ROW *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_readrecipients(uint32_t row_id, uint16_t reserved, uint8_t *count, EXT_PUSH *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_reloadcachedinformation(uint16_t reserved, uint8_t *has_named_properties, TYPED_STRING *subject_prefix, TYPED_STRING *normalized_subject, uint16_t *rcpt_count, PROPTAG_ARRAY *rcpt_cols, uint8_t *row_count, OPENRECIPIENT_ROW **rcpt_row, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setmessagestatus(uint64_t msg_id, uint32_t msg_status, uint32_t status_mask, uint32_t *msg_status_out, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getmessagestatus(uint64_t msg_id, uint32_t *msg_status, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setreadflags(uint8_t want_async, uint8_t read_flags, const LONGLONG_ARRAY *msg_ids, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setmessagereadflag(uint8_t read_flags, const LONG_TERM_ID *client_data, uint8_t *read_change, LOGMAP *, uint8_t logon_id, uint32_t hresponse, uint32_t hin);
extern uint32_t rop_openattachment(uint8_t flags, uint32_t attachment_id, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_createattachment(uint32_t *pattachment_id, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_deleteattachment(uint32_t attachment_id, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_savechangesattachment(uint8_t save_flags, LOGMAP *, uint8_t logon_id, uint32_t hresponse, uint32_t hin);
extern uint32_t rop_openembeddedmessage(uint16_t cpid, uint8_t open_embedded_flags, uint8_t *reserved, uint64_t *msg_id, uint8_t *has_named_properties, TYPED_STRING *subject_prefix, TYPED_STRING *normalized_subject, uint16_t *rcpt_count, PROPTAG_ARRAY *rcpt_cols, uint8_t *row_count, OPENRECIPIENT_ROW **rcpt_row, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_getattachmenttable(uint8_t tbl_flags, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_getvalidattachments(LONG_ARRAY *attachment_ids, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_submitmessage(uint8_t submit_flags, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_abortsubmit(uint64_t folder_id, uint64_t msg_id, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getaddresstypes(STRING_ARRAY *addr_types, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setspooler(LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_spoolerlockmessage(uint64_t msg_id, uint8_t lock_stat, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_transportsend(TPROPVAL_ARRAY **, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_transportnewmail(uint64_t msg_id, uint64_t folder_id, const char *str_class, uint32_t message_flags, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_gettransportfolder(uint64_t *folder_id, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_optionsdata(const char *addr_type, uint8_t want_win32, uint8_t *reserved, BINARY *options_info, BINARY *help_file, char **file_name, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getpropertyidsfromnames(uint8_t flags, const PROPNAME_ARRAY *, PROPID_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getnamesfrompropertyids(const PROPID_ARRAY *, PROPNAME_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getpropertiesspecific(uint16_t size_limit, uint16_t want_unicode, const PROPTAG_ARRAY *, PROPERTY_ROW *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getpropertiesall(uint16_t size_limit, uint16_t want_unicode, TPROPVAL_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getpropertieslist(PROPTAG_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setproperties(const TPROPVAL_ARRAY *, PROBLEM_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setpropertiesnoreplicate(const TPROPVAL_ARRAY *, PROBLEM_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_deleteproperties(const PROPTAG_ARRAY *, PROBLEM_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_deletepropertiesnoreplicate(const PROPTAG_ARRAY *, PROBLEM_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_querynamedproperties(uint8_t query_flags, const GUID *, PROPIDNAME_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_copyproperties(uint8_t want_async, uint8_t copy_flags, const PROPTAG_ARRAY *, PROBLEM_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hsrc, uint32_t hdst);
extern uint32_t rop_copyto(uint8_t want_async, uint8_t want_subobjects, uint8_t copy_flags, const PROPTAG_ARRAY *exclprop, PROBLEM_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hsrc, uint32_t hdst);
extern uint32_t rop_progress(uint8_t want_cancel, uint32_t *completed_count, uint32_t *total_count, uint8_t *rop_id, uint8_t *partial_completion, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_openstream(uint32_t proptag, uint8_t flags, uint32_t *stream_size, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_readstream(uint16_t byte_count, uint32_t max_byte_count, BINARY *data, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_writestream(const BINARY *data, uint16_t *written_size, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_commitstream(LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getstreamsize(uint32_t *stream_size, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setstreamsize(uint64_t stream_size, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_seekstream(uint8_t seek_pos, int64_t offset, uint64_t *new_pos, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_copytostream(uint64_t byte_count, uint64_t *read_bytes, uint64_t *written_bytes, LOGMAP *, uint8_t logon_id, uint32_t hsrc, uint32_t hdst);
extern uint32_t rop_lockregionstream(uint64_t region_offset, uint64_t region_size, uint32_t lock_flags, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_unlockregionstream(uint64_t region_offset, uint64_t region_size, uint32_t lock_flags, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_writeandcommitstream(const BINARY *data, uint16_t *written_size, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_clonestream(LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_modifypermissions(uint8_t flags, uint16_t count, const PERMISSION_DATA *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getpermissionstable(uint8_t flags, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_modifyrules(uint8_t flags, uint16_t count, const RULE_DATA *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getrulestable(uint8_t flags, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_updatedeferredactionmessages(const BINARY *server_entry_id, const BINARY *client_entry_id, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_fasttransferdestconfigure(uint8_t source_operation, uint8_t flags, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_fasttransferdestputbuffer(const BINARY *xfer_data, uint16_t *xfer_status, uint16_t *in_progress_count, uint16_t *total_step_count, uint8_t *reserved, uint16_t *used_size, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_fasttransfersourcegetbuffer(uint16_t buffer_size, uint16_t max_buffer_size, uint16_t *xfer_status, uint16_t *in_progress_count, uint16_t *total_step_count, uint8_t *reserved, BINARY *xfer_data, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_fasttransfersourcecopyfolder(uint8_t flags, uint8_t send_options, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_fasttransfersourcecopymessages(const LONGLONG_ARRAY *msg_ids, uint8_t flags, uint8_t send_options, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_fasttransfersourcecopyto(uint8_t level, uint32_t flags, uint8_t send_options, const PROPTAG_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_fasttransfersourcecopyproperties(uint8_t level, uint8_t flags, uint8_t send_options, const PROPTAG_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_tellversion(const uint16_t *version, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_syncconfigure(uint8_t sync_type, uint8_t send_options, uint16_t sync_flags, const RESTRICTION *, uint32_t extra_flags, const PROPTAG_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_syncimportmessagechange(uint8_t import_flags, const TPROPVAL_ARRAY *, uint64_t *msg_id, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_syncimportreadstatechanges(uint16_t count, const MESSAGE_READ_STAT *read_stat, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_syncimporthierarchychange(const TPROPVAL_ARRAY *hichyvals, const TPROPVAL_ARRAY *propvals, uint64_t *folder_id, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_syncimportdeletes(uint8_t flags, const TPROPVAL_ARRAY *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_syncimportmessagemove(const BINARY *src_folder_id, const BINARY *src_msg_id, const BINARY *change_list, const BINARY *dst_msg_id, const BINARY *change_number, uint64_t *msg_id, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_syncopencollector(uint8_t is_content_collector, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_syncgettransferstate(LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern uint32_t rop_syncuploadstatestreambegin(uint32_t proptag_state, uint32_t buffer_size, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_syncuploadstatestreamcontinue(const BINARY *stream_data, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_syncuploadstatestreamend(LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_setlocalreplicamidsetdeleted(uint32_t count, const LONG_TERM_ID_RANGE *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_getlocalreplicaids(uint32_t count, GUID *, GLOBCNT *, LOGMAP *, uint8_t logon_id, uint32_t hin);
extern uint32_t rop_registernotification(uint8_t notification_types, uint8_t reserved, uint8_t want_whole_store, const uint64_t *folder_id, const uint64_t *msg_id, LOGMAP *, uint8_t logon_id, uint32_t hin, uint32_t *hout);
extern void rop_release(LOGMAP *, uint8_t logon_id, uint32_t hin);
