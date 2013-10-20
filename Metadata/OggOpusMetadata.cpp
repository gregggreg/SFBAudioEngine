/*
 *  Copyright (C) 2011, 2012, 2013 Stephen F. Booth <me@sbooth.org>
 *  All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    - Neither the name of Stephen F. Booth nor the names of its 
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memory>

#include <taglib/tfilestream.h>
#include <taglib/opusfile.h>

#include "OggOpusMetadata.h"
#include "CFWrapper.h"
#include "CFErrorUtilities.h"
#include "AddXiphCommentToDictionary.h"
#include "SetXiphCommentFromMetadata.h"
#include "AddAudioPropertiesToDictionary.h"

namespace {

	void RegisterOggOpusMetadata() __attribute__ ((constructor));
	void RegisterOggOpusMetadata()
	{
		AudioMetadata::RegisterSubclass<OggOpusMetadata>();
	}

}

#pragma mark Static Methods

CFArrayRef OggOpusMetadata::CreateSupportedFileExtensions()
{
	CFStringRef supportedExtensions [] = { CFSTR("opus") };
	return CFArrayCreate(kCFAllocatorDefault, (const void **)supportedExtensions, 1, &kCFTypeArrayCallBacks);
}

CFArrayRef OggOpusMetadata::CreateSupportedMIMETypes()
{
	CFStringRef supportedMIMETypes [] = { CFSTR("audio/opus") };
	return CFArrayCreate(kCFAllocatorDefault, (const void **)supportedMIMETypes, 1, &kCFTypeArrayCallBacks);
}

bool OggOpusMetadata::HandlesFilesWithExtension(CFStringRef extension)
{
	if(nullptr == extension)
		return false;
	
	if(kCFCompareEqualTo == CFStringCompare(extension, CFSTR("opus"), kCFCompareCaseInsensitive))
		return true;
	
	return false;
}

bool OggOpusMetadata::HandlesMIMEType(CFStringRef mimeType)
{
	if(nullptr == mimeType)
		return false;
	
	if(kCFCompareEqualTo == CFStringCompare(mimeType, CFSTR("audio/opus"), kCFCompareCaseInsensitive))
		return true;
	
	return false;
}

AudioMetadata * OggOpusMetadata::CreateMetadata(CFURLRef url)
{
	return new OggOpusMetadata(url);
}

#pragma mark Creation and Destruction

OggOpusMetadata::OggOpusMetadata(CFURLRef url)
	: AudioMetadata(url)
{}

OggOpusMetadata::~OggOpusMetadata()
{}

#pragma mark Functionality

bool OggOpusMetadata::ReadMetadata(CFErrorRef *error)
{
	ClearAllMetadata();

	UInt8 buf [PATH_MAX];
	if(!CFURLGetFileSystemRepresentation(mURL, false, buf, PATH_MAX))
		return false;
	
	std::unique_ptr<TagLib::FileStream> stream(new TagLib::FileStream((const char *)buf, true));
	if(!stream->isOpen()) {
		if(error) {
			SFB::CFString description = CFCopyLocalizedString(CFSTR("The file “%@” could not be opened for reading."), "");
			SFB::CFString failureReason = CFCopyLocalizedString(CFSTR("Input/output error"), "");
			SFB::CFString recoverySuggestion = CFCopyLocalizedString(CFSTR("The file may have been renamed, moved, deleted, or you may not have appropriate permissions."), "");

			*error = CreateErrorForURL(AudioMetadataErrorDomain, AudioMetadataInputOutputError, description, mURL, failureReason, recoverySuggestion);
		}

		return false;
	}

	TagLib::Ogg::Opus::File file(stream.get());
	if(!file.isValid()) {
		if(nullptr != error) {
			SFB::CFString description = CFCopyLocalizedString(CFSTR("The file “%@” is not a valid Ogg Opus file."), "");
			SFB::CFString failureReason = CFCopyLocalizedString(CFSTR("Not an Ogg Opus file"), "");
			SFB::CFString recoverySuggestion = CFCopyLocalizedString(CFSTR("The file's extension may not match the file's type."), "");
			
			*error = CreateErrorForURL(AudioMetadataErrorDomain, AudioMetadataInputOutputError, description, mURL, failureReason, recoverySuggestion);
		}
		
		return false;
	}
	
	CFDictionarySetValue(mMetadata, kPropertiesFormatNameKey, CFSTR("Ogg Opus"));
	
	if(file.audioProperties())
		AddAudioPropertiesToDictionary(mMetadata, file.audioProperties());
	
	if(file.tag())
		AddXiphCommentToDictionary(mMetadata, mPictures, file.tag());

	return true;
}

bool OggOpusMetadata::WriteMetadata(CFErrorRef *error)
{
	UInt8 buf [PATH_MAX];
	if(!CFURLGetFileSystemRepresentation(mURL, false, buf, PATH_MAX))
		return false;
	
	std::unique_ptr<TagLib::FileStream> stream(new TagLib::FileStream((const char *)buf));
	if(!stream->isOpen()) {
		if(error) {
			SFB::CFString description = CFCopyLocalizedString(CFSTR("The file “%@” could not be opened for writing."), "");
			SFB::CFString failureReason = CFCopyLocalizedString(CFSTR("Input/output error"), "");
			SFB::CFString recoverySuggestion = CFCopyLocalizedString(CFSTR("The file may have been renamed, moved, deleted, or you may not have appropriate permissions."), "");

			*error = CreateErrorForURL(AudioMetadataErrorDomain, AudioMetadataInputOutputError, description, mURL, failureReason, recoverySuggestion);
		}

		return false;
	}

	TagLib::Ogg::Opus::File file(stream.get(), false);
	if(!file.isValid()) {
		if(error) {
			SFB::CFString description = CFCopyLocalizedString(CFSTR("The file “%@” is not a valid Ogg Opus file."), "");
			SFB::CFString failureReason = CFCopyLocalizedString(CFSTR("Not an Ogg Opus file"), "");
			SFB::CFString recoverySuggestion = CFCopyLocalizedString(CFSTR("The file's extension may not match the file's type."), "");
			
			*error = CreateErrorForURL(AudioMetadataErrorDomain, AudioMetadataInputOutputError, description, mURL, failureReason, recoverySuggestion);
		}
		
		return false;
	}
	
	SetXiphCommentFromMetadata(*this, file.tag());
	
	if(!file.save()) {
		if(error) {
			SFB::CFString description = CFCopyLocalizedString(CFSTR("The file “%@” is not a valid Ogg Opus file."), "");
			SFB::CFString failureReason = CFCopyLocalizedString(CFSTR("Unable to write metadata"), "");
			SFB::CFString recoverySuggestion = CFCopyLocalizedString(CFSTR("The file's extension may not match the file's type."), "");
			
			*error = CreateErrorForURL(AudioMetadataErrorDomain, AudioMetadataInputOutputError, description, mURL, failureReason, recoverySuggestion);
		}
		
		return false;
	}
	
	MergeChangedMetadataIntoMetadata();
	
	return true;
}
