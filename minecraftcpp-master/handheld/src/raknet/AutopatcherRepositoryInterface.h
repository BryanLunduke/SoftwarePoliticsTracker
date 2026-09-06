///
/// \file AutopatcherRepositoryInterface.h
/// \brief An interface used by AutopatcherServer to get the data necessary to run an autopatcher.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
/// Usage of RakNet is subject to the appropriate license agreement.
///

#ifndef __AUTOPATCHER_REPOSITORY_INTERFACE_H
#define __AUTOPATCHER_REPOSITORY_INTERFACE_H

#include "IncrementalReadInterface.h"
#include "SimpleMutex.h"

namespace RakNet
{
/// Forward declarations
class FileList;
class BitStream;

/// An interface used by AutopatcherServer to get the data necessary to run an autopatcher.  This is up to you to implement for custom repository solutions.
class AutopatcherRepositoryInterface : public IncrementalReadInterface
{
public:
	/// Get list of files added and deleted since a certain date.  This is used by AutopatcherServer and not usually explicitly called.
	/// \param[in] applicationName A null terminated string identifying the application
	/// \param[out] addedFiles A list of the current versions of filenames with hashes as their data that were created after \a sinceData
	/// \param[out] deletedFiles A list of the current versions of filenames that were deleted after \a sinceData
	/// \param[in] An input date, in whatever format your repository uses
	/// \param[out] currentDate The current server date, in whatever format your repository uses
	/// \return True on success, false on failure.
	virtual bool GetChangelistSinceDate(const char *applicationName, FileList *addedFiles, FileList *deletedFiles, double sinceDate)=0;

	/// Get patches (or files) for every file in input, assuming that input has a hash for each of those files.
	/// \param[in] applicationName A null terminated string identifying the application
	/// \param[in] input A list of files with SHA1_LENGTH byte hashes to get from the database.
	/// \param[out] patchList You should return list of files with either the filedata or the patch.  This is a subset of \a input.  The context data for each file will be either PC_WRITE_FILE (to just write the file) or PC_HASH_WITH_PATCH (to patch).  If PC_HASH_WITH_PATCH, then the file contains a SHA1_LENGTH byte patch followed by the hash.  The datalength is patchlength + SHA1_LENGTH
	/// \param[out] currentDate The current server date, in whatever format your repository uses
	/// \return True on success, false on failure.
	virtual bool GetPatches(const char *applicationName, FileList *input, FileList *patchList)=0;

	/// For the most recent update, return files that were patched, added, or deleted. For files that were patched, return both the patch in \a patchedFiles and the current version in \a updatedFiles
	/// The cache will be used if the client last patched between \a priorRowPatchTime and \a mostRecentRowPatchTime
	/// No files changed will be returned to the client if the client last patched after mostRecentRowPatchTime
	/// \param[in,out] applicationName Name of the application to get patches for. If empty, uses the most recently updated application, and the string will be updated to reflect this name.
	/// \param[out] patchedFiles Given the most recent update, if a file was patched, add it to this list. The context data for each file will be PC_HASH_WITH_PATCH. The first 4 bytes of data should be a hash of the file being patched. The second 4 bytes should be the hash of the file after the patch. The remaining bytes should be the patch itself.
	/// \param[out] updatedFiles The current value of the file. List should have the same length and order as \a patchedFiles
	/// \param[out] updatedFileHashes The hash of the current value of the file. List should have the same length and order as \a patchedFiles
	/// \param[out] deletedFiles Files that were deleted in the last patch.
	/// \param[out] priorRowPatchTime  When the patch before the most recent patch took place. 0 means never.
	/// \param[out] mostRecentRowPatchTime When the most recent patch took place. 0 means never. 
	/// \return true on success, false on failure
	virtual bool GetMostRecentChangelistWithPatches(
		RakNet::RakString &applicationName,
		FileList *patchedFiles,
		FileList *updatedFiles,
		FileList *updatedFileHashes,
		FileList *deletedFiles,
		double *priorRowPatchTime,
		double *mostRecentRowPatchTime)=0;

	/// \return Whatever this function returns is sent from the AutopatcherServer to the AutopatcherClient when one of the above functions returns false.
	virtual const char *GetLastError(void) const=0;

	/// \return Passed to FileListTransfer::Send() as the _chunkSize parameter.
	virtual const int GetIncrementalReadChunkSize(void) const=0;
};

} // namespace RakNet

#endif

