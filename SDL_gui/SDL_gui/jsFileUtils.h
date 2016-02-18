#pragma once

#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>  //for ostringsream
#include <iomanip>  //for setprecision
#include <fstream>
#include <algorithm>
#include <cfloat>
#include <map>
#include <stack>
#include <unordered_map>
#include <memory>

#if !_MSC_VER
#define BOOST_NO_CXX11_SCOPED_ENUMS
#define BOOST_NO_SCOPED_ENUMS
#endif
#include <boost/filesystem.hpp>
namespace std {
	namespace filesystem = boost::filesystem;
}

using namespace std;


void jsInitFileUtils();
string jsToDataPath(const string& path, bool makeAbsolute=false);
void jsEnableDataPath();
void jsDisableDataPath();
bool jsIsEnableDataPath();
bool jsRestoreWorkingDirectoryToDefault();
void jsSetDataPathRoot(const string& root);

template <class T>
string jsToString(const T& value) {
	ostringstream out;
	out << value;
	return out.str();
}

//----------------------------------------------------------
// jsBuffer
//----------------------------------------------------------

class jsBuffer{
	
public:
	jsBuffer();
	jsBuffer(const char * buffer, std::size_t size);
	jsBuffer(const string & text);
	jsBuffer(istream & stream, size_t ioBlockSize = 1024);

	void set(const char * _buffer, std::size_t _size);
    void set(const string & text);
	bool set(istream & stream, size_t ioBlockSize = 1024);
    void setall(char mem);
	void append(const string& _buffer);
	void append(const char * _buffer, std::size_t _size);
	void reserve(size_t size);

	bool writeTo(ostream & stream) const;

	void clear();

	void allocate(std::size_t _size);
    void resize(std::size_t _size);

	char * getData();
	const char * getData() const;

	string getText() const;
	operator string() const;  // cast to string, to use a buffer as a string
	jsBuffer & operator=(const string & text);

	std::size_t size() const;
    
	friend ostream & operator<<(ostream & ostr, const jsBuffer & buf);
	friend istream & operator>>(istream & istr, jsBuffer & buf);

	vector<char>::iterator begin();
	vector<char>::iterator end();
	vector<char>::const_iterator begin() const;
	vector<char>::const_iterator end() const;
	vector<char>::reverse_iterator rbegin();
	vector<char>::reverse_iterator rend();
	vector<char>::const_reverse_iterator rbegin() const;
	vector<char>::const_reverse_iterator rend() const;

	struct Line: public std::iterator<std::forward_iterator_tag,Line>{
		Line(vector<char>::iterator _begin, vector<char>::iterator _end);
        const string & operator*() const;
        const string * operator->() const;
        const string & asString() const;
        Line& operator++();
        Line operator++(int);
        bool operator!=(Line const& rhs) const;
        bool operator==(Line const& rhs) const;
        bool empty() const;

	private:
        string line;
        vector<char>::iterator _current, _begin, _end;
	};

	struct Lines{
		Lines(vector<char>::iterator begin, vector<char>::iterator end);
        Line begin();
        Line end();

	private:
        vector<char>::iterator _begin, _end;
	};

	Lines getLines();

private:
	vector<char> 	buffer;
	Line			currentLine;
};

//--------------------------------------------------
jsBuffer jsBufferFromFile(const string & path, bool binary=false);

//--------------------------------------------------
bool jsBufferToFile(const string & path, jsBuffer & buffer, bool binary=false);


//--------------------------------------------------
class jsFilePath{
public:
		
	static string getFileExt(const std::string& filename);
	static string removeExt(const std::string& filename);
	static string addLeadingSlash(const std::string& path);
	static string addTrailingSlash(const std::string& path);
	static string removeTrailingSlash(const std::string& path);
	static string getPathForDirectory(const std::string& path);
	static string getAbsolutePath(const std::string& path, bool bRelativeToData = true);

	static bool isAbsolute(const std::string& path);
	
	static string getFileName(const std::string& filePath, bool bRelativeToData = true);
	static string getBaseName(const std::string& filePath); // filename without extension

	static string getEnclosingDirectory(const std::string& filePath, bool bRelativeToData = true);
	static bool createEnclosingDirectory(const std::string& filePath, bool bRelativeToData = true, bool bRecursive = true);
	static string getCurrentWorkingDirectory();
	static string join(const std::string& path1, const std::string& path2);
	
	static string getCurrentExePath();
	static string getCurrentExeDir();

	static string getUserHomeDir();

	static string makeRelative(const std::string & from, const std::string & to);
};

class jsFile: public fstream{

public:
	
	enum Mode{
		Reference,
		ReadOnly,
		WriteOnly,
		ReadWrite,
		Append
	};

	jsFile();
	jsFile(const std::filesystem::path & path, Mode mode=ReadOnly, bool binary=true);
	jsFile(const jsFile & mom);
	jsFile & operator= (const jsFile & mom);
	~jsFile();

	bool open(const std::filesystem::path & path, Mode mode=ReadOnly, bool binary=false);
	bool changeMode(Mode mode, bool binary=false); // reopens a file to the same path with a different mode;
	void close();
	bool create();
	
	bool exists() const;
	string path() const;
	
	string getExtension() const;
	string getFileName() const;
	string getBaseName() const; // filename without extension
	string getEnclosingDirectory() const;
	string getAbsolutePath() const;

	bool canRead() const;
	bool canWrite() const;
	bool canExecute() const;

