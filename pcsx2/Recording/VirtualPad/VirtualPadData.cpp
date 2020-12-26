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

#include "Recording/VirtualPad/VirtualPadData.h"

bool VirtualPadData::UpdateVirtualPadData(u16 bufIndex, PadData* padData, bool ignoreRealController, bool readOnly)
{
	bool changeDetected = false;
	PadData::BufferIndex index = static_cast<PadData::BufferIndex>(bufIndex);
	switch (index)
	{
		case PadData::BufferIndex::PressedFlagsGroupOne:
			changeDetected |= left.UpdateData(padData->m_leftPressed, ignoreRealController, readOnly);
			changeDetected |= down.UpdateData(padData->m_downPressed, ignoreRealController, readOnly);
			changeDetected |= right.UpdateData(padData->m_rightPressed, ignoreRealController, readOnly);
			changeDetected |= up.UpdateData(padData->m_upPressed, ignoreRealController, readOnly);
			changeDetected |= start.UpdateData(padData->m_start, ignoreRealController, readOnly);
			changeDetected |= r3.UpdateData(padData->m_r3, ignoreRealController, readOnly);
			changeDetected |= l3.UpdateData(padData->m_l3, ignoreRealController, readOnly);
			changeDetected |= select.UpdateData(padData->m_select, ignoreRealController, readOnly);
			return changeDetected;
		case PadData::BufferIndex::PressedFlagsGroupTwo:
			changeDetected |= square.UpdateData(padData->m_squarePressed, ignoreRealController, readOnly);
			changeDetected |= cross.UpdateData(padData->m_crossPressed, ignoreRealController, readOnly);
			changeDetected |= circle.UpdateData(padData->m_circlePressed, ignoreRealController, readOnly);
			changeDetected |= triangle.UpdateData(padData->m_trianglePressed, ignoreRealController, readOnly);
			changeDetected |= r1.UpdateData(padData->m_r1Pressed, ignoreRealController, readOnly);
			changeDetected |= l1.UpdateData(padData->m_l1Pressed, ignoreRealController, readOnly);
			changeDetected |= r2.UpdateData(padData->m_r2Pressed, ignoreRealController, readOnly);
			changeDetected |= l2.UpdateData(padData->m_l2Pressed, ignoreRealController, readOnly);
			return changeDetected;
		case PadData::BufferIndex::RightAnalogXVector:
			return rightAnalog.xVector.UpdateData(padData->m_rightAnalogX, ignoreRealController, readOnly);
		case PadData::BufferIndex::RightAnalogYVector:
			return rightAnalog.yVector.UpdateData(padData->m_rightAnalogY, ignoreRealController, readOnly);
		case PadData::BufferIndex::LeftAnalogXVector:
			return leftAnalog.xVector.UpdateData(padData->m_leftAnalogX, ignoreRealController, readOnly);
		case PadData::BufferIndex::LeftAnalogYVector:
			return leftAnalog.yVector.UpdateData(padData->m_leftAnalogY, ignoreRealController, readOnly);
		case PadData::BufferIndex::RightPressure:
			return right.UpdateData(padData->m_rightPressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::LeftPressure:
			return left.UpdateData(padData->m_leftPressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::UpPressure:
			return up.UpdateData(padData->m_upPressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::DownPressure:
			return down.UpdateData(padData->m_downPressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::TrianglePressure:
			return triangle.UpdateData(padData->m_trianglePressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::CirclePressure:
			return circle.UpdateData(padData->m_circlePressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::CrossPressure:
			return cross.UpdateData(padData->m_crossPressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::SquarePressure:
			return square.UpdateData(padData->m_squarePressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::L1Pressure:
			return l1.UpdateData(padData->m_l1Pressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::R1Pressure:
			return r1.UpdateData(padData->m_r1Pressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::L2Pressure:
			return l2.UpdateData(padData->m_l2Pressure, ignoreRealController, readOnly);
		case PadData::BufferIndex::R2Pressure:
			return r2.UpdateData(padData->m_r2Pressure, ignoreRealController, readOnly);
	}
	return changeDetected;
}

#endif
