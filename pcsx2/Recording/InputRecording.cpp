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

#include "AppSaveStates.h"
#include "Counters.h"

#ifndef DISABLE_RECORDING

#include "AppGameDatabase.h"
#include "DebugTools/Debug.h"

#include "InputRecording.h"
#include "InputRecordingControls.h"
#include "Utilities/InputRecordingLogger.h"

#endif

void SaveStateBase::InputRecordingFreeze()
{
	// NOTE - BE CAREFUL
	// CHANGING THIS WILL BREAK BACKWARDS COMPATIBILITY ON SAVESTATES
	FreezeTag("InputRecording");
	Freeze(g_FrameCount);

#ifndef DISABLE_RECORDING
	if (g_Conf->EmuOptions.EnableRecordingTools)
	{
		// Loading a savestate is an asynchronous task. If we are playing a recording
		// that starts from a savestate (not power-on) and the starting (pcsx2 internal) frame
		// marker has not been set (which comes from the savestate), we initialize it.
		if (g_InputRecording.IsInitialLoad())
			g_InputRecording.SetupInitialState(g_FrameCount);
		else if (g_InputRecording.IsActive() && IsLoading())
			g_InputRecording.SetFrameCounter(g_FrameCount);
	}
#endif
}

#ifndef DISABLE_RECORDING

InputRecording g_InputRecording;

InputRecording::InputRecordingPad::InputRecordingPad()
	: m_padData(std::make_unique<PadData>()) {}

void InputRecording::InitVirtualPadWindows(wxWindow* parent)
{
	for (int port = 0; port < s_NUM_PORTS; ++port)
			if (!m_pads[port].m_virtualPad)
				m_pads[port].m_virtualPad = new VirtualPad(parent, port, g_Conf->inputRecording);
}

void InputRecording::ShowVirtualPad(const int port)
{
	m_pads[port].m_virtualPad->Show();
}

void InputRecording::RecordingReset()
{
	// Booting is an asynchronous task. If we are playing a recording
	// that starts from power-on and the starting (pcsx2 internal) frame
	// marker has not been set, we initialize it.
	if (m_initialLoad)
		SetupInitialState(0);
	else if (IsActive())
	{
		SetFrameCounter(0);
		g_InputRecordingControls.Lock(0);
	}
	else
		g_InputRecordingControls.Resume();
}

void InputRecording::ControllerInterrupt(const u8 data, const u8 port, const u16 bufCount, u8& bufVal)
{
	// TODO - Multi-Tap Support

	const InputRecordingPad& pad = m_pads[port];

	if (bufCount == 1)
		m_fInterruptFrame = data == READ_DATA_AND_VIBRATE_FIRST_BYTE;
	else if (bufCount == 2)
	{
		if (bufVal != READ_DATA_AND_VIBRATE_SECOND_BYTE)
			m_fInterruptFrame = false;
	}
	else if (m_fInterruptFrame)
	{
		const u16 bufIndex = bufCount - 3;
		if (m_state == InputRecordingMode::Replaying)
		{
			if (m_frameCounter >= 0 && m_frameCounter < INT_MAX)
			{
				if (!m_inputRecordingData.ReadKeyBuffer(bufVal, m_frameCounter, port, bufIndex))
					inputRec::consoleLog(fmt::format("Failed to read input data at frame {}", m_frameCounter));

				// Update controller data state for future VirtualPad / logging usage.
				pad.m_padData->UpdateControllerData(bufIndex, bufVal);
				
				if (pad.m_virtualPad->IsShown())
					pad.m_virtualPad->UpdateControllerData(bufIndex, pad.m_padData.get());
			}
		}
		else
		{
			// Update controller data state for future VirtualPad / logging usage.
			pad.m_padData->UpdateControllerData(bufIndex, bufVal);

			// Commit the byte to the movie file if we are recording
			if (m_state == InputRecordingMode::Recording)
			{
				if (m_frameCounter >= 0)
				{
					// If the VirtualPad updated the PadData, we have to update the buffer
					// before committing it to the recording / sending it to the game
					if (pad.m_virtualPad->IsShown() && pad.m_virtualPad->UpdateControllerData(bufIndex, pad.m_padData.get()))
						bufVal = pad.m_padData->PollControllerData(bufIndex);

					if (m_incrementRedo)
					{
						m_inputRecordingData.IncrementRedoCount();
						m_incrementRedo = false;
					}

					if (m_frameCounter < INT_MAX && !m_inputRecordingData.WriteKeyBuffer(bufVal, m_frameCounter, port, bufIndex))
						inputRec::consoleLog(fmt::format("Failed to write input data at frame {}", m_frameCounter));
				}
			}
			// If the VirtualPad updated the PadData, we have to update the buffer
			// before sending it to the game
			else if (pad.m_virtualPad->IsShown() && pad.m_virtualPad->UpdateControllerData(bufIndex, pad.m_padData.get()))
				bufVal = pad.m_padData->PollControllerData(bufIndex);
		}
	}
}

