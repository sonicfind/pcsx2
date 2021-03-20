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

void InputRecordingFile::InputRecordingFileHeader::Init() noexcept
{
	m_fileVersion = 1;
	m_totalFrames = 0;
	m_redoCount = 0;
}

bool InputRecordingFile::InputRecordingFileHeader::ReadHeader(FILE* recordingFile)
{
	return fread(this, s_seekpointTotalFrames, 1, recordingFile) == 1 &&	// Reads in m_fileVersion, m_emulatorVersion, m_author, & m_gameName
			fread(&m_totalFrames, 9, 1, recordingFile) == 1; 				// Reads in m_totalFrames, m_redoCount, & m_startType
}

void InputRecordingFile::InputRecordingFileHeader::SetEmulatorVersion()
{
	m_emulatorVersion.fill(0);
	strncpy(m_emulatorVersion.data(), pxGetAppName().c_str(), m_emulatorVersion.size() - 1);
}

void InputRecordingFile::InputRecordingFileHeader::SetAuthor(wxString author)
{
	m_author.fill(0);
	strncpy(m_author.data(), author.c_str(), m_author.size() - 1);
}

void InputRecordingFile::InputRecordingFileHeader::SetGameName(wxString gameName)
{
	m_gameName.fill(0);
	strncpy(m_gameName.data(), gameName.c_str(), m_gameName.size() - 1);
}

u8 InputRecordingFile::GetFileVersion() const noexcept
{
	return m_header.m_fileVersion;
}

const char* InputRecordingFile::GetEmulatorVersion() const noexcept
{
	return m_header.m_emulatorVersion.data();
}

const char* InputRecordingFile::GetAuthor() const noexcept
{
	return m_header.m_author.data();
}

const char* InputRecordingFile::GetGameName() const noexcept
{
	return m_header.m_gameName.data();
}

long InputRecordingFile::GetTotalFrames() const noexcept
{
	return m_header.m_totalFrames;
}

unsigned long InputRecordingFile::GetRedoCount() const noexcept
{
	return m_header.m_redoCount;
}

bool InputRecordingFile::FromSavestate() const noexcept
{
	return m_header.m_fromSavestate;
}

bool InputRecordingFile::Close()
{
	if (m_recordingFile == nullptr)
		return false;
	fclose(m_recordingFile);
	m_recordingFile = nullptr;
	m_filename = "";
	return true;
}

const wxString& InputRecordingFile::GetFilename() const noexcept
{
	return m_filename;
}

InputRecordingFile::InputRecordingFileHeader& InputRecordingFile::GetHeader() noexcept
{
	return m_header;
}

void InputRecordingFile::IncrementRedoCount()
{
	m_header.m_redoCount++;
	if (fseek(m_recordingFile, InputRecordingFileHeader::s_seekpointRedoCount, SEEK_SET) == 0)
		fwrite(&m_header.m_redoCount, 4, 1, m_recordingFile);
}

bool InputRecordingFile::open(const wxString path, const bool newRecording)
{
	if (newRecording)
	{
		if ((m_recordingFile = wxFopen(path, L"wb+")) != nullptr)
		{
			m_filename = path;
			m_header.Init();
			return true;
		}
	}
	else if ((m_recordingFile = wxFopen(path, L"rb+")) != nullptr)
	{
		if (verifyRecordingFileHeader())
		{
			m_filename = path;
			return true;
		}
		Close();
		inputRec::consoleLog("Input recording file header is invalid");
		return false;
	}
	inputRec::consoleLog(fmt::format("Input recording file opening failed. Error - {}", strerror(errno)));
	return false;
}

bool InputRecordingFile::OpenNew(const wxString& path, const bool fromSaveState)
{
	if (!open(path, true))
		return false;
	m_header.m_fromSavestate = fromSaveState;
	return true;
}

bool InputRecordingFile::OpenExisting(const wxString& path)
{
	return open(path, false);
}

bool InputRecordingFile::WriteHeader() const
{
	return fwrite(&m_header, InputRecordingFileHeader::s_seekpointTotalFrames, 1, m_recordingFile) == 1 &&
		   fwrite(&m_header.m_totalFrames, 10, 1, m_recordingFile) == 1; // Writes m_totalFrames, m_redoCount, m_startType, & m_pads
}

void InputRecordingFile::SetTotalFrames(const long frame) noexcept
{
	m_header.m_totalFrames = frame;
	if (fseek(m_recordingFile, InputRecordingFileHeader::s_seekpointTotalFrames, SEEK_SET) == 0)
	{
		fwrite(&m_header.m_totalFrames, 4, 1, m_recordingFile);
		fflush(m_recordingFile);
	}
}

bool InputRecordingFile::ReadKeyBuffer(u8& result, const s32 frame, const uint port, const uint bufIndex) const
{
	const long seek = getRecordingBlockSeekPoint(frame) + port * s_controllerInputBytes + bufIndex;
	return fseek(m_recordingFile, seek, SEEK_SET) == 0 && fread(&result, 1, 1, m_recordingFile) == 1;
}

bool InputRecordingFile::WriteKeyBuffer(const u8 buf, const s32 frame, const uint port, const uint bufIndex) const
{
	const long seek = getRecordingBlockSeekPoint(frame) + port * s_controllerInputBytes + bufIndex;
	return fseek(m_recordingFile, seek, SEEK_SET) == 0 && fwrite(&buf, 1, 1, m_recordingFile) == 1;
}

s64 InputRecordingFile::getRecordingBlockSeekPoint(const s32 frame) const noexcept
{
	return s_seekpointInputData + (s64)frame * s_inputBytesPerFrame;
}

bool InputRecordingFile::verifyRecordingFileHeader()
{
	// Verify header contents
	if (!m_header.ReadHeader(m_recordingFile))
		return false;
	
	// Check for current verison
	if (m_header.m_fileVersion != 1)
	{
		inputRec::consoleLog(fmt::format("Input recording file is not a supported version - {}", m_header.m_fileVersion));
		return false;
	}
	return true;
}
#endif
