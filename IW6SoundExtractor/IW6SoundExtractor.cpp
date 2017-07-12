// IW6SoundExtractor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define DEFAULT_COLOR	0x07
#define INFO_COLOR		0x70
#define WARNING_COLOR	0xE0
#define ERROR_COLOR		0xC0

#define FF_MAGIC_SIGNED "IWff01005"
#define FF_MAGIC_UNSIGNED "IWffu1005"

#define FF_ZLIB_DATA_HEADER_SIGNED "IWffs100" // remember to skip 4 bytes
#define FF_ZLIB_DATA_HEADER_UNSIGNED "\x78\xDA"

#define ERR_NOT_IMPLEMENTED Print(CHANNEL_ERROR, "Not implemented!")


int errorCount = 0;
int warningCount = 0;
int startClock = 0;
int endClock = 0;
float operationTime = 0;

enum PrintChannel {
	CHANNEL_NULL,
	CHANNEL_INFO,
	CHANNEL_WARNING,
	CHANNEL_ERROR
};

inline void PressEnterToContinue()
{
	std::cout << "Press ENTER to continue... " << std::flush;
	std::cin.ignore(std::numeric_limits <std::streamsize> ::max(), '\n');
}

inline void PrintChannelName(int id)
{
	switch (id)
	{
	case CHANNEL_INFO:
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), INFO_COLOR);
		std::cout << "  INFO   ";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DEFAULT_COLOR);
		break;
	case CHANNEL_WARNING:
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WARNING_COLOR);
		std::cout << " WARNING ";
		warningCount++;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DEFAULT_COLOR);
		break;
	case CHANNEL_ERROR:
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ERROR_COLOR);
		std::cout << "  ERROR  ";
		errorCount++;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DEFAULT_COLOR);
		break;
	default:
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), INFO_COLOR);
		std::cout << "         ";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DEFAULT_COLOR);
		break;
	}
	std::cout << " ";
}

inline void Print(int channel, const char* fmt, ...)
{
	PrintChannelName(channel);
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);
	std::cout << buffer << std::endl;
}

inline void PrintTitle()
{
	Print(CHANNEL_NULL, "IW6 Sound Extractor");
	Print(CHANNEL_NULL, "Rewritten in C++ by Ray1235");
	Print(CHANNEL_NULL, "Based on an app written by Scobalula");
	Print(CHANNEL_NULL, "------------------------------------");
	Print(CHANNEL_INFO, "zlib version: %s", zlibVersion());
	Print(CHANNEL_NULL, "------------------------------------");
}

inline void PrintErrorNotEnoughArgs()
{
	Print(CHANNEL_ERROR, "No files were given!");
	Print(CHANNEL_NULL, "Simply drag and drop files onto the .exe to extract!");
	Print(CHANNEL_NULL, "------------------------------------");
}

const char * ZlibError(int index)
{
	return "";
}

int InflateChunk(
	z_stream m_stream,
	unsigned char * pDataIn,
	unsigned char * pDataOut)
{
	int dataInSize = FF_CHUNK_SIZE;
	int dataOutSize = FF_CHUNK_SIZE;
	if (pDataIn)
	{
		if (m_stream.avail_in == 0)
		{
			m_stream.avail_in = dataInSize;
			m_stream.next_in = pDataIn;
		}
		else
		{
			Print(CHANNEL_ERROR, "No space for input data");
		}
	}

	m_stream.avail_out = dataOutSize;
	m_stream.next_out = pDataOut;

	int done = 0;

	do
	{
		int result = inflate(&m_stream, Z_BLOCK);

		if (result < 0)
		{
			Print(CHANNEL_ERROR, "An error occured: %s", m_stream.msg);
			return -1;
		}

		done = (m_stream.avail_in == 0 ||
			(dataOutSize != m_stream.avail_out &&
				m_stream.avail_out != 0));
	} while (!done && m_stream.avail_out == dataOutSize);

	dataInSize = m_stream.avail_in;

	dataOutSize = dataOutSize - m_stream.avail_out;

	return done;
}

