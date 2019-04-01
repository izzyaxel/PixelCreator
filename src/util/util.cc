#include "util.hh"
#include "threadPool.hh"

#include <codecvt>
#include <locale>
#include <fstream>
#include <glad/glad.h>
#include <png.h>

#if defined(WINDOWS)
#include <windows.h>
#include <libloaderapi.h>
#elif defined(LINUX)
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#endif

std::string getCWD()
{
	#if defined(WINDOWS)
	wchar_t rawdir[2048];
	GetModuleFileNameW(nullptr, rawdir, 2048);
	std::string exeDir = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(std::wstring(rawdir));
	exeDir = StringTools::replaceAll(exeDir, '\\', '/');
	return exeDir.substr(0, exeDir.find_last_of('/')) + "/";
	#elif defined(LINUX)
	char dir[2048];
	getcwd(dir, 2048);
	return {dir};
	#endif
}

std::string readTextFile(std::string const &filePath)
{
	std::ifstream fileIn;
	fileIn.open(filePath);
	if(!fileIn) printf("Error opening file %s\n", filePath.data());
	return {std::istreambuf_iterator<char>(fileIn), std::istreambuf_iterator<char>()};
}

void createDirectory(std::string const &folderPath)
{
	#if defined(_WIN32)
	CreateDirectory(folderPath.data(), nullptr);
	#elif defined(__linux__)
	mkdir(folderPath.data(), 0700);
	#endif
}

void writePNG(std::string const &filePath, uint32_t width, uint32_t height, unsigned char **imageData)
{
	FILE *output = fopen(filePath.data(), "wb");
	if(!output)
	{
		printf("PNG Encoder: Failed to open %s for writing\n", filePath.data());
		return;
	}
	png_structp pngPtr;
	png_infop infoPtr;
	pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if(!pngPtr)
	{
		printf("PNG Encoder: Failed to create PNG write struct\n");
		fclose(output);
		return;
	}
	infoPtr = png_create_info_struct(pngPtr);
	if(!infoPtr)
	{
		printf("PNG Encoder: Failed to create PNG info struct\n");
		fclose(output);
		return;
	}
	if(setjmp((png_jmpbuf(pngPtr))))
	{
		printf("PNG Encoder: An error occured during I/O init\n");
		fclose(output);
		return;
	}
	png_init_io(pngPtr, output);
	if(setjmp(png_jmpbuf(pngPtr)))
	{
		printf("PNG Encoder: An error occured while writing header\n");
		fclose(output);
		return;
	}
	png_set_IHDR(pngPtr,
	             infoPtr,
	             width,
	             height,
	             8,
	             PNG_FORMAT_RGB,
	             PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_BASE,
	             PNG_FILTER_TYPE_BASE);
	png_write_info(pngPtr, infoPtr);
	if(setjmp(png_jmpbuf(pngPtr)))
	{
		printf("PNG Encoder: An error occured while writing\n");
		fclose(output);
		return;
	}
	png_write_image(pngPtr, imageData);
	if(setjmp(png_jmpbuf(pngPtr)))
	{
		printf("PNG Encoder: An error occured during end of write\n");
		fclose(output);
		return;
	}
	png_write_end(pngPtr, nullptr);
	fclose(output);
}

void screenshotIOThread(std::string const &folderPath, uint32_t width, uint32_t height, std::vector<unsigned char> pixels)
{
	createDirectory(folderPath); //Create the screenshots directory if it doesn't exist
	std::string fileName = "Screenshot ";
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);
	char dt[16];
	strftime(dt, 16, "%m-%d-%y %H%M%S", &tm); //Produce a formatted date and time string for the filename
	fileName += dt;
	fileName += ".png";
	unsigned char **pngData = new unsigned char *[height * sizeof(unsigned char *)]; //Reorder the data into rows for libpng
	for(unsigned int i = 0; i < height; i++) pngData[i] = new unsigned char[width * 3];
	for(unsigned int x = 0; x < height; x++) memcpy(pngData[height - x - 1], pixels.data() + x * (width * 3), width * 3);
	writePNG(folderPath + fileName, width, height, pngData); //Call to Beast to write the PNG file to disk
	for(uint32_t i = 0; i < height; i++) delete[] pngData[i];
	delete[] pngData;
}

void screenshot(std::string const &folderPath, uint32_t width, uint32_t height)
{
	std::vector<unsigned char> pixels;
	pixels.resize(width * height * 3); //Preallocate
	glPixelStorei(GL_PACK_ALIGNMENT, 1); //Ensure the pixel data we get from OGL is in the right format
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data()); //Grab the pixels currently in the buffer and store them in the vector
	threadPool.enqueue(screenshotIOThread, folderPath, width, height, pixels); //I/O will cause a hiccup in the framerate if we don't spin it off into a new asynchronous thread 
}

namespace StringTools
{
	bool contains(std::string const &input, std::string const &searchToken) {return input.find(searchToken) != std::string::npos;}
	
	bool startsWith(std::string const &input, std::string const &searchToken)
	{
		for(size_t i = 0; i < searchToken.length(); i++) if(input[i] != searchToken[i]) return false;
		return true;
	}
	
