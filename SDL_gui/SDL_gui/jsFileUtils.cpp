#include "jsFileUtils.h"
#include "GUI_utils.h"
#ifndef _WIN32
	#include <pwd.h>
	#include <sys/stat.h>
#else
	#if (_MSC_VER)       // microsoft visual studio
		#define _CRT_SECURE_NO_WARNINGS
		#define _WINSOCK_DEPRECATED_NO_WARNINGS
		#ifndef UNICODE
			#define UNICODE
		#endif
		#include <stdint.h>
		#include <functional>
		#include <Windows.h>
	#endif
#endif

#if defined( __APPLE_CC__)
    #define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE_SIMULATOR || TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE || TARGET_IPHONE
        #include <unistd.h>
    #endif
#endif

#ifdef __OSX__
	#include <mach-o/dyld.h>       /* _NSGetExecutablePath */
	#include <limits.h>        /* PATH_MAX */
#endif

bool enableDataPath = true;

//--------------------------------------------------
string defaultDataPath() {
#if defined __OSX__
	try {
		return std::filesystem::canonical(ofFilePath::join(ofFilePath::getCurrentExeDir(), "../../../data/")).string();
	}
	catch (...) {
		return ofFilePath::join(ofFilePath::getCurrentExeDir(), "../../../data/");
	}
#elif defined __ANDROID__
	return string("sdcard/");
#else
	try {
		return std::filesystem::canonical(jsFilePath::join(jsFilePath::getCurrentExeDir(), "data/")).string();
	}
	catch (...) {
		return jsFilePath::join(jsFilePath::getCurrentExeDir(), "data/");
	}
#endif
}

//--------------------------------------------------
std::filesystem::path & defaultWorkingDirectory() {
	static auto * defaultWorkingDirectory = new std::filesystem::path();
	return *defaultWorkingDirectory;
}

//--------------------------------------------------
std::filesystem::path & dataPathRoot() {
	static auto * dataPathRoot = new std::filesystem::path(defaultDataPath());
	return *dataPathRoot;
}

//--------------------------------------------------
bool __bInitFileUtils = false;
void jsInitFileUtils() {
	if (!__bInitFileUtils) {
		defaultWorkingDirectory() = std::filesystem::absolute(std::filesystem::current_path());
		__bInitFileUtils = true;
	}
}

//--------------------------------------------------
void jsEnableDataPath() {
	enableDataPath = true;
}

//--------------------------------------------------
void jsDisableDataPath() {
	enableDataPath = false;
}

//--------------------------------------------------
bool jsIsEnableDataPath() {
	return enableDataPath;
}

//--------------------------------------------------
bool jsRestoreWorkingDirectoryToDefault() {
	try {
		std::filesystem::current_path(defaultWorkingDirectory());
		return true;
	}
	catch (...) {
		return false;
	}
}

//--------------------------------------------------
void jsSetDataPathRoot(const string& newRoot) {
	dataPathRoot() = newRoot;
}

//--------------------------------------------------
string jsToDataPath(const string& path, bool makeAbsolute) {
	if (!enableDataPath)
		return path;

	bool hasTrailingSlash = !path.empty() && std::filesystem::path(path).generic_string().back() == '/';

	// if our Current Working Directory has changed (e.g. file open dialog)
#ifdef _WIN32
	if (defaultWorkingDirectory() != std::filesystem::current_path()) {
		// change our cwd back to where it was on app load
		bool ret = jsRestoreWorkingDirectoryToDefault();
		if (!ret) {
			GUI_log("jsToDataPath: error while trying to change back to default working directory %s\n", defaultWorkingDirectory().c_str());
		}
	}
#endif

	// this could be performed here, or wherever we might think we accidentally change the cwd, e.g. after file dialogs on windows
	const auto  & dataPath = dataPathRoot();
	std::filesystem::path inputPath(path);
	std::filesystem::path outputPath;

	// if path is already absolute, just return it
	if (inputPath.is_absolute()) {
		try {
			auto outpath = std::filesystem::canonical(inputPath);
			if (std::filesystem::is_directory(outpath) && hasTrailingSlash) {
				return jsFilePath::addTrailingSlash(outpath.string());
			}
			else {
				return outpath.string();
			}
		}
		catch (...) {
			return inputPath.string();
		}
	}

	// here we check whether path already refers to the data folder by looking for common elements
	// if the path begins with the full contents of dataPathRoot then the data path has already been added
	// we compare inputPath.toString() rather that the input var path to ensure common formatting against dataPath.toString()
	auto dirDataPath = dataPath.string();
	// also, we strip the trailing slash from dataPath since `path` may be input as a file formatted path even if it is a folder (i.e. missing trailing slash)
	dirDataPath = jsFilePath::addTrailingSlash(dirDataPath);

	auto relativeDirDataPath = jsFilePath::makeRelative(std::filesystem::current_path().string(), dataPath.string());
	relativeDirDataPath = jsFilePath::addTrailingSlash(relativeDirDataPath);

	if (inputPath.string().find(dirDataPath) != 0 && inputPath.string().find(relativeDirDataPath) != 0) {
		// inputPath doesn't contain data path already, so we build the output path as the inputPath relative to the dataPath
		if (makeAbsolute) {
			outputPath = dirDataPath / inputPath;
		}
		else {
			outputPath = relativeDirDataPath / inputPath;
		}
	}
	else {
		// inputPath already contains data path, so no need to change
		outputPath = inputPath;
	}

	// finally, if we do want an absolute path and we don't already have one
	if (makeAbsolute) {
		// then we return the absolute form of the path
		try {
			auto outpath = std::filesystem::canonical(std::filesystem::absolute(outputPath));
			if (std::filesystem::is_directory(outpath) && hasTrailingSlash) {
				return jsFilePath::addTrailingSlash(outpath.string());
			}
			else {
				return outpath.string();
			}
		}
		catch (std::exception &) {
			return std::filesystem::absolute(outputPath).string();
		}
	}
	else {
		// or output the relative path
		return outputPath.string();
	}
}