bool ProcessFile(char path[])
{
	bool isSigned = true;


	char targetDumpName[256];
	memset(targetDumpName, 0, sizeof(char) * 256);

	Print(CHANNEL_INFO, "Processing %s", PathFindFileName(path));

	strcat_s(targetDumpName, path);
	strcat_s(targetDumpName, ".dump");
	
	FILE * currentFile;
	FILE * dumpOutput;
	fopen_s(&currentFile, path, "rb");
	if (!currentFile)
	{
		Print(CHANNEL_ERROR, "Couldn't find file!");
		return false;
	}
	Print(CHANNEL_INFO, "Dumping to %s", PathFindFileName(targetDumpName));
	fopen_s(&dumpOutput, targetDumpName, "wb");
	char FFMagic[10];
	fread_s(FFMagic, 10, 1, 9, currentFile);
	FFMagic[9] = '\0';
	Print(CHANNEL_INFO, "Magic: %s", FFMagic);
	if (strncmp(FFMagic, FF_MAGIC_SIGNED, 9) != 0)
	{
		if (strncmp(FFMagic, FF_MAGIC_UNSIGNED, 9) != 0)
		{
			Print(CHANNEL_ERROR, "Unsupported magic! Must be %s or %s", FF_MAGIC_SIGNED, FF_MAGIC_UNSIGNED);
			return false;
		}
		else {
			isSigned = false;
		}
	}

	Print(CHANNEL_INFO, "FastFile is %s", (isSigned ? "signed" : "unsigned"));
	z_stream stream;
	stream.zalloc = NULL;
	stream.zfree = NULL;
	stream.opaque = NULL;
	stream.avail_in = 0;
	stream.avail_out = 0;
	stream.next_out = NULL;
	int ret = inflateInit(&stream);
	int ret2 = 0;
	unsigned have;
	unsigned char in[FF_CHUNK_SIZE+1];
	unsigned char out[FF_CHUNK_SIZE+1];
	char dummy[0x3ff8];
	if (isSigned)
	{
		char zlibDataHeader[9] = FF_ZLIB_DATA_HEADER_SIGNED;
		char zlibDataHeaderToCompare[8];
		while (strncmp(zlibDataHeader, zlibDataHeaderToCompare, 8) != 0)
		{
			for (int i = 1; i < 8; i++)
			{
				zlibDataHeaderToCompare[i - 1] = zlibDataHeaderToCompare[i];
			}
			fread_s(&zlibDataHeaderToCompare[7], 1, 1, 1, currentFile);
		}

		fread_s(dummy, 0x3ff8, 1, 0x3ff8, currentFile); // skip the garbage //0x146
		int zlibDataBegin = ftell(currentFile);
		Print(CHANNEL_INFO, "Found zlib data at %d", zlibDataBegin);
		
		while (ret != Z_STREAM_END)
		{
			stream.avail_in = fread_s(in, FF_CHUNK_SIZE, 1, FF_CHUNK_SIZE, currentFile);
			ret2 = ferror(currentFile);
			if (ret2)
			{
				inflateEnd(&stream);
				Print(CHANNEL_ERROR, "An unknown error occured! Code: %d", ret);
				fclose(dumpOutput);
				fclose(currentFile);
				return false;
			}
			if (stream.avail_in == 0)
			{
				break;
			}
			stream.next_in = in;
			//Print(CHANNEL_INFO, "Current pos: %d", ftell(currentFile));
			while (stream.avail_out == 0 && stream.avail_in > 0)
			{
				//InflateChunk(stream, in, out);
				
				stream.avail_out = FF_CHUNK_SIZE;
				stream.next_out = out;
				
				ret = inflate(&stream, Z_BLOCK);
				if(ret != Z_OK && ret != Z_STREAM_END)
				{
					Print(CHANNEL_ERROR, "Can't decompress FF: %s (%d) %s (%d)", zError(ret), ret, stream.msg ? stream.msg : "", zlibDataBegin + stream.total_in);
					fclose(dumpOutput);
					fclose(currentFile);
					inflateEnd(&stream);
					return false;
				}
				else if (ret == Z_STREAM_END) {
					break;
				}
				have = FF_CHUNK_SIZE - stream.avail_out;
				fwrite(out, 1, have, dumpOutput);
				ret2 = ferror(dumpOutput);
				if (ret2)
				{
					Print(CHANNEL_ERROR, "An unknown error occured! Code: %d", ret);
					fclose(dumpOutput);
					fclose(currentFile);
					inflateEnd(&stream);
					return false;
				}
				stream.avail_out = 0;
			}
		}
		inflateEnd(&stream);
		Print(CHANNEL_ERROR, "%d", ret);
		ret = (ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR);
		if (ret == Z_DATA_ERROR)
		{
			Print(CHANNEL_ERROR, "A data error occured and I have no idea what that means");
		}
		fclose(dumpOutput);
		fclose(currentFile);
	}
	else {
		ERR_NOT_IMPLEMENTED;
		fclose(dumpOutput);
		fclose(currentFile);
		return false;
	}
	return true;
}

int main(int argc, char * argv[])
{
	startClock = clock();
	PrintTitle();
	if (argc < 2)
	{
		PrintErrorNotEnoughArgs();
	}
	else
	{
		for (int i = 1; i < argc; i++)
		{
			char path[256];
			strcpy_s(path, argv[i]);
			if (ProcessFile(path))
				Print(CHANNEL_NULL, "File processed successfully");
			else
				Print(CHANNEL_NULL, "Errors occured while processing the file");
			Print(CHANNEL_NULL, "------------------------------------");
		}
	}
	endClock = clock();
	operationTime = (endClock - startClock) / (float)CLOCKS_PER_SEC;
	Print(CHANNEL_INFO, "Operation completed in %f seconds with %d errors and %d warnings", operationTime, errorCount, warningCount);
	Print(CHANNEL_NULL, "------------------------------------");
#ifdef PAUSE_AFTER_EXTRACT
	PressEnterToContinue();
#endif
    return 0;
}