s32 InputRecording::GetFrameCounter()
{
	return m_frameCounter;
}

InputRecordingFile& InputRecording::GetInputRecordingData()
{
	return m_inputRecordingData;
}

u32 InputRecording::GetStartingFrame()
{
	return m_startingFrame;
}

void InputRecording::IncrementFrameCounter()
{
	if (m_frameCounter < INT_MAX)
	{
		m_frameCounter++;
		if (m_frameCounter > m_inputRecordingData.GetTotalFrames())
		{
			if (IsRecording())
				m_inputRecordingData.SetTotalFrames(m_frameCounter);
			m_incrementRedo = false;
		}
		else if (m_frameCounter == m_inputRecordingData.GetTotalFrames())
			m_incrementRedo = false;
	}
}

void InputRecording::LogAndRedraw()
{
	for (u8 port = 0; port < s_NUM_PORTS; ++port)
	{
		InputRecordingPad& pad = m_pads[port];
		pad.m_padData->LogPadData(port);
		// As well as re-render the virtual pad UI, if applicable
		// - Don't render if it's minimized
		if (pad.m_virtualPad->IsShown() && !pad.m_virtualPad->IsIconized())
			pad.m_virtualPad->Redraw();
	}
}

bool InputRecording::IsInterruptFrame()
{
	return m_fInterruptFrame;
}

bool InputRecording::IsActive()
{
	return m_state != InputRecordingMode::NotActive;
}

bool InputRecording::IsInitialLoad()
{
	return m_initialLoad;
}

bool InputRecording::IsReplaying()
{
	return m_state == InputRecordingMode::Replaying;
}

bool InputRecording::IsRecording()
{
	return m_state == InputRecordingMode::Recording;
}

wxString InputRecording::RecordingModeTitleSegment()
{
	switch (m_state)
	{
		case InputRecordingMode::Recording:
			return wxString("Recording");
		case InputRecordingMode::Replaying:
			return wxString("Replaying");
		default:
			return wxString("No Movie");
	}
}

void InputRecording::SetToRecordMode(const bool log)
{
	m_state = InputRecordingMode::Recording;
	// Set active VirtualPads to record mode
	for (u8 port = 0; port < s_NUM_PORTS; port++)
		m_pads[port].m_virtualPad->SetReadOnlyMode(false);

	if (log)
		inputRec::log("Record mode ON");
}

void InputRecording::SetToReplayMode(const bool log)
{
	m_state = InputRecordingMode::Replaying;
	// Set active VirtualPads to record mode
	for (u8 port = 0; port < s_NUM_PORTS; port++)
		m_pads[port].m_virtualPad->SetReadOnlyMode(true);

	if (log)
		inputRec::log("Replay mode ON");
}