	bool isFile() const;
	bool isLink() const;
	bool isDirectory() const;
	bool isDevice() const;
	bool isHidden() const;

	void setWriteable(bool writeable=true);
	void setReadOnly(bool readable=true);
	void setExecutable(bool executable=true);
	
	//these all work for files and directories
	bool copyTo(const std::string& path, bool bRelativeToData = true, bool overwrite = false) const;
	bool moveTo(const std::string& path, bool bRelativeToData = true, bool overwrite = false);
	bool renameTo(const std::string& path, bool bRelativeToData = true, bool overwrite = false);
	
	
	//be careful! this deletes a file or folder :) 
	bool remove(bool recursive=false);

	uint64_t getSize() const;

	//this allows to compare files by their paths, also provides sorting and use as key in stl containers
	bool operator==(const jsFile & file) const;
	bool operator!=(const jsFile & file) const;
	bool operator<(const jsFile & file) const;
	bool operator<=(const jsFile & file) const;
	bool operator>(const jsFile & file) const;
	bool operator>=(const jsFile & file) const;


	//------------------
	// stream operations
	//------------------

	// since this class inherits from fstream it can be used as a r/w stream:
	// http://www.cplusplus.com/reference/iostream/fstream/


	//helper functions to read/write a whole file to/from an ofBuffer
	jsBuffer readToBuffer();
	bool writeFromBuffer(const jsBuffer & buffer);

	
	// this can be used to read the whole stream into an output stream. ie:
	// it's equivalent to rdbuf() just here to make it easier to use
	// ofLogNotice() << file.getFileBuffer();
	// write_file << file.getFileBuffer();
	filebuf * getFileBuffer() const;
	
	operator std::filesystem::path(){
		return myFile;
	}

    operator const std::filesystem::path() const{
        return myFile;
    }
	

	//-------
	//static helpers
	//-------

	static bool copyFromTo(const std::string& pathSrc, const std::string& pathDst, bool bRelativeToData = true,  bool overwrite = false);

	//be careful with slashes here - appending a slash when moving a folder will causes mad headaches in osx
	static bool moveFromTo(const std::string& pathSrc, const std::string& pathDst, bool bRelativeToData = true, bool overwrite = false);
	static bool doesFileExist(const std::string& fPath,  bool bRelativeToData = true);
	static bool removeFile(const std::string& path, bool bRelativeToData = true);

private:
	bool isWriteMode();
	bool openStream(Mode _mode, bool binary);
	void copyFrom(const jsFile & mom);
	std::filesystem::path myFile;
	Mode mode;
	bool binary;
};

class jsDirectory{

public:
	jsDirectory();
	jsDirectory(const std::filesystem::path & path);

	void open(const std::filesystem::path & path);
	void close();
	bool create(bool recursive = false);

	bool exists() const;
	string path() const;
	string getAbsolutePath() const;

	bool canRead() const;
	bool canWrite() const;
	bool canExecute() const;
	
	bool isDirectory() const;
	bool isHidden() const;

	void setWriteable(bool writeable=true);
	void setReadOnly(bool readable=true);
	void setExecutable(bool executable=true);
	void setShowHidden(bool showHidden);

	bool copyTo(const string& path, bool bRelativeToData = true, bool overwrite = false);
	bool moveTo(const string& path, bool bRelativeToData = true, bool overwrite = false);
	bool renameTo(const string& path, bool bRelativeToData = true, bool overwrite = false);

	//be careful! this deletes a file or folder :)
	bool remove(bool recursive);

	//-------------------
	// dirList operations
	//-------------------
	void allowExt(const string& extension);
	std::size_t listDir(const string& path);
	std::size_t listDir();

	string getOriginalDirectory() const;
	string getName(std::size_t position) const; // e.g., "image.png"
	string getPath(std::size_t position) const;
	jsFile getFile(std::size_t position, jsFile::Mode mode=jsFile::Reference, bool binary=false) const;
	const vector<jsFile> & getFiles() const;

	jsFile operator[](std::size_t position) const;

	bool getShowHidden() const;

	void reset(); //equivalent to close, just here for bw compatibility with ofxDirList
	void sort();
    jsDirectory getSorted();

	std::size_t size() const;

	//this allows to compare dirs by their paths, also provides sorting and use as key in stl containers
	bool operator==(const jsDirectory & dir) const;
	bool operator!=(const jsDirectory & dir) const;
	bool operator<(const jsDirectory & dir) const;
	bool operator<=(const jsDirectory & dir) const;
	bool operator>(const jsDirectory & dir) const;
	bool operator>=(const jsDirectory & dir) const;

	operator std::filesystem::path(){
		return myDir;
	}

    operator const std::filesystem::path() const{
        return myDir;
    }

	//-------
	//static helpers
	//-------

	static bool createDirectory(const std::string& dirPath, bool bRelativeToData = true, bool recursive = false);
	static bool isDirectoryEmpty(const std::string& dirPath, bool bRelativeToData = true );
	static bool doesDirectoryExist(const std::string& dirPath, bool bRelativeToData = true);
	static bool removeDirectory(const std::string& path, bool deleteIfNotEmpty,  bool bRelativeToData = true);

	vector<jsFile>::const_iterator begin() const;
	vector<jsFile>::const_iterator end() const;
	vector<jsFile>::const_reverse_iterator rbegin() const;
	vector<jsFile>::const_reverse_iterator rend() const;

private:
	std::filesystem::path myDir;
	string originalDirectory;
	vector <string> extensions;
	vector <jsFile> files;
	bool showHidden;

};