//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- jsBuffer
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

//--------------------------------------------------
jsBuffer::jsBuffer()
:currentLine(end(),end()){
}

//--------------------------------------------------
jsBuffer::jsBuffer(const char * _buffer, std::size_t size)
:buffer(_buffer,_buffer+size)
,currentLine(end(),end()){
}

//--------------------------------------------------
jsBuffer::jsBuffer(const string & text)
:buffer(text.begin(),text.end())
,currentLine(end(),end()){
}

//--------------------------------------------------
jsBuffer::jsBuffer(istream & stream, size_t ioBlockSize)
:currentLine(end(),end()){
	set(stream, ioBlockSize);
}

//--------------------------------------------------
bool jsBuffer::set(istream & stream, size_t ioBlockSize){
	if(stream.bad()){
		clear();
		return false;
	}else{
		buffer.clear();
	}

	vector<char> aux_buffer(ioBlockSize);
	while(stream.good()){
		stream.read(&aux_buffer[0], ioBlockSize);
		append(aux_buffer.data(), stream.gcount());
	}
	return true;
}

//--------------------------------------------------
void jsBuffer::setall(char mem){
	buffer.assign(buffer.size(), mem);
}

//--------------------------------------------------
bool jsBuffer::writeTo(ostream & stream) const {
	if(stream.bad()){
		return false;
	}
	stream.write(buffer.data(), buffer.size());
	return stream.good();
}

//--------------------------------------------------
void jsBuffer::set(const char * _buffer, std::size_t _size){
	buffer.assign(_buffer, _buffer+_size);
}

//--------------------------------------------------
void jsBuffer::set(const string & text){
	set(text.c_str(), text.size());
}

//--------------------------------------------------
void jsBuffer::append(const string& _buffer){
	append(_buffer.c_str(), _buffer.size());
}

//--------------------------------------------------
void jsBuffer::append(const char * _buffer, std::size_t _size){
	buffer.insert(buffer.end(), _buffer, _buffer + _size);
}

//--------------------------------------------------
void jsBuffer::reserve(size_t size){
	buffer.reserve(size);
}

//--------------------------------------------------
void jsBuffer::clear(){
	buffer.clear();
}

//--------------------------------------------------
void jsBuffer::allocate(std::size_t _size){
	resize(_size);
}

//--------------------------------------------------
void jsBuffer::resize(std::size_t _size){
	buffer.resize(_size);
}


//--------------------------------------------------
char * jsBuffer::getData(){
	return buffer.data();
}

//--------------------------------------------------
const char * jsBuffer::getData() const{
	return buffer.data();
}

//--------------------------------------------------
string jsBuffer::getText() const {
	if(buffer.empty()){
		return "";
	}
	return std::string(buffer.begin(), buffer.end());
}

//--------------------------------------------------
jsBuffer::operator string() const {
	return getText();
}

//--------------------------------------------------
jsBuffer & jsBuffer::operator=(const string & text){
	set(text);
	return *this;
}

//--------------------------------------------------
std::size_t jsBuffer::size() const {
	return buffer.size();
}

//--------------------------------------------------
vector<char>::iterator jsBuffer::begin(){
	return buffer.begin();
}

//--------------------------------------------------
vector<char>::iterator jsBuffer::end(){
	return buffer.end();
}

//--------------------------------------------------
vector<char>::const_iterator jsBuffer::begin() const{
	return buffer.begin();
}

//--------------------------------------------------
vector<char>::const_iterator jsBuffer::end() const{
	return buffer.end();
}

//--------------------------------------------------
vector<char>::reverse_iterator jsBuffer::rbegin(){
	return buffer.rbegin();
}

//--------------------------------------------------
vector<char>::reverse_iterator jsBuffer::rend(){
	return buffer.rend();
}

//--------------------------------------------------
vector<char>::const_reverse_iterator jsBuffer::rbegin() const{
	return buffer.rbegin();
}

//--------------------------------------------------
vector<char>::const_reverse_iterator jsBuffer::rend() const{
	return buffer.rend();
}

//--------------------------------------------------
jsBuffer::Line::Line(vector<char>::iterator _begin, vector<char>::iterator _end)
	:_current(_begin)
	,_begin(_begin)
	,_end(_end){
	if(_begin == _end){
		line =  "";
		return;
	}

	bool lineEndWasCR = false;
	while(_current != _end && *_current != '\n'){
		if(*_current == '\r'){
			lineEndWasCR = true;
			break;
		}else if(*_current==0 && _current+1 == _end){
			break;
		}else{
			_current++;
		}
	}
	line = string(_begin, _current);
	if(_current != _end){
		_current++;
	}
	// if lineEndWasCR check for CRLF
	if(lineEndWasCR && _current != _end && *_current == '\n'){
		_current++;
	}
}

//--------------------------------------------------
const string & jsBuffer::Line::operator*() const{
	return line;
}

//--------------------------------------------------
const string * jsBuffer::Line::operator->() const{
	return &line;
}

//--------------------------------------------------
const string & jsBuffer::Line::asString() const{
	return line;
}

//--------------------------------------------------
jsBuffer::Line & jsBuffer::Line::operator++(){
	*this = Line(_current,_end);
	return *this;
}

//--------------------------------------------------
jsBuffer::Line jsBuffer::Line::operator++(int) {
	Line tmp(*this);
	operator++();
	return tmp;
}

//--------------------------------------------------
bool jsBuffer::Line::operator!=(Line const& rhs) const{
	return rhs._begin != _begin || rhs._end != _end;
}

//--------------------------------------------------
bool jsBuffer::Line::operator==(Line const& rhs) const{
	return rhs._begin == _begin && rhs._end == _end;
}

bool jsBuffer::Line::empty() const{
	return _begin == _end;
}

//--------------------------------------------------
jsBuffer::Lines::Lines(vector<char>::iterator begin, vector<char>::iterator end)
:_begin(begin)
,_end(end){}

