#pragma once

enum {
	ropRelease = 0x01,
	ropOpenFolder = 0x02,
	ropOpenMessage = 0x03,
	ropGetHierarchyTable = 0x04,
	ropGetContentsTable = 0x05,
	ropCreateMessage = 0x06,
	ropGetPropertiesSpecific = 0x07,
	ropGetPropertiesAll = 0x08,
	ropGetPropertiesList = 0x09,
	ropSetProperties = 0x0A,
	ropDeleteProperties = 0x0B,
	ropSaveChangesMessage = 0x0C,
	ropRemoveAllRecipients = 0x0D,
	ropModifyRecipients = 0x0E,
	ropReadRecipients = 0x0F,
	ropReloadCachedInformation = 0x10,
	ropSetMessageReadFlag = 0x11,
	ropSetColumns = 0x12,
	ropSortTable = 0x13,
	ropRestrict = 0x14,
	ropQueryRows = 0x15,
	ropGetStatus = 0x16,
	ropQueryPosition = 0x17,
	ropSeekRow = 0x18,
	ropSeekRowBookmark = 0x19,
	ropSeekRowFractional = 0x1A,
	ropCreateBookmark = 0x1B,
	ropCreateFolder = 0x1C,
	ropDeleteFolder = 0x1D,
	ropDeleteMessages = 0x1E,
	ropGetMessageStatus = 0x1F,
	ropSetMessageStatus = 0x20,
	ropGetAttachmentTable = 0x21,
	ropOpenAttachment = 0x22,
	ropCreateAttachment = 0x23,
	ropDeleteAttachment = 0x24,
	ropSaveChangesAttachment = 0x25,
	ropSetReceiveFolder = 0x26,
	ropGetReceiveFolder = 0x27,
	ropRegisterNotification = 0x29,
	ropRegisterNotify = 0x2A,
	ropOpenStream = 0x2B,
	ropReadStream = 0x2C,
	ropWriteStream = 0x2D,
	ropSeekStream = 0x2E,
	ropSetStreamSize = 0x2F,
	ropSetSearchCriteria = 0x30,
	ropGetSearchCriteria = 0x31,
	ropSubmitMessage = 0x32,
	ropMoveCopyMessages = 0x33,
	ropAbortSubmit = 0x34,
	ropMoveFolder = 0x35,
	ropCopyFolder = 0x36,
	ropQueryColumnsAll = 0x37,
	ropAbort = 0x38,
	ropCopyTo = 0x39,
	ropCopyToStream = 0x3A,
	ropCloneStream = 0x3B,
	ropGetPermissionsTable = 0x3E,
	ropGetRulesTable = 0x3F,
	ropModifyPermissions = 0x40,
	ropModifyRules = 0x41,
	ropGetOwningServers = 0x42,
	ropLongTermIdFromId = 0x43,
	ropIdFromLongTermId = 0x44,
	ropPublicFolderIsGhosted = 0x45,
	ropOpenEmbeddedMessage = 0x46,
	ropSetSpooler = 0x47,
	ropSpoolerLockMessage = 0x48,
	ropGetAddressTypes = 0x49,
	ropTransportSend = 0x4A,
	ropFastTransferSourceCopyMessages = 0x4B,
	ropFastTransferSourceCopyFolder = 0x4C,
	ropFastTransferSourceCopyTo = 0x4D,
	ropFastTransferSourceGetBuffer = 0x4E,
	ropFindRow = 0x4F,
	ropProgress = 0x50,
	ropTransportNewMail = 0x51,
	ropGetValidAttachments = 0x52,
	ropFastTransferDestinationConfigure = 0x53,
	ropFastTransferDestinationPutBuffer = 0x54,
	ropGetNamesFromPropertyIds = 0x55,
	ropGetPropertyIdsFromNames = 0x56,
	ropUpdateDeferredActionMessages = 0x57,
	ropEmptyFolder = 0x58,
	ropExpandRow = 0x59,
	ropCollapseRow = 0x5A,
	ropLockRegionStream = 0x5B,
	ropUnlockRegionStream = 0x5C,
	ropCommitStream = 0x5D,
	ropGetStreamSize = 0x5E,
	ropQueryNamedProperties = 0x5F,
	ropGetPerUserLongTermIds = 0x60,
	ropGetPerUserGuid = 0x61,
	ropReadPerUserInformation = 0x63,
	ropWritePerUserInformation = 0x64,
	ropSetReadFlags = 0x66,
	ropCopyProperties = 0x67,
	ropGetReceiveFolderTable = 0x68,
	ropFastTransferSourceCopyProperties = 0x69,
	ropGetCollapseState = 0x6B,
	ropSetCollapseState = 0x6C,
	ropGetTransportFolder = 0x6D,
	ropPending = 0x6E,
	ropOptionsData = 0x6F,
	ropSynchronizationConfigure = 0x70,
	ropSynchronizationImportMessageChange = 0x72,
	ropSynchronizationImportHierarchyChange = 0x73,
	ropSynchronizationImportDeletes = 0x74,
	ropSynchronizationUploadStateStreamBegin = 0x75,
	ropSynchronizationUploadStateStreamContinue = 0x76,
	ropSynchronizationUploadStateStreamEnd = 0x77,
	ropSynchronizationImportMessageMove = 0x78,
	ropSetPropertiesNoReplicate = 0x79,
	ropDeletePropertiesNoReplicate = 0x7A,
	ropGetStoreState = 0x7B,
	ropSynchronizationOpenCollector = 0x7E,
	ropGetLocalReplicaIds = 0x7F,
	ropSynchronizationImportReadStateChanges = 0x80,
	ropResetTable = 0x81,
	ropSynchronizationGetTransferState = 0x82,
	ropSynchronizationOpenAdvisor = 0x83, /* removed in OXCROPS v9 */
	ropRegisterSynchronizationNotifications = 0x84, /* gone in v9 */
	ropTellVersion = 0x86,
	ropOpenPublicFolderByName = 0x87, /* removed in OXCROPS v1.02 */
	ropSetSynchronizationNotificationGuid = 0x88, /* gone in v9 */
	ropFreeBookmark = 0x89,
	ropDeletePublicFolderByName = 0x8A, /* gone in v1.02 */
	ropWriteAndCommitStream = 0x90,
	ropHardDeleteMessages = 0x91,
	ropHardDeleteMessagesAndSubfolders = 0x92,
	ropSetLocalReplicaMidsetDeleted = 0x93,
	ropBackoff = 0xF9,
	ropLogon = 0xFE,
	ropBufferTooSmall = 0xFF,
};

#ifdef __cplusplus
extern "C" {
#endif

extern const char *rop_idtoname(unsigned int i);

#ifdef __cplusplus
}
#endif
