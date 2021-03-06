
/* ---------------------------------------------------------------------
// 
// j2k - JPEG 2000 plug-ins for Adobe programs
// Copyright (c) 2002-2016,  Brendan Bolles, http://www.fnordware.com
// Copyright (c) 2016,       Aaron Boxer,    http://grokimagecompression.github.io/grok
// 
// This file is part of j2k.
//
// j2k is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
// -------------------------------------------------------------------*/

#ifndef J2K_CODEC_H
#define J2K_CODEC_H

#include "j2k_io.h"

#include <list>

namespace j2k
{

enum Format
{
	UNKNOWN_FORMAT,
	J2C,
	JP2,
	JPX
};

enum Alpha
{
	NO_ALPHA,
	UNKNOWN_ALPHA,
	PREMULTIPLIED,
	STRAIGHT
};

typedef struct Rational
{
	int num;
	unsigned int den;
	
	Rational(int n = 0, unsigned int d = 1) : num(n), den(d) {}
	
} Rational;

typedef struct Subsampling
{
	int x; // width * subsampling.x == full width
	int y;
	
	Subsampling(int xin = 1, int yin = 1) : x(xin), y(yin) {}
	
} Subsampling;

enum ColorSpace
{
	UNKNOWN_COLOR_SPACE,
	sRGB,
	sLUM,
	sYCC,
	esRGB,
	esYCC,
	ROMM,
	CMYK,
	CIELab,
	iccLUM,
	iccRGB,
	iccANY
};

#define J2K_CODEC_MAX_CHANNELS 4
#define J2K_CODEC_MAX_LUT_ENTRIES 256
#define J2K_CODEC_MAX_LAYERS 50

typedef struct
{
	unsigned char channel[J2K_CODEC_MAX_CHANNELS];
	
} LUTentry;

enum ChannelName
{
	RED = 0,
	GREEN,
	BLUE,
	ALPHA,
	
	CYAN,
	MAGENTA,
	YELLOW,
	BLACK
};

enum CompressionMethod
{
	LOSSLESS,
	SIZE,
	QUALITY,
	CINEMA
};

enum Order
{
	LRCP,
	RLCP,
	RPCL,
	PCRL,
	CPRL
};

enum DCIProfile
{
	DCI_2K,
	DCI_4K
};

typedef struct CompressionSettings
{
	CompressionMethod method;
	size_t fileSize; // in kilobytes (1 kb = 1024 bytes)
	unsigned char quality;
	unsigned char layers;
	Order order;
	DCIProfile dciProfile;
	unsigned short tileSize;
	bool ycc;
	bool reversible;
	
	CompressionSettings() :
		method(LOSSLESS),
		fileSize(50),
		quality(50),
		layers(12),
		order(RPCL),
		dciProfile(DCI_2K),
		tileSize(1024),
		ycc(false),
		reversible(false)
	{
	}
	
} CompressionSettings;


typedef struct FileInfo
{
	unsigned int width;
	unsigned int height;
	unsigned char channels;
	unsigned char depth;
	Subsampling subsampling[J2K_CODEC_MAX_CHANNELS];
	
	Format format;
	Rational pixelAspect;
	float dpi; // dots per inch
	Alpha alpha;
	
	ColorSpace colorSpace;
	void *iccProfile;
	size_t profileLen;
	
	ChannelName channelMap[J2K_CODEC_MAX_CHANNELS];
	ChannelName LUTmap[J2K_CODEC_MAX_CHANNELS];
	
	unsigned int LUTsize;
	LUTentry LUT[J2K_CODEC_MAX_LUT_ENTRIES];
	
	CompressionSettings settings;
	
	FileInfo() :
		width(0),
		height(0),
		channels(0),
		depth(0),
		format(UNKNOWN_FORMAT),
		pixelAspect( Rational(0, 1) ),
		dpi(0),
		alpha(NO_ALPHA),
		colorSpace(UNKNOWN_COLOR_SPACE),
		iccProfile(NULL),
		profileLen(0),
		LUTsize(0)
	{
		channelMap[0] = RED;
		channelMap[1] = GREEN;
		channelMap[2] = BLUE;
		channelMap[3] = ALPHA;
		
		LUTmap[0] = RED;
		LUTmap[1] = GREEN;
		LUTmap[2] = BLUE;
		LUTmap[3] = ALPHA;
	}
	
} FileInfo;


enum SampleType
{
	UCHAR,
	USHORT,
	UINT,
	
	INT
};

typedef struct Channel
{
	unsigned int width;
	unsigned int height;
	Subsampling subsampling;
	
	SampleType sampleType;
	unsigned char depth;
	bool sgnd; // signed (which is a C keyword, hence the Hungarian)
	
	unsigned char *buf;
	intptr_t colbytes;
	intptr_t rowbytes;
	
	Channel() :
		width(0),
		height(0),
		sampleType(UCHAR),
		depth(8),
		sgnd(false),
		buf(NULL),
		colbytes(0),
		rowbytes(0)
	{
	}
	
} Channel;


typedef struct Buffer
{
	unsigned char channels;
	Channel channel[J2K_CODEC_MAX_CHANNELS];
	
	Buffer() : channels(0) {}
	
} Buffer;


// return false to cancel
typedef bool (* ProgressProc)(void *refCon, size_t count, size_t total);
typedef bool (* AbortProc)(void *refCon);

typedef struct Progress
{
	ProgressProc progressProc;
	AbortProc abortProc;
	
	void *refCon;
	
	bool keepGoing;
	
	
	Progress() : progressProc(NULL), abortProc(NULL), refCon(NULL), keepGoing(true) {}
	
} Progress;


class Codec
{
  public:
	enum {
		J2K_CAN_NOT_READ	= 0,
		J2K_CAN_READ		= (1L << 0),
		J2K_CAN_SUBSAMPLE	= (1L << 1),
		J2K_APPLIES_LUT		= (1L << 2) // can't get the index, just the applied LUT
	};
	
	typedef unsigned int ReadFlags;
	
	enum {
		J2K_CAN_NOT_WRITE	= 0,
		J2K_CAN_WRITE		= (1L << 0),
	};
	
	typedef unsigned int WriteFlags;
	

  public:
	Codec() {}
	virtual ~Codec() {}
	
	virtual const char * Name() const = 0;
	virtual const char * FourCharCode() const = 0;
	
	virtual ReadFlags GetReadFlags() = 0;
	virtual WriteFlags GetWriteFlags() = 0;
	
	virtual bool Verify(InputFile &file);
	virtual void GetFileInfo(InputFile &file, FileInfo &info) = 0;
	virtual void ReadFile(InputFile &file, const Buffer &buffer, unsigned int subsample = 1, Progress *progress = NULL) = 0;
	// Subsample 1 means normal resolution.  Buffer width = image width / subsample.
	// But for all known JPEG 2000 implementations, subsample should be a power of 2.
	
	virtual void WriteFile(OutputFile &file, const FileInfo &info, const Buffer &buffer, Progress *progress = NULL) = 0;

	static Format GetFileFormat(InputFile &file);
	static void *CreateProfile(ColorSpace colorSpace, size_t &profileSize);
	static bool IssRGBProfile(const void *iccProfile, size_t profileSize);
	
	static void CopyBuffer(const Buffer &destination, const Buffer &source);

  protected:
	
	static unsigned int NumberOfCPUs();
};


size_t SizeOfSample(SampleType type);

unsigned int SubsampledSize(unsigned int size, int subsampling);


typedef std::list<Codec *> CodecList;

const CodecList & GetCodecList();
Codec * GetDefaultCodec();

}; // namespace j2k


#endif // J2K_CODEC_H