void InputRecording::SetFrameCounter(u32 newGFrameCount)
{
	if (newGFrameCount > m_startingFrame + static_cast<u32>(m_inputRecordingData.GetTotalFrames()))
	{
		inputRec::consoleLog("Warning, you've loaded PCSX2 emulation to a point after the end of the original recording. This should be avoided.");
		inputRec::consoleLog("Savestate's framecount has been ignored.");
		m_frameCounter = m_inputRecordingData.GetTotalFrames();
		if (m_state == InputRecordingMode::Replaying)
			SetToRecordMode();
		m_incrementRedo = false;
	}
	else
	{
		if (newGFrameCount < m_startingFrame)
		{
			inputRec::consoleLog("Warning, you've loaded PCSX2 emulation to a point before the start of the original recording. This should be avoided.");
			if (m_state == InputRecordingMode::Recording)
				SetToReplayMode();
		}
		else if (newGFrameCount == 0 && m_state == InputRecordingMode::Recording)
			SetToReplayMode();
		m_frameCounter = newGFrameCount - static_cast<s32>(m_startingFrame);
		m_incrementRedo = true;
	}
}

void InputRecording::SetupInitialState(u32 newStartingFrame)
{
	m_startingFrame = newStartingFrame;
	if (m_state != InputRecordingMode::Replaying)
	{
		inputRec::log("Started new input recording");
		inputRec::consoleLog(fmt::format("Filename {}", std::string(m_inputRecordingData.GetFilename())));
		SetToRecordMode(false);
	}
	else
	{
		// Check if the current game matches with the one used to make the original recording
		if (!g_Conf->CurrentIso.IsEmpty())
			if (resolveGameName() != m_inputRecordingData.GetGameName())
				inputRec::consoleLog("Input recording was possibly constructed for a different game.");

		m_incrementRedo = true;
		inputRec::log("Replaying input recording");
		inputRec::consoleMultiLog({fmt::format("File: {}", std::string(m_inputRecordingData.GetFilename())),
								   fmt::format("PCSX2 Version Used: {}", std::string(m_inputRecordingData.GetEmulatorVersion())),
								   fmt::format("Recording File Version: {}", m_inputRecordingData.GetFileVersion()),
								   fmt::format("Associated Game Name or ISO Filename: {}", std::string(m_inputRecordingData.GetGameName())),
								   fmt::format("Author: {}", m_inputRecordingData.GetAuthor()),
								   fmt::format("Total Frames: {}", m_inputRecordingData.GetTotalFrames()),
								   fmt::format("Undo Count: {}", m_inputRecordingData.GetRedoCount())});
		SetToReplayMode(false);
	}

	if (m_inputRecordingData.FromSavestate())
		inputRec::consoleLog(fmt::format("Internal Starting Frame: {}", m_startingFrame));
	m_frameCounter = 0;
	m_initialLoad = false;
	g_InputRecordingControls.Lock(m_startingFrame);
}

void InputRecording::FailedSavestate()
{
	inputRec::consoleLog(fmt::format("{} is not compatible with this version of PCSX2", m_savestate));
	inputRec::consoleLog(fmt::format("Original PCSX2 version used: {}", m_inputRecordingData.GetEmulatorVersion()));
	m_inputRecordingData.Close();
	m_initialLoad = false;
	m_state = InputRecordingMode::NotActive;
	g_InputRecordingControls.Resume();
}

void InputRecording::Stop()
{
	m_state = InputRecordingMode::NotActive;
	m_incrementRedo = false;
	for (u8 port = 0; port < s_NUM_PORTS; port++)
		m_pads[port].m_virtualPad->SetReadOnlyMode(false);

	inputRec::log("Input recording stopped");
}


bool InputRecording::Create(wxString fileName, const bool fromSavestate, wxString authorName)
{
	if (!m_inputRecordingData.OpenNew(fileName, fromSavestate))
		return false;

	m_initialLoad = true;
	m_state = InputRecordingMode::Recording;
	if (m_inputRecordingData.FromSavestate())
	{
		m_savestate = fileName + "_SaveState.p2s";
		if (wxFileExists(m_savestate))
			wxCopyFile(m_savestate, m_savestate + ".bak", true);
		StateCopy_SaveToFile(m_savestate);
	}
	else
		sApp.SysExecute(g_Conf->CdvdSource);

	// Set emulator version
	m_inputRecordingData.GetHeader().SetEmulatorVersion();

	// Set author name
	if (!authorName.IsEmpty())
		m_inputRecordingData.GetHeader().SetAuthor(authorName);

	// Set Game Name
	m_inputRecordingData.GetHeader().SetGameName(resolveGameName());
	// Write header contents
	m_inputRecordingData.WriteHeader();
	return true;
}

