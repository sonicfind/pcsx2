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

// Note the file is mostly an altered copy paste of the WavFile.h from SoundTouch library. It was
// shrunken to only handle simple wav file creation to 16bit, 24bit (new addition), and 32bit.

#include "PrecompiledHeader.h"
#include "WavFile.h"

static const char riffStr[] = "RIFF";
static const char waveStr[] = "WAVE";
static const char fmtStr[] = "fmt ";
static const char dataStr[] = "data";

//////////////////////////////////////////////////////////////////////////////
//
// Class WavFile
//

WavFile::WavFile(const char* fileName, int bits, int channels)
{
	fptr = fopen(fileName, "wb");
	if (fptr == nullptr)
	{
		std::string msg = "Error : Unable to open file \"";
		msg += fileName;
		msg += "\" for writing.";
		//pmsg = msg.c_str;
		throw std::runtime_error(msg);
	}

	fillInHeader(bits, channels);
	writeHeader();
}

WavFile::~WavFile()
{
	if (fptr)
	{
		finishHeader();
		fclose(fptr);
	}
}

void WavFile::fillInHeader(uint bits, uint channels)
{
	// fill in the 'riff' part..

	// copy string 'RIFF' to riff_char
	memcpy(&(header.riff.riff_char), riffStr, 4);
	// package_len unknown so far
	header.riff.package_len = 0;
	// copy string 'WAVE' to wave
	memcpy(&(header.riff.wave), waveStr, 4);


	// fill in the 'format' part..

	// copy string 'fmt ' to fmt
	memcpy(&(header.format.fmt), fmtStr, 4);

	header.format.format_len = 0x10;
	header.format.fixed = 1;
	header.format.channel_number = (short)channels;
	header.format.sample_rate = 48000;
	header.format.bits_per_sample = (short)bits;
	header.format.bytes_per_sample = (short)((bits * channels) >> 3);
	header.format.byte_rate = 48000 * header.format.bytes_per_sample;

	// fill in the 'data' part..

	// copy string 'data' to data_field
	memcpy(&(header.data.data_field), dataStr, 4);
	// data_len unknown so far
	header.data.data_len = 0;
}

void WavFile::finishHeader()
{
	// supplement the file length into the header structure
	header.data.data_len = (uint)ftell(fptr) - sizeof(WavHeader);
	if (header.data.data_len & 1)
		fputc(0, fptr);

	header.riff.package_len = header.data.data_len + 36;
	writeHeader();
}

void WavFile::writeHeader()
{
	// write the supplemented header in the beginning of the file
	fseek(fptr, 0, SEEK_SET);
	if (fwrite(&header, sizeof(header), 1, fptr) != 1)
	{
		throw std::runtime_error("Error while writing to a wav file.");
	}

	// jump back to the end of the file
	fseek(fptr, 0, SEEK_END);
}

void WavFile::write(const StereoOut16& samples)
{
	if (fwrite(&samples, 2, 2, fptr) != 2)
	{
		throw std::runtime_error("Error while writing to a wav file.");
	}
}

void WavFile::write(StereoOut32 samples)
{
	int res = 0;
	
	switch (header.format.bits_per_sample)
	{
	case 16:
	{
		StereoOut16 temp = samples.DownSample();
		if (header.format.channel_number == 1)
		{
			s16 mono = (temp.Left >> 1) + (temp.Right >> 1);
			res = fwrite(&mono, 2, 1, fptr);
		}
		else
			res = fwrite(&temp, 2, 2, fptr);
		break;
	}
	case 24:
	{
		
		if (header.format.channel_number == 1)
		{
			s32 temp = (samples.Left >> 5) + (samples.Right >> 5);
			res = fwrite(&temp, 3, 1, fptr);
		}
		else
		{
			samples.Left >>= 4;
			samples.Right >>= 4;
			res = fwrite(&samples.Left, 3, 1, fptr);
			res += fwrite(&samples.Right, 3, 1, fptr);
		}
		break;
	}
	case 32:
	{
		if (header.format.channel_number == 1)
		{
			s32 temp = (samples.Left << 3) + (samples.Right << 3);
			res = fwrite(&temp, 4, 1, fptr);
		}
		else
		{
			samples.Left <<= 4;
			samples.Right <<= 4;
			res = fwrite(&samples, 4, 2, fptr);
		}
		break;
	}
	}

	if (res != header.format.channel_number)
	{
		throw std::runtime_error("Error while writing to a wav file.");
	}
}
