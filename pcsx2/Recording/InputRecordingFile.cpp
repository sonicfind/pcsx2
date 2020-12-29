/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#ifndef DISABLE_RECORDING

#include "DebugTools/Debug.h"
#include "MainFrame.h"
#include "MemoryTypes.h"

#include "InputRecordingFile.h"
#include "Utilities/InputRecordingLogger.h"

void InputRecordingFileHeader::Init()
{
	memset(author, 0, ArraySize(author));
	memset(gameName, 0, ArraySize(gameName));
}

void InputRecordingFileHeader::SetEmulatorVersion()
{
	wxString emuVersion = wxString::Format("%s-%d.%d.%d", pxGetAppName().c_str(), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo);
	int max = ArraySize(emu) - 1;
	strncpy(emu, emuVersion.c_str(), max);
	emu[max] = 0;
}

void InputRecordingFileHeader::SetAuthor(wxString _author)
{
	int max = ArraySize(author) - 1;
	strncpy(author, _author.c_str(), max);
	author[max] = 0;
}

void InputRecordingFileHeader::SetGameName(wxString _gameName)
{
	int max = ArraySize(gameName) - 1;
	strncpy(gameName, _gameName.c_str(), max);
	gameName[max] = 0;
}

bool InputRecordingFile::Close()
{
	if (recordingFile == nullptr)
		return false;
	fclose(recordingFile);
	recordingFile = nullptr;
	filename = "";
	return true;
}

const wxString &InputRecordingFile::GetFilename()
{
	return filename;
}

InputRecordingFileHeader &InputRecordingFile::GetHeader()
{
	return header;
}

long &InputRecordingFile::GetTotalFrames()
{
	return totalFrames;
}

unsigned long &InputRecordingFile::GetUndoCount()
{
	return undoCount;
}

bool InputRecordingFile::FromSaveState()
{
	return savestate.fromSavestate;
}

void InputRecordingFile::IncrementUndoCount()
{
	if (fseek(recordingFile, seekpointUndoCount, SEEK_SET) == 0)
	{
		undoCount++;
		fwrite(&undoCount, 4, 1, recordingFile);
	}
}

bool InputRecordingFile::open(const wxString path, bool newRecording)
{
	if (newRecording)
	{
		if ((recordingFile = wxFopen(path, L"wb+")) != nullptr)
		{
			filename = path;
			totalFrames = 0;
			undoCount = 0;
			header.Init();
			return true;
		}
	}
	else if ((recordingFile = wxFopen(path, L"rb+")) != nullptr)
	{
		if (verifyRecordingFileHeader())
		{
			filename = path;
			return true;
		}
		Close();
		inputRec::consoleLog("Input recording file header is invalid");
		return false;
	}
	inputRec::consoleLog(fmt::format("Input recording file opening failed. Error - {}", strerror(errno)));
	return false;
}

bool InputRecordingFile::OpenNew(const wxString& path, bool fromSavestate)
{
	if (!open(path, true))
		return false;
	savestate.fromSavestate = fromSavestate;
	return true;
}

bool InputRecordingFile::OpenExisting(const wxString& path)
{
	return open(path, false);
}

bool InputRecordingFile::ReadKeyBuffer(u8 &result, const uint &frame, const uint port, const uint bufIndex)
{
	const long seek = getRecordingBlockSeekPoint(frame) + controllerInputBytes * port + bufIndex;
	return fseek(recordingFile, seek, SEEK_SET) == 0 && fread(&result, 1, 1, recordingFile) == 1;
}

bool InputRecordingFile::SetTotalFrames(long frame)
{
	if (totalFrames < frame)
	{
		totalFrames = frame;
		if (fseek(recordingFile, seekpointTotalFrames, SEEK_SET) == 0)
		{
			fwrite(&totalFrames, 4, 1, recordingFile);
			fflush(recordingFile);
		}
		return true;
	}
	else
	{
		fflush(recordingFile);
		return totalFrames == frame;
	}
}

bool InputRecordingFile::WriteHeader()
{
	rewind(recordingFile);
	return fwrite(&header, sizeof(InputRecordingFileHeader), 1, recordingFile) == 1 &&
		fwrite(&totalFrames, 4, 1, recordingFile) == 1 &&
		fwrite(&undoCount, 4, 1, recordingFile) == 1 &&
		fwrite(&savestate, 1, 1, recordingFile) == 1;
}

bool InputRecordingFile::WriteKeyBuffer(const uint &frame, const uint port, const uint bufIndex, const u8 &buf)
{
	const long seek = getRecordingBlockSeekPoint(frame) + controllerInputBytes * port + bufIndex;
	return fseek(recordingFile, seek, SEEK_SET) == 0 && fwrite(&buf, 1, 1, recordingFile) == 1;
}

long InputRecordingFile::getRecordingBlockSeekPoint(const long frame)
{
	return headerSize + sizeof(bool) + frame * inputBytesPerFrame;
}

bool InputRecordingFile::verifyRecordingFileHeader()
{
	// Verify header contents
	rewind(recordingFile);
	if (fread(&header, sizeof(InputRecordingFileHeader), 1, recordingFile) != 1
		|| fread(&totalFrames, 4, 1, recordingFile) != 1
		|| fread(&undoCount, 4, 1, recordingFile) != 1
		|| fread(&savestate, sizeof(bool), 1, recordingFile) != 1)
	{
		return false;
	}
	
	// Check for current verison
	if (header.version != 1)
	{
		inputRec::consoleLog(fmt::format("Input recording file is not a supported version - {}", header.version));
		return false;
	}
	return true;
}
#endif