//--------------------------------------------------
jsBuffer::Line jsBuffer::Lines::begin(){
	return Line(_begin,_end);
}

//--------------------------------------------------
jsBuffer::Line jsBuffer::Lines::end(){
	return Line(_end,_end);
}

//--------------------------------------------------
jsBuffer::Lines jsBuffer::getLines(){
	return jsBuffer::Lines(begin(), end());
}

//--------------------------------------------------
ostream & operator<<(ostream & ostr, const jsBuffer & buf){
	buf.writeTo(ostr);
	return ostr;
}

//--------------------------------------------------
istream & operator>>(istream & istr, jsBuffer & buf){
	buf.set(istr);
	return istr;
}

//--------------------------------------------------
jsBuffer jsBufferFromFile(const string & path, bool binary){
	jsFile f(path,jsFile::ReadOnly, binary);
	return jsBuffer(f);
}

//--------------------------------------------------
bool jsBufferToFile(const string & path, jsBuffer & buffer, bool binary){
	jsFile f(path, jsFile::WriteOnly, binary);
	return buffer.writeTo(f);
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- jsFile
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
jsFile::jsFile()
:mode(Reference)
,binary(true){
}

jsFile::jsFile(const std::filesystem::path & path, Mode mode, bool binary)
:mode(mode)
,binary(true){
	open(path, mode, binary);
}

//-------------------------------------------------------------------------------------------------------------
jsFile::~jsFile(){
	//close();
}

//-------------------------------------------------------------------------------------------------------------
jsFile::jsFile(const jsFile & mom)
:basic_ios()
,fstream()
,mode(Reference)
,binary(true){
	copyFrom(mom);
}

//-------------------------------------------------------------------------------------------------------------
jsFile & jsFile::operator=(const jsFile & mom){
	copyFrom(mom);
	return *this;
}

//-------------------------------------------------------------------------------------------------------------
void jsFile::copyFrom(const jsFile & mom){
	if(&mom != this){
		Mode new_mode = mom.mode;
		if(new_mode != Reference && new_mode != ReadOnly){
			new_mode = ReadOnly;
			GUI_log("sfFile::copyFrom(): copying a writable file, opening new copy as read only\n");
		}
		open(mom.myFile.string(), new_mode, mom.binary);
	}
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::openStream(Mode _mode, bool _binary){
	mode = _mode;
	binary = _binary;
	ios_base::openmode binary_mode = binary ? ios::binary : (ios_base::openmode)0;
	switch(_mode) {
		case WriteOnly:
		case ReadWrite:
		case Append:
			if(!jsDirectory(jsFilePath::getEnclosingDirectory(path())).exists()){
				jsFilePath::createEnclosingDirectory(path());
			}
			break;
		case Reference:
		case ReadOnly:
			break;
	}
	switch(_mode){
		case Reference:
			return true;
			break;

		case ReadOnly:
			if(exists() && isFile()){
				fstream::open(path().c_str(), ios::in | binary_mode);
			}
			break;

		case WriteOnly:
			fstream::open(path().c_str(), ios::out | binary_mode);
			break;

		case ReadWrite:
			fstream::open(path().c_str(), ios_base::in | ios_base::out | binary_mode);
			break;

		case Append:
			fstream::open(path().c_str(), ios::out | ios::app | binary_mode);
			break;
	}
	return fstream::good();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::open(const std::filesystem::path & _path, Mode _mode, bool binary){
	close();
	//myFile = std::filesystem::path(GUI_getResourcePath() + _path.string());
	myFile = std::filesystem::path( jsToDataPath(_path.string()) );
	return openStream(_mode, binary);
}

//-------------------------------------------------------------------------------------------------------------
bool jsFile::changeMode(Mode _mode, bool binary){
	if(_mode != mode){
		string _path = path();
		close();
		myFile = std::filesystem::path(_path);
		return openStream(_mode, binary);
	}
	else{
		return true;
	}
}

//-------------------------------------------------------------------------------------------------------------
bool jsFile::isWriteMode(){
	return mode != ReadOnly;
}

//-------------------------------------------------------------------------------------------------------------
void jsFile::close(){
	myFile = std::filesystem::path();
	if(mode!=Reference) fstream::close();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::create(){
	bool success = false;

	if(!myFile.string().empty()){
		auto oldmode = this->mode;
		auto oldpath = path();
		success = open(path(),jsFile::WriteOnly,binary);
		close();
		open(oldpath,oldmode,binary);
	}

	return success;
}

//------------------------------------------------------------------------------------------------------------
jsBuffer jsFile::readToBuffer(){
	if(myFile.string().empty() || !std::filesystem::exists(myFile)){
		return jsBuffer();
	}

	return jsBuffer(*this);
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::writeFromBuffer(const jsBuffer & buffer){
	if(myFile.string().empty()){
		return false;
	}
	if(!isWriteMode()){
		GUI_log("jsFile::writeFromBuffer() : trying to write to read only file %s\n", myFile.string().c_str());
	}
	return buffer.writeTo(*this);
}

//------------------------------------------------------------------------------------------------------------
filebuf *jsFile::getFileBuffer() const {
	return rdbuf();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::exists() const {
	if(path().empty()){
		return false;
	}

	return std::filesystem::exists(myFile);
}

//------------------------------------------------------------------------------------------------------------
string jsFile::path() const {
	return myFile.string();
}

//------------------------------------------------------------------------------------------------------------
string jsFile::getExtension() const {
	auto dotext = myFile.extension().string();
	if(!dotext.empty() && dotext.front()=='.'){
		return std::string(dotext.begin()+1,dotext.end());
	}else{
		return dotext;
	}
}

//------------------------------------------------------------------------------------------------------------
string jsFile::getFileName() const {
	return myFile.filename().string();
}

//------------------------------------------------------------------------------------------------------------
string jsFile::getBaseName() const {
	return myFile.stem().string();
}

//------------------------------------------------------------------------------------------------------------
string jsFile::getEnclosingDirectory() const {
	return jsFilePath::getEnclosingDirectory(path());
}

//------------------------------------------------------------------------------------------------------------
string jsFile::getAbsolutePath() const {
	return jsFilePath::getAbsolutePath(path());
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::canRead() const {
	auto perm = std::filesystem::status(myFile).permissions();
#ifdef _WIN32
	DWORD attr = GetFileAttributes(myFile.native().c_str());
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	return true;
#else
	struct stat info;
	stat(path().c_str(), &info);  // Error check omitted
	if(geteuid() == info.st_uid){
		return perm & std::filesystem::owner_read;
	}else if (getegid() == info.st_gid){
		return perm & std::filesystem::group_read;
	}else{
		return perm & std::filesystem::others_read;
	}
#endif
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::canWrite() const {
	auto perm = std::filesystem::status(myFile).permissions();
#ifdef _WIN32
	DWORD attr = GetFileAttributes(myFile.native().c_str());
	if (attr == INVALID_FILE_ATTRIBUTES){
		return false;
	}else{
		return (attr & FILE_ATTRIBUTE_READONLY) == 0;
	}
#else
	struct stat info;
	stat(path().c_str(), &info);  // Error check omitted
	if(geteuid() == info.st_uid){
		return perm & std::filesystem::owner_write;
	}else if (getegid() == info.st_gid){
		return perm & std::filesystem::group_write;
	}else{
		return perm & std::filesystem::others_write;
	}
#endif
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::canExecute() const {
	auto perm = std::filesystem::status(myFile).permissions();
#ifdef _WIN32
	return getExtension() == "exe";
#else
	struct stat info;
	stat(path().c_str(), &info);  // Error check omitted
	if(geteuid() == info.st_uid){
		return perm & std::filesystem::owner_exe;
	}else if (getegid() == info.st_gid){
		return perm & std::filesystem::group_exe;
	}else{
		return perm & std::filesystem::others_exe;
	}
#endif
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::isFile() const {
	return std::filesystem::is_regular_file(myFile);
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::isLink() const {
	return std::filesystem::is_symlink(myFile);
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::isDirectory() const {
	return std::filesystem::is_directory(myFile);
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::isDevice() const {
#ifdef TARGET_WIN32
	return false;
#else
	return std::filesystem::status(myFile).type() == std::filesystem::block_file;
#endif
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::isHidden() const {
#ifdef TARGET_WIN32
	return false;
#else
	return myFile.filename() != "." && myFile.filename() != ".." && myFile.filename().string()[0] == '.';
#endif
}

//------------------------------------------------------------------------------------------------------------
void jsFile::setWriteable(bool flag){
	setReadOnly(!flag);
}

//------------------------------------------------------------------------------------------------------------
void jsFile::setReadOnly(bool flag){
	try{
		if(flag){
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::remove_perms);
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::remove_perms);
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::remove_perms);
		}else{
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::add_perms);
		}
	}catch(std::exception & e){
		//ofLogError() << "Couldn't set write permission on " << myFile << ": " << e.what();
		GUI_log("Couldn't set write permission on %s:%s\n", myFile.string().c_str(), e.what());
	}
}

//------------------------------------------------------------------------------------------------------------
void jsFile::setExecutable(bool flag){
	try{
		std::filesystem::permissions(myFile, std::filesystem::perms::owner_exe | std::filesystem::perms::add_perms);
	}catch(std::exception & e){
		//ofLogError() << "Couldn't set write permission on " << myFile << ": " << e.what();
		GUI_log("Couldn't set write permission on %s:%s\n", myFile.string().c_str(), e.what());
	}
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::copyTo(const string& _path, bool bRelativeToData, bool overwrite) const{
	std::string path = _path;

	if(isDirectory()){
		return jsDirectory(myFile).copyTo(path,bRelativeToData,overwrite);
	}
	if(path.empty()){
		//ofLogError("ofFile") << "copyTo(): destination path " << _path << " is empty";
		GUI_log("jsFile::copyTo(): destination path %s is empty\n", _path.c_str());
		return false;
	}
	if(!exists()){
		//ofLogError("ofFile") << "copyTo(): source file " << this->path() << " does not exist";
		GUI_log("jsFile::copyTo(): source path %s is does not exist\n", this->path().c_str());
		return false;
	}

	if(bRelativeToData){
		path = jsToDataPath(path);
		//path = GUI_getResourcePath() + path;
	}
	if(jsFile::doesFileExist(path, bRelativeToData)){
		if(isFile() && jsFile(path,jsFile::Reference).isDirectory()){
			path = jsFilePath::join(path,getFileName());
		}
		if(jsFile::doesFileExist(path, bRelativeToData)){
			if(overwrite){
				jsFile::removeFile(path, bRelativeToData);
			}else{
				//ofLogWarning("ofFile") << "copyTo(): destination file \"" << path << "\" already exists, set bool overwrite to true if you want to overwrite it";
			}
		}
	}

	try{
		if(!jsDirectory(jsFilePath::getEnclosingDirectory(path,bRelativeToData)).exists()){
			jsFilePath::createEnclosingDirectory(path, bRelativeToData);
		}
		std::filesystem::copy_file(myFile,path);
	}catch(std::exception & except){
		//ofLogError("ofFile") <<  "copyTo(): unable to copy \"" << path << "\":" << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::moveTo(const string& _path, bool bRelativeToData, bool overwrite){
	std::string path = _path;

	if(path.empty()){
		//ofLogError("ofFile") << "moveTo(): destination path is empty";
		return false;
	}
	if(!exists()){
		//ofLogError("ofFile") << "moveTo(): source file does not exist";
		return false;
	}

	if(bRelativeToData){
		path = jsToDataPath(path);
		//path = GUI_getResourcePath() + path;
	}
	if(jsFile::doesFileExist(path, bRelativeToData)){
		if(isFile() && jsFile(path,jsFile::Reference).isDirectory()){
			path = jsFilePath::join(path,getFileName());
		}
		if(jsFile::doesFileExist(path, bRelativeToData)){
			if(overwrite){
				jsFile::removeFile(path, bRelativeToData);
			}else{
				//ofLogWarning("ofFile") << "copyTo(): destination file \"" << path << "\" already exists, set bool overwrite to true if you want to overwrite it";
			}
		}
	}

	try{
		auto mode = this->mode;
		if(mode != jsFile::Reference){
			changeMode(jsFile::Reference, binary);
		}
		if(!jsDirectory(jsFilePath::getEnclosingDirectory(path,bRelativeToData)).exists()){
			jsFilePath::createEnclosingDirectory(path, bRelativeToData);
		}
		std::filesystem::rename(myFile,path);
		myFile = path;
		if(mode != jsFile::Reference){
			changeMode(mode, binary);
		}
	}
	catch(std::exception & except){
		//ofLogError("ofFile") << "moveTo(): unable to move \"" << path << "\":" << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::renameTo(const string& path, bool bRelativeToData, bool overwrite){
	return moveTo(path,bRelativeToData,overwrite);
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::remove(bool recursive){
	if(myFile.string().empty()){
		//ofLogError("ofFile") << "remove(): file path is empty";
		return false;
	}
	if(!exists()){
		//ofLogError("ofFile") << "remove(): file does not exist";
		return false;
	}

	try{
		if(mode!=Reference){
			open(path(),Reference,binary);
		}
		if(recursive){
			std::filesystem::remove_all(myFile);
		}else{
			std::filesystem::remove(myFile);
		}
	}catch(std::exception & except){
		//ofLogError("ofFile") << "remove(): unable to remove \"" << myFile << "\": " << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
uint64_t jsFile::getSize() const {
	try{
		return std::filesystem::file_size(myFile);
	}catch(std::exception & except){
		//ofLogError("ofFile") << "getSize(): unable to get size of \"" << myFile << "\": " << except.what();
		return 0;
	}
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::operator==(const jsFile & file) const {
	return getAbsolutePath() == file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::operator!=(const jsFile & file) const {
	return getAbsolutePath() != file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::operator<(const jsFile & file) const {
	return getAbsolutePath() < file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::operator<=(const jsFile & file) const {
	return getAbsolutePath() <= file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::operator>(const jsFile & file) const {
	return getAbsolutePath() > file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::operator>=(const jsFile & file) const {
	return getAbsolutePath() >= file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
// ofFile Static Methods
//------------------------------------------------------------------------------------------------------------

bool jsFile::copyFromTo(const std::string& pathSrc, const std::string& pathDst, bool bRelativeToData,  bool overwrite){
	return jsFile(pathSrc,jsFile::Reference).copyTo(pathDst,bRelativeToData,overwrite);
}

//be careful with slashes here - appending a slash when moving a folder will causes mad headaches
//------------------------------------------------------------------------------------------------------------
bool jsFile::moveFromTo(const std::string& pathSrc, const std::string& pathDst, bool bRelativeToData, bool overwrite){
	return jsFile(pathSrc,jsFile::Reference).moveTo(pathDst, bRelativeToData, overwrite);
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::doesFileExist(const std::string& _fPath, bool bRelativeToData){
	std::string fPath = _fPath;
	if(bRelativeToData){
		fPath = jsToDataPath(fPath);
		//fPath = GUI_getResourcePath() + fPath;
	}
	jsFile file(fPath,jsFile::Reference);
	return !fPath.empty() && file.exists();
}

//------------------------------------------------------------------------------------------------------------
bool jsFile::removeFile(const std::string& _path, bool bRelativeToData){
	std::string path = _path;
	if(bRelativeToData){
		path = jsToDataPath(path);
		//path = GUI_getResourcePath() + path;
	}
	return jsFile(path,jsFile::Reference).remove();
}


//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- jsDirectory
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
jsDirectory::jsDirectory(){
	showHidden = false;
}

//------------------------------------------------------------------------------------------------------------
jsDirectory::jsDirectory(const std::filesystem::path & path){
	showHidden = false;
	open(path);
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::open(const std::filesystem::path & path){
	originalDirectory = jsFilePath::getPathForDirectory(path.string());
	files.clear();
    myDir = std::filesystem::path(jsToDataPath(originalDirectory));
	//myDir = std::filesystem::path( GUI_getResourcePath() + originalDirectory );
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::close(){
	myDir = std::filesystem::path();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::create(bool recursive){

	if(!myDir.string().empty()){
		try{
			if(recursive){
                std::filesystem::create_directories(myDir);
			}else{
                std::filesystem::create_directory(myDir);
			}
		}
		catch(std::exception & except){
			//ofLogError("ofDirectory") << "create(): " << except.what();
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::exists() const {
	return std::filesystem::exists(myDir);
}

//------------------------------------------------------------------------------------------------------------
string jsDirectory::path() const {
	return myDir.string();
}

//------------------------------------------------------------------------------------------------------------
string jsDirectory::getAbsolutePath() const {
	try{
		return std::filesystem::canonical(std::filesystem::absolute(myDir)).string();
	}catch(...){
		return std::filesystem::absolute(myDir).string();
	}
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::canRead() const {
	return jsFile(myDir,jsFile::Reference).canRead();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::canWrite() const {
	return jsFile(myDir,jsFile::Reference).canWrite();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::canExecute() const {
	return jsFile(myDir,jsFile::Reference).canExecute();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::isHidden() const {
	return jsFile(myDir,jsFile::Reference).isHidden();
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::setWriteable(bool flag){
	return jsFile(myDir,jsFile::Reference).setWriteable(flag);
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::setReadOnly(bool flag){
	return jsFile(myDir,jsFile::Reference).setReadOnly(flag);
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::setExecutable(bool flag){
	return jsFile(myDir,jsFile::Reference).setExecutable(flag);
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::setShowHidden(bool showHidden){
	this->showHidden = showHidden;
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::isDirectory() const {
	return std::filesystem::is_directory(myDir);
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::copyTo(const std::string& _path, bool bRelativeToData, bool overwrite){
	std::string path = _path;

	if(myDir.string().empty()){
		//ofLogError("ofDirectory") << "copyTo(): source path is empty";
		return false;
	}
	if(!std::filesystem::exists(myDir)){
		//ofLogError("ofDirectory") << "copyTo(): source directory does not exist";
		return false;
	}
	if(!std::filesystem::is_directory(myDir)){
		//ofLogError("ofDirectory") << "copyTo(): source path is not a directory";
		return false;
	}

	if(bRelativeToData){
		path = jsToDataPath(path, bRelativeToData);
		//path = GUI_getResourcePath() + path;
	}

	if(jsDirectory::doesDirectoryExist(path, bRelativeToData)){
		if(overwrite){
			jsDirectory::removeDirectory(path, true, bRelativeToData);
		}else{
			//ofLogWarning("ofDirectory") << "copyTo(): dest \"" << path << "\" already exists, set bool overwrite to true to overwrite it";
			return false;
		}
	}

	jsDirectory(path).create(true);
	// Iterate through the source directory
	for(std::filesystem::directory_iterator file(myDir); file != std::filesystem::directory_iterator(); ++file){
		auto currentPath = std::filesystem::absolute(file->path());
		auto dst = std::filesystem::path(path) / currentPath.filename();
		if(std::filesystem::is_directory(currentPath)){
			jsDirectory current(currentPath);
			// Found directory: Recursion
			if(!current.copyTo(dst.string(),false)){
				return false;
			}
		}else{
			jsFile(file->path(),jsFile::Reference).copyTo(dst.string(),false);
		}
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::moveTo(const std::string& path, bool bRelativeToData, bool overwrite){
	if(copyTo(path,bRelativeToData,overwrite)){
		return remove(true);
	}else{
		return false;
	}
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::renameTo(const std::string& path, bool bRelativeToData, bool overwrite){
	return moveTo(path, bRelativeToData, overwrite);
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::remove(bool recursive){
	if(path().empty() || !std::filesystem::exists(myDir)){
		return false;
	}

	try{
		if(recursive){
            std::filesystem::remove_all(std::filesystem::canonical(myDir));
		}else{
            std::filesystem::remove(std::filesystem::canonical(myDir));
		}
	}catch(std::exception & except){
		//ofLogError("ofDirectory") << "remove(): unable to remove file/directory: " << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::allowExt(const std::string& extension){
	if(extension == "*"){
		//ofLogWarning("ofDirectory") << "allowExt(): wildcard extension * is deprecated";
	}
	//extensions.push_back(ofToLower(extension));
	extensions.push_back(extension);
}

//------------------------------------------------------------------------------------------------------------
std::size_t jsDirectory::listDir(const std::string& directory){
	open(directory);
	return listDir();
}

template<class T, class BoolFunction>
void jsRemove(vector<T>& values, BoolFunction shouldErase) {
	values.erase(remove_if(values.begin(), values.end(), shouldErase), values.end());
}

template <class T>
std::size_t jsFind(const vector<T>& values, const T& target) {
	return distance(values.begin(), find(values.begin(), values.end(), target));
}


template <class T>
bool jsContains(const vector<T>& values, const T& target) {
	return jsFind(values, target) != values.size();
}

//------------------------------------------------------------------------------------------------------------
std::size_t jsDirectory::listDir(){
	files.clear();
	if(path().empty()){
		//ofLogError("ofDirectory") << "listDir(): directory path is empty";
		GUI_log("jsDirectory::listDir(): directory path is empty\n");
		return 0;
	}
	if(!std::filesystem::exists(myDir)){
		/*ofLogError("ofDirectory") << "listDir:() source directory does not exist: \"" << myDir << "\"";*/
		GUI_log("ofDirectory::listDir:() source directory does not exist: %s\n", myDir.string().c_str());
		return 0;
	}
	
	std::filesystem::directory_iterator end_iter;
	if ( std::filesystem::exists(myDir) && std::filesystem::is_directory(myDir) && !std::filesystem::is_symlink(myDir)){
		for( std::filesystem::directory_iterator dir_iter(myDir) ; dir_iter != end_iter ; ++dir_iter){
			files.emplace_back(dir_iter->path().string(), jsFile::Reference);
		}
	}else{
		//ofLogError("ofDirectory") << "listDir:() source directory does not exist: \"" << myDir << "\"";
		GUI_log("ofDirectory::listDir:() source directory does not exist: %s\n", myDir.string().c_str());
		return 0;
	}

	if(!showHidden){
		jsRemove(files, [](jsFile & file){
			return file.isHidden();
		});
	}


	if(!extensions.empty() && !jsContains(extensions, (string)"*")){
		jsRemove(files, [&](jsFile & file){
            return ((std::find(extensions.begin(), extensions.end(), file.getExtension()) == extensions.end()) && (!file.isDirectory()));
		});
	}        

//	if(ofGetLogLevel() == OF_LOG_VERBOSE){
//		for(int i = 0; i < (int)size(); i++){
//			ofLogVerbose() << "\t" << getName(i);
//		}
//		ofLogVerbose() << "listed " << size() << " files in \"" << originalDirectory << "\"";
//	}
//	for (int i = 0; i < (int)size(); i++) {
//		GUI_log("\t%s\n", getName(i).c_str());
//	}
//	GUI_log("listed %d files in %s\n", size(), originalDirectory.c_str());

//    std::cout << "cwd=" << originalDirectory.c_str() << " canRead:" << canRead() << " canWrite:" << canWrite() << std::endl;
    
	return size();
}

//------------------------------------------------------------------------------------------------------------
string jsDirectory::getOriginalDirectory() const {
	return originalDirectory;
}

//------------------------------------------------------------------------------------------------------------
string jsDirectory::getName(std::size_t position) const{
	return files.at(position).getFileName();
}

//------------------------------------------------------------------------------------------------------------
string jsDirectory::getPath(std::size_t position) const{
	return originalDirectory + getName(position);
}

//------------------------------------------------------------------------------------------------------------
jsFile jsDirectory::getFile(std::size_t position, jsFile::Mode mode, bool binary) const {
	jsFile file = files[position];
	file.changeMode(mode, binary);
	return file;
}

jsFile jsDirectory::operator[](std::size_t position) const {
	return getFile(position);
}

//------------------------------------------------------------------------------------------------------------
const vector<jsFile> & jsDirectory::getFiles() const{
	if(files.empty() && !myDir.empty()){
		const_cast<jsDirectory*>(this)->listDir();
	}
	return files;
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::getShowHidden() const{
	return showHidden;
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::reset(){
	close();
}

//------------------------------------------------------------------------------------------------------------
static bool natural(const jsFile& a, const jsFile& b) {
	string aname = a.getBaseName(), bname = b.getBaseName();
	int aint = atoi(aname.c_str()), bint = atoi(bname.c_str());
	if(jsToString(aint) == aname && jsToString(bint) == bname) {
		return aint < bint;
	} else {
		return a < b;
	}
}

template<class T>
void jsSort(vector<T>& values) {
	sort(values.begin(), values.end());
}

template<class T, class BoolFunction>
void jsSort(vector<T>& values, BoolFunction compare) {
	sort(values.begin(), values.end(), compare);
}

//------------------------------------------------------------------------------------------------------------
void jsDirectory::sort(){
    if(files.empty() && !myDir.empty()){
        listDir();
    }
	jsSort(files, natural);
}

//------------------------------------------------------------------------------------------------------------
jsDirectory jsDirectory::getSorted(){
    jsDirectory sorted(*this);
    sorted.listDir();
    sorted.sort();
    return sorted;
}

//------------------------------------------------------------------------------------------------------------
std::size_t jsDirectory::size() const{
	return files.size();
}

//------------------------------------------------------------------------------------------------------------
// jsDirectory Static Methods
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::removeDirectory(const std::string& _path, bool deleteIfNotEmpty, bool bRelativeToData){
	std::string path = _path;

	if(bRelativeToData){
		path = jsToDataPath(path);
		//path = GUI_getResourcePath() + path;
	}
	return jsFile(path,jsFile::Reference).remove(deleteIfNotEmpty);
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::createDirectory(const std::string& _dirPath, bool bRelativeToData, bool recursive){
	
	std::string dirPath = _dirPath;

	if(bRelativeToData){
		dirPath = jsToDataPath(dirPath);
		//dirPath = GUI_getResourcePath() + dirPath;
	}
	
	
	// on OSX,std::filesystem::create_directories seems to return false *if* the path has folders that already exist
	// and true if it doesn't
	// so to avoid unnecessary warnings on OSX, we check if it exists here:
	
	bool bDoesExistAlready = jsDirectory::doesDirectoryExist(dirPath);
	
	if (!bDoesExistAlready){
		
		bool success = false;
		try{
			if(!recursive){
				success = std::filesystem::create_directory(dirPath);
			}else{
				success = std::filesystem::create_directories(dirPath);
			}
		} catch(std::exception & except){
			//ofLogError("ofDirectory") << "createDirectory(): couldn't create directory \"" << dirPath << "\": " << except.what();
			return false;
		}
		return success;
		
	} else {
		
		// no need to create it - it already exists.
		return true;
	}



	
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::doesDirectoryExist(const std::string& _dirPath, bool bRelativeToData){
	std::string dirPath = _dirPath;
	if(bRelativeToData){
		dirPath = jsToDataPath(dirPath);
		//dirPath = GUI_getResourcePath() + dirPath;
	}
	return std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath);
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::isDirectoryEmpty(const std::string& _dirPath, bool bRelativeToData){
	std::string dirPath = _dirPath;
	if(bRelativeToData){
		dirPath = jsToDataPath(dirPath);
		//dirPath = GUI_getResourcePath() + dirPath;
	}

	if(!dirPath.empty() && std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)){
		return std::filesystem::directory_iterator(dirPath) == std::filesystem::directory_iterator();
	}
	return false;
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::operator==(const jsDirectory & dir) const{
	return getAbsolutePath() == dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::operator!=(const jsDirectory & dir) const{
	return getAbsolutePath() != dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::operator<(const jsDirectory & dir) const{
	return getAbsolutePath() < dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::operator<=(const jsDirectory & dir) const{
	return getAbsolutePath() <= dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::operator>(const jsDirectory & dir) const{
	return getAbsolutePath() > dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool jsDirectory::operator>=(const jsDirectory & dir) const{
	return getAbsolutePath() >= dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
vector<jsFile>::const_iterator jsDirectory::begin() const{
	return getFiles().begin();
}

//------------------------------------------------------------------------------------------------------------
vector<jsFile>::const_iterator jsDirectory::end() const{
	return files.end();
}

//------------------------------------------------------------------------------------------------------------
vector<jsFile>::const_reverse_iterator jsDirectory::rbegin() const{
	return getFiles().rbegin();
}

//------------------------------------------------------------------------------------------------------------
vector<jsFile>::const_reverse_iterator jsDirectory::rend() const{
	return files.rend();
}


//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- ofFilePath
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------
string jsFilePath::addLeadingSlash(const std::string& _path){
	std::string path = _path;
	auto sep = std::filesystem::path("/").make_preferred();
	if(!path.empty()){
		if(jsToString(path[0]) != sep.string()){
			path = (sep / path).string();
		}
	}
	return path;
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::addTrailingSlash(const std::string& _path){
	std::string path = _path;
	path = std::filesystem::path(path).make_preferred().string();
	auto sep = std::filesystem::path("/").make_preferred();
	if(!path.empty()){
		if(jsToString(path.back()) != sep.string()){
			path = (path / sep).string();
		}
	}
	return path;
}


//------------------------------------------------------------------------------------------------------------
string jsFilePath::getFileExt(const std::string& filename){
	return jsFile(filename,jsFile::Reference).getExtension();
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::removeExt(const std::string& filename){
	return jsFilePath::join(getEnclosingDirectory(filename,false), jsFile(filename,jsFile::Reference).getBaseName());
}


//------------------------------------------------------------------------------------------------------------
string jsFilePath::getPathForDirectory(const std::string& path){
	// if a trailing slash is missing from a path, this will clean it up
	// if it's a windows-style "\" path it will add a "\"
	// if it's a unix-style "/" path it will add a "/"
	auto sep = std::filesystem::path("/").make_preferred();
	if(!path.empty() && jsToString(path.back())!=sep.string()){
		return (std::filesystem::path(path) / sep).string();
	}else{
		return path;
	}
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::removeTrailingSlash(const std::string& _path){
	std::string path = _path;
	if(path.length() > 0 && (path[path.length() - 1] == '/' || path[path.length() - 1] == '\\')){
		path = path.substr(0, path.length() - 1);
	}
	return path;
}


//------------------------------------------------------------------------------------------------------------
string jsFilePath::getFileName(const std::string& _filePath, bool bRelativeToData){
	std::string filePath = _filePath;

	if(bRelativeToData){
		filePath = jsToDataPath(filePath);
		//filePath = GUI_getResourcePath() + filePath;
	}

	return std::filesystem::path(filePath).filename().string();
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::getBaseName(const std::string& filePath){
	return jsFile(filePath,jsFile::Reference).getBaseName();
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::getEnclosingDirectory(const std::string& _filePath, bool bRelativeToData){
	std::string filePath = _filePath;
	if(bRelativeToData){
		filePath = jsToDataPath(filePath);
		//filePath = GUI_getResourcePath() + filePath;
	}
	return addTrailingSlash(std::filesystem::path(filePath).parent_path().string());
}

//------------------------------------------------------------------------------------------------------------
bool jsFilePath::createEnclosingDirectory(const std::string& filePath, bool bRelativeToData, bool bRecursive) {
	return jsDirectory::createDirectory(jsFilePath::getEnclosingDirectory(filePath), bRelativeToData, bRecursive);
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::getAbsolutePath(const std::string& path, bool bRelativeToData){
	if(bRelativeToData){
		return jsToDataPath(path, true);
		//return GUI_getResourcePath() + path;
	}else{
		try{
			return std::filesystem::canonical(std::filesystem::absolute(path)).string();
		}catch(...){
			return std::filesystem::absolute(path).string();
		}
	}
}


//------------------------------------------------------------------------------------------------------------
bool jsFilePath::isAbsolute(const std::string& path){
	return std::filesystem::path(path).is_absolute();
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::getCurrentWorkingDirectory(){
	return std::filesystem::current_path().string();
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::join(const std::string& path1, const std::string& path2){
	return (std::filesystem::path(path1) / std::filesystem::path(path2)).string();
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::getCurrentExePath(){
	#if defined(__LINUX__) || defined(__ANDROID__)
		char buff[FILENAME_MAX];
		ssize_t size = readlink("/proc/self/exe", buff, sizeof(buff) - 1);
		if (size == -1){
			ofLogError("ofFilePath") << "getCurrentExePath(): readlink failed with error " << errno;
		}
		else{
			buff[size] = '\0';
			return buff;
		}
	#elif defined(__OSX__)
		char path[FILENAME_MAX];
		uint32_t size = sizeof(path);
		if(_NSGetExecutablePath(path, &size) != 0){
			ofLogError("ofFilePath") << "getCurrentExePath(): path buffer too small, need size " <<  size;
		}
		return path;
	#elif defined(_WIN32)
		vector<char> executablePath(MAX_PATH);
		DWORD result = ::GetModuleFileNameA(nullptr, &executablePath[0], static_cast<DWORD>(executablePath.size()));
		if(result == 0) {
			//ofLogError("ofFilePath") << "getCurrentExePath(): couldn't get path, GetModuleFileNameA failed";
		}else{
			return string(executablePath.begin(), executablePath.begin() + result);
		}
	#endif
	return "";
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::getCurrentExeDir(){
	return getEnclosingDirectory(getCurrentExePath(), false);
}

std::string jsGetEnv(const std::string & var) {
#ifdef _WIN32
	const size_t BUFSIZE = 4096;
	std::vector<char> pszOldVal(BUFSIZE, 0);
	auto size = GetEnvironmentVariableA(var.c_str(), pszOldVal.data(), BUFSIZE);
	if (size>0) {
		return std::string(pszOldVal.begin(), pszOldVal.begin() + size);
	}
	else {
		return "";
	}
#else
	auto value = getenv(var.c_str());
	if (value) {
		return value;
	}
	else {
		return "";
	}
#endif
}

//------------------------------------------------------------------------------------------------------------
string jsFilePath::getUserHomeDir(){
	#ifdef _WIN32
		// getenv will return any Environent Variable on Windows
		// USERPROFILE is the key on Windows 7 but it might be HOME
		// in other flavours of windows...need to check XP and NT...
		return jsGetEnv("USERPROFILE");
	#elif !defined(__EMSCRIPTEN__)
		struct passwd * pw = getpwuid(getuid());
		return pw->pw_dir;
	#else
		return "";
	#endif
}

string jsFilePath::makeRelative(const std::string & from, const std::string & to){
    auto pathFrom = std::filesystem::absolute( from );
    auto pathTo = std::filesystem::absolute( to );
    std::filesystem::path ret;
    std::filesystem::path::const_iterator itrFrom( pathFrom.begin() ), itrTo( pathTo.begin() );
    // Find common base
    for( std::filesystem::path::const_iterator toEnd( pathTo.end() ), fromEnd( pathFrom.end() ) ; itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo );
    // Navigate backwards in directory to reach previously found base
    for( std::filesystem::path::const_iterator fromEnd( pathFrom.end() ); itrFrom != fromEnd; ++itrFrom ){
        if( (*itrFrom) != "." ){
            ret /= "..";
        }
    }
    // Now navigate down the directory branch
    for( ; itrTo != pathTo.end() ; ++itrTo ){
        if( itrTo->string() != "."){
            ret /= *itrTo;
        }
    }

    return ret.string();
}
