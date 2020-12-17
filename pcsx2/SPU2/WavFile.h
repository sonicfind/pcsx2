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

// Note the file is mostly a copy paste of the WavFile.h from SoundTouch library. It was
// shrunken to support only output 16 bits wav files

#ifndef WAVFILE_H
#define WAVFILE_H

#include "Global.h"

#ifndef uint
typedef unsigned int uint;
#endif


/// WAV audio file 'riff' section header
typedef struct
{
	char riff_char[4];
	int package_len;
	char wave[4];
} WavRiff;

/// WAV audio file 'format' section header
typedef struct
{
	char fmt[4];
	int format_len;
	short fixed;
	short channel_number;
	int sample_rate;
	int byte_rate;
	short bytes_per_sample;
	short bits_per_sample;
} WavFormat;

/// WAV audio file 'data' section header
typedef struct
{
	char data_field[4];
	uint data_len;
} WavData;


/// WAV audio file header
typedef struct
{
	WavRiff riff;
	WavFormat format;
	WavData data;
} WavHeader;


/// Class for writing WAV audio files.
class WavFile
{
private:
	/// Pointer to the WAV file
	FILE* fptr;

	/// WAV file header data.
	WavHeader header;

	/// Fills in WAV file header information.
	void fillInHeader(const uint bits, const uint channels);

	/// Finishes the WAV file header by supplementing information of amount of
	/// data written to file etc
	void finishHeader();

	/// Writes the WAV file header.
	void writeHeader();

public:
	/// Constructor: Creates a new WAV file. Throws a 'runtime_error' exception
	/// if file creation fails.
	WavFile(const char* fileName, ///< Filename
			   int bits,             ///< Bits per sample (16, 24 or 16 bits)
			   int channels          ///< Number of channels (1=mono, 2=stereo)
	);

	/// Destructor: Finalizes & closes the WAV file.
	~WavFile();
	
	/// Write data to WAV file. Throws a 'runtime_error' exception if writing to
	/// file fails.
	void write(const StereoOut16& samples);

	/// Write data to WAV file. Throws a 'runtime_error' exception if writing to
	/// file fails.
	///
	/// Use the short* overloaded function for core dumping
	void write(StereoOut32 samples);
};

#endif
