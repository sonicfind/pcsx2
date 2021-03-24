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

#pragma once

#ifndef DISABLE_RECORDING

#include <queue>

#include "Pcsx2Types.h"
#include "wx/bitmap.h"
#include "wx/checkbox.h"
#include "wx/gdicmn.h"
#include "wx/slider.h"
#include "wx/spinctrl.h"
#include "wx/dcbuffer.h"

struct ImageFile
{
	wxBitmap image;
	wxPoint coords;
};

struct VirtualPadElement
{
	bool m_currentlyRendered = false;

	wxCommandEvent ConstructEvent(wxEventTypeTag<wxCommandEvent> eventType, wxWindow *obj);
	wxCommandEvent ConstructEvent(wxEventTypeTag<wxSpinEvent> eventType, wxWindow *obj);

	virtual void EnableWidgets(bool enable) = 0;
	virtual void Render(wxDC& dc) = 0;
	virtual void Reset(wxEvtHandler* destWindow) = 0;
	virtual void UpdateGuiElement(std::queue<VirtualPadElement*>& renderQueue, bool& clearScreenRequired) = 0;
};

struct ControllerButton
{
	bool m_isControllerPressBypassed = false;
	bool m_pressed = false;
	bool m_prevPressedVal = false;
	bool m_widgetUpdateRequired = false;

	bool UpdateButtonData(bool& padDataVal, bool ignoreRealController, bool readOnly) noexcept;
};

struct ControllerNormalButton : public ControllerButton, public VirtualPadElement
{
	ImageFile m_icon;
	wxCheckBox* m_pressedBox = 0;

	bool UpdateData(bool& padDataVal, bool ignoreRealController, bool readOnly);
	void EnableWidgets(bool enable) override;
	void Render(wxDC& dc) override;
	void Reset(wxEvtHandler* destWindow) override;
	void UpdateGuiElement(std::queue<VirtualPadElement*>& renderQueue, bool& clearScreenRequired) override;
};

struct ControllerPressureButton : public ControllerButton, public VirtualPadElement
{
	ImageFile m_icon;
	wxSpinCtrl* m_pressureSpinner = 0;

	u8 m_pressure = 0;

	bool m_isControllerPressureBypassed = false;
	u8 m_prevPressureVal = 0;

	bool UpdateData(bool& padDataVal, bool ignoreRealController, bool readOnly);
	bool UpdateData(u8& padDataVal, bool ignoreRealController, bool readOnly) noexcept;
	void EnableWidgets(bool enable) override;
	void Render(wxDC& dc) override;
	void Reset(wxEvtHandler* destWindow) override;
	void UpdateGuiElement(std::queue<VirtualPadElement*>& renderQueue, bool& clearScreenRequired) override;
};

struct AnalogVector
{
	wxSlider* m_slider = 0;
	wxSpinCtrl* m_spinner = 0;

	u8 m_val = 127;

	bool m_isControllerBypassed = false;
	bool m_widgetUpdateRequired = false;
	u8 m_prevVal = 127;

	bool UpdateData(u8& padDataVal, bool ignoreRealController, bool readOnly) noexcept;
};

class AnalogStick : public VirtualPadElement
{
	struct AnalogPosition
	{
		wxPoint centerCoords;
		wxPoint endCoords;

		int lineThickness = 0;
		int radius = 0;
	};
public:
	AnalogPosition m_positionGraphic;
	AnalogVector m_xVector;
	AnalogVector m_yVector;

	void EnableWidgets(bool enable) override;
	void Render(wxDC& dc) override;
	void Reset(wxEvtHandler* destWindow) override;
	void UpdateGuiElement(std::queue<VirtualPadElement*>& renderQueue, bool& clearScreenRequired) override;
};
#endif