bool InputRecording::Play(wxWindow* parent, wxString filename)
{
	if (!m_inputRecordingData.OpenExisting(filename))
		return false;

	// Either load the savestate, or restart the game
	if (m_inputRecordingData.FromSavestate())
	{
		if (CoreThread.IsClosed())
		{
			inputRec::consoleLog("Game is not open, aborting playing input recording which starts on a savestate.");
			m_inputRecordingData.Close();
			return false;
		}

		m_savestate = m_inputRecordingData.GetFilename() + "_SaveState.p2s";
		if (!wxFileExists(m_savestate))
		{
			wxFileDialog loadStateDialog(parent, _("Select the savestate that will accompany this recording"), L"", L"",
										 L"Savestate files (*.p2s)|*.p2s", wxFD_OPEN);
			if (loadStateDialog.ShowModal() == wxID_CANCEL)
			{
				inputRec::consoleLog(fmt::format("Could not locate savestate file at location - {}", m_savestate));
				inputRec::log("Savestate load failed");
				m_inputRecordingData.Close();
				return false;
			}

			m_savestate = loadStateDialog.GetPath();
			inputRec::consoleLog(fmt::format("Base savestate set to {}", m_savestate));
		}
		m_state = InputRecordingMode::Replaying;
		m_initialLoad = true;
		StateCopy_LoadFromFile(m_savestate);
	}
	else
	{
		m_state = InputRecordingMode::Replaying;
		m_initialLoad = true;
		sApp.SysExecute(g_Conf->CdvdSource);
	}
	
	return true;
}

void InputRecording::GoToFirstFrame(wxWindow* parent)
{
	if (m_inputRecordingData.FromSavestate())
	{
		if (!wxFileExists(m_savestate))
		{
			const bool initiallyPaused = g_InputRecordingControls.IsPaused();

			if (!initiallyPaused)
				g_InputRecordingControls.PauseImmediately();

			inputRec::consoleLog(fmt::format("Could not locate savestate file at location - {}\n", m_savestate));
			wxFileDialog loadStateDialog(parent, _("Select a savestate to accompany the recording with"), L"", L"",
										 L"Savestate files (*.p2s)|*.p2s", wxFD_OPEN);
			int result = loadStateDialog.ShowModal();
			if (!initiallyPaused)
				g_InputRecordingControls.Resume();

			if (result == wxID_CANCEL)
			{
				inputRec::log("Savestate load cancelled");
				return;
			}
			m_savestate = loadStateDialog.GetPath();
			inputRec::consoleLog(fmt::format ("Base savestate swapped to {}", m_savestate));
		}
		StateCopy_LoadFromFile(m_savestate);
	}
	else
		sApp.SysExecute(g_Conf->CdvdSource);

	if (IsRecording())
		SetToReplayMode();
}

wxString InputRecording::resolveGameName()
{
	// Code loosely taken from AppCoreThread::_ApplySettings to resolve the Game Name
	wxString gameName;
	const wxString gameKey(SysGetDiscID());
	if (!gameKey.IsEmpty())
	{
		if (IGameDatabase* gameDB = AppHost_GetGameDatabase())
		{
			GameDatabaseSchema::GameEntry game = gameDB->findGame(std::string(gameKey));
			if (game.isValid)
			{
				gameName = game.name;
				gameName += L" (" + game.region + L")";
			}
		}
	}
	return !gameName.IsEmpty() ? gameName : (wxString)Path::GetFilename(g_Conf->CurrentIso);
}

#endif