	bool endsWith(std::string const &input, std::string const &searchToken)
	{
		size_t j = searchToken.length();
		for(size_t i = input.length(); i > input.length() - searchToken.length(); i--)
		{
			if(input[i] != searchToken[j]) return false;
			j--;
		}
		return true;
	}
	
	std::string subString(std::string const &input, uint32_t const &distFromStart, uint32_t const &distFromEnd) {return std::string{input.begin() + distFromStart, input.end() - distFromEnd};}
	
	std::string trim(std::string const &input)
	{
		std::string::const_iterator begin = input.begin(), end = input.end();
		while(*begin == ' ') begin++;
		while(*end == ' ') end--;
		return std::string{begin, end};
	}
	
	std::string remove(std::string const &input, char const &remove)
	{
		std::string out = input;
		size_t loc = 0;
		while((loc = out.find(remove)) != std::string::npos) out.replace(loc, 1, "");
		return out;
	}
	
	std::string removeMult(std::string const &input, std::vector<char> const &remove)
	{
		std::string out = input;
		size_t loc = 0;
		for(auto const &token : remove) while((loc = out.find(token)) != std::string::npos) out.replace(loc, 1, "");
		return out;
	}
	
	std::vector<std::string> split(std::string const &input, char const &splitOn, bool const &removeMatches)
	{
		std::vector<std::string> out;
		if(input.begin() == input.end()) return out;
		std::string::const_iterator iEnd = input.begin(), iBegin = iEnd;
		while(iEnd < input.end())
		{
			if(*iEnd == splitOn)
			{
				if(iBegin != iEnd) out.emplace_back(iBegin, removeMatches ? iEnd : iEnd + 1);
				iBegin = ++iEnd;
				continue;
			}
			iEnd++;
		}
		if(*iEnd == splitOn && iEnd != iBegin) out.emplace_back(iBegin, removeMatches ? iEnd : iEnd + 1);
		else if(iBegin != iEnd) out.emplace_back(iBegin, iEnd);
		return out;
	}
	
	std::vector<std::string> splitMult(std::string const &input, std::vector<char> const &splitOn, bool const &removeMatches)
	{
		std::vector<std::string> out{};
		if(input.length() == 0) return out;
		std::string::const_iterator iBegin = input.begin(), iEnd = iBegin;
		bool matchFound = false;
		do
		{
			matchFound = false;
			for(auto const &token : splitOn)
			{
				if(*iEnd == token)
				{
					if(iBegin != iEnd) out.emplace_back(iBegin, removeMatches ? iEnd : iEnd + 1);
					iBegin = ++iEnd;
					if(iEnd >= input.end()) break;
					matchFound = true;
					break;
				}
			}
			if(!matchFound) iEnd++;
		}
		while(iEnd < input.end());
		matchFound = false;
		if(iBegin != iEnd)
		{
			for(auto const &token : splitOn)
			{
				if(*iEnd == token)
				{
					matchFound = true;
					out.emplace_back(iBegin, removeMatches ? iEnd : iEnd + 1);
					break;
				}
			}
			if(!matchFound) out.emplace_back(iBegin, iEnd);
		}
		return out;
	}
	
	std::vector<std::string> splitSeq(std::string const &input, std::string const &splitOn)
	{
		std::vector<std::string> out{};
		if(input.begin() == input.end()) return out;
		std::string::const_iterator iEnd = input.begin(), iBegin = iEnd;
		while(iEnd < input.end())
		{
			for(size_t i = 0; i < splitOn.length(); i++)
			{
				if(*iEnd != splitOn[i]) break;
				if(i == splitOn.length() - 1)
				{
					out.emplace_back(iBegin, iEnd + 1);
					iBegin = ++iEnd;
					break;
				}
				iEnd++;
			}
			iEnd++;
		}
		if(iBegin != iEnd) out.emplace_back(iBegin, iEnd);
		return out;
	}
	
	std::vector<std::string> splitSeqMult(std::string const &input, std::vector<std::string> const &splitOn)
	{
		std::vector<std::string> out{};
		if(input.begin() == input.end()) return out;
		std::string::const_iterator iEnd = input.begin(), iBegin = iEnd;
		while(iEnd < input.end())
		{
			for(auto const &token : splitOn) //for each word to split on
			{
				if(iEnd + token.length() > input.end()) continue; //if what we have left is longer than the length of the word we're checking it can't be a match, go to the next word
				for(size_t i = 0; i < token.length(); i++) //for each letter of the word
				{
					if(*iEnd != token[i]) break; //if the current letters don't match, break out of the loop
					if(i == token.length() - 1) //if we made it to the end of the word, it's a match, adjust for i starting at 0
					{
						out.emplace_back(iBegin, iEnd + 1); //add the word to the list
						iBegin = ++iEnd; //and set the iterators to one letter after the word
						break; //then end the current loop
					}
					iEnd++; //advance one letter in the input string
				}
			}
			iEnd++;
		}
		if(iBegin != iEnd) out.emplace_back(iBegin, iEnd); //Add any left over letters to the list
		return out;
	}
	
	std::string replaceAll(std::string const &input, char const &searchFor, char const &replaceWith)
	{
		std::string out;
		for(auto &character : input)
		{
			if(character == searchFor) out.push_back(replaceWith);
			else out.push_back(character);
		}
		return out;
	}
}