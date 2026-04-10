#include "java/File.h"

#include <stdio.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "java/FileFilter.h"
#include "platform/fs/fs.h"
#include "util/StringHelpers.h"  // 4jcraft TODO

const char File::pathSeparator = '/';

const std::string File::pathRoot =
    "";  // Path root after pathSeparator has been removed

namespace {
namespace fs = std::filesystem;

fs::path ToFilesystemPath(const std::string& path) { return fs::path(path); }

std::string ToFilename(const fs::path& path) {
    const std::string filename = path.filename().string();
    return filename;
}

int64_t ToEpochMilliseconds(const fs::file_time_type& fileTime) {
    using namespace std::chrono;

    const auto systemTime = time_point_cast<milliseconds>(
        fileTime - fs::file_time_type::clock::now() + system_clock::now());
    return static_cast<int64_t>(systemTime.time_since_epoch().count());
}
}  // namespace

// Creates a new File instance from a parent abstract pathname and a child
// pathname string.
File::File(const File& parent, const std::string& child) {
    m_abstractPathName = parent.getPath() + pathSeparator + child;
}

// Creates a new File instance by converting the given pathname string into an
// abstract pathname.

File::File(const std::string& pathname) {
    if (pathname.empty()) {
        m_abstractPathName = "";
        return;
    }

    std::string fixedPath = pathname;
    for (size_t i = 0; i < fixedPath.length(); ++i) {
        if (fixedPath[i] == '\\') fixedPath[i] = '/';
    }
    size_t dpos;
    while ((dpos = fixedPath.find("//")) != std::string::npos)
        fixedPath.erase(dpos, 1);
    if (fixedPath.find("GAME:/") == 0) fixedPath = fixedPath.substr(6);
    m_abstractPathName = fixedPath;

    std::string request = std::filesystem::path(m_abstractPathName).string();
    while (!request.empty() && request[0] == '/') request.erase(0, 1);
    if (request.find("res/") == 0) request.erase(0, 4);

    std::string exeDir = PlatformFilesystem.getBasePath().string();
    std::string fileName = request;
    size_t lastSlash = fileName.find_last_of('/');
    if (lastSlash != std::string::npos)
        fileName = fileName.substr(lastSlash + 1);

    const char* bases[] = {"/",
                           "/Common/res/TitleUpdate/res/",
                           "/Common/Media/",
                           "/Common/res/",
                           "/Common/",
                           "resources/"};

    for (const char* base : bases) {
        std::string tryFull = exeDir + base + request;
        std::string tryFile = exeDir + base + fileName;
        if (PlatformFilesystem.exists(tryFull)) {
            m_abstractPathName = tryFull;
            return;
        }
        if (PlatformFilesystem.exists(tryFile)) {
            m_abstractPathName = tryFile;
            return;
        }
    }
    // #ifdef _WINDOWS64
    //     std::string path =
    //     std::filesystem::path(m_abstractPathName).string(); std::string
    //     finalPath = PlatformStorage.GetMountedPath(path.c_str()); if
    //     (finalPath.size() == 0) finalPath = path; m_abstractPathName =
    //     finalPath;
    // #endif

    /*
    std::vector<std::string> path = stringSplit( pathname, pathSeparator );

    if( path.back().compare( pathRoot ) != 0 )
    m_abstractPathName = path.back();
    else
    m_abstractPathName = "";

    path.pop_back();

    if( path.size() > 0 )
    {
    // If the last member of the vector is the root then just stop
    if( path.back().compare( pathRoot ) != 0 )
    this->parent = new File( &path );
    else
    this->parent = nullptr;
    }
    */
}

File::File(const std::string& parent,
           const std::string& child)  //: m_abstractPathName( child  )
{
    m_abstractPathName =
        pathRoot + pathSeparator + parent + pathSeparator + child;
    // this->parent = new File( parent );
}

// Creates a new File instance by converting the given path vector into an
// abstract pathname.
/*
File::File( std::vector<std::string> *path ) : parent( nullptr )
{
m_abstractPathName = path->back();
path->pop_back();

if( path->size() > 0 )
{
// If the last member of the vector is the root then just stop
if( path->back().compare( pathRoot ) != 0 )
this->parent = new File( path );
else
this->parent = nullptr;
}
}
*/

// Deletes the file or directory denoted by this abstract pathname. If this
// pathname denotes a directory, then the directory must be empty in order to be
// deleted. Returns: true if and only if the file or directory is successfully
// deleted; false otherwise
bool File::_delete() {
    std::error_code error;
    const bool result = fs::remove(ToFilesystemPath(getPath()), error);
    if (!result || error) {
#ifndef _CONTENT_PACKAGE
        printf("File::_delete - Error code %d (%#0.8X)\n", error.value(),
               error.value());
#endif
        return false;
    }
    return true;
}

// Creates the directory named by this abstract pathname.
// Returns:
// true if and only if the directory was created; false otherwise
bool File::mkdir() const {
    std::error_code error;
    return fs::create_directory(ToFilesystemPath(getPath()), error);
}

// Creates the directory named by this abstract pathname, including any
// necessary but nonexistent parent directories.  Note that if this
// operation fails it may have succeeded in creating some of the necessary
// parent directories.
//
//@return  <code>true</code> if and only if the directory was created,
//          along with all necessary parent directories; <code>false</code>
//          otherwise
//
//@throws  SecurityException
//          If a security manager exists and its <code>{@link
//          java.lang.SecurityManager#checkRead(java.lang.String)}</code>
//          method does not permit verification of the existence of the
//          named directory and all necessary parent directories; or if
//          the <code>{@link
//          java.lang.SecurityManager#checkWrite(java.lang.String)}</code>
//          method does not permit the named directory and all necessary
//          parent directories to be created
//
bool File::mkdirs() const {
    std::error_code error;
    const fs::path path = ToFilesystemPath(getPath());

    if (fs::exists(path, error)) {
        return fs::is_directory(path, error);
    }

    if (error) {
        return false;
    }

    return fs::create_directories(path, error);
}

/*
File *File::getParent() const
{
return (File *) parent;
}
*/

// Tests whether the file or directory denoted by this abstract pathname exists.
// Returns:
// true if and only if the file or directory denoted by this abstract pathname
// exists; false otherwise
bool File::exists() const {
    // TODO 4J Stu - Possible we could get an error result from something other
    // than the file not existing?
    std::error_code error;
    return fs::exists(ToFilesystemPath(getPath()), error);
}

// Tests whether the file denoted by this abstract pathname is a normal file. A
// file is normal if it is not a directory and, in addition, satisfies other
// system-dependent criteria. Any non-directory file created by a Java
// application is guaranteed to be a normal file. Returns: true if and only if
// the file denoted by this abstract pathname exists and is a normal file; false
// otherwise
bool File::isFile() const { return exists() && !isDirectory(); }

// Renames the file denoted by this abstract pathname.
// Whether or not this method can move a file from one filesystem to another is
// platform-dependent. The return value should always be checked to make sure
// that the rename operation was successful.
//
// Parameters:
// dest - The new abstract pathname for the named file
// Returns:
// true if and only if the renaming succeeded; false otherwise
bool File::renameTo(File dest) {
    std::error_code error;
    fs::rename(ToFilesystemPath(getPath()), ToFilesystemPath(dest.getPath()),
               error);
    if (error) {
        perror("File::renameTo - Error renaming file");
        return false;
    }
    return true;
}

// Returns an array of abstract pathnames denoting the files in the directory
// denoted by this abstract pathname. If this abstract pathname does not denote
// a directory, then this method returns null. Otherwise an array of File
// objects is returned, one for each file or directory in the directory.
// Pathnames denoting the directory itself and the directory's parent directory
// are not included in the result. Each resulting abstract pathname is
// constructed from this abstract pathname using the File(File, String)
// constructor. Therefore if this pathname is absolute then each resulting
// pathname is absolute; if this pathname is relative then each resulting
// pathname will be relative to the same directory.
//
// There is no guarantee that the name strings in the resulting array will
// appear in any specific order; they are not, in particular, guaranteed to
// appear in alphabetical order.
//
// Returns:
// An array of abstract pathnames denoting the files and directories in the
// directory denoted by this abstract pathname. The array will be empty if the
// directory is empty. Returns null if this abstract pathname does not denote a
// directory, or if an I/O error occurs.
std::vector<File*>* File::listFiles() const {
    std::vector<File*>* vOutput = new std::vector<File*>();

    // TODO 4J Stu - Also need to check for I/O errors?
    if (!isDirectory()) return vOutput;

    std::error_code error;
    for (fs::directory_iterator it(ToFilesystemPath(getPath()), error);
         !error && it != fs::directory_iterator(); it.increment(error)) {
        vOutput->push_back(new File(*this, ToFilename(it->path())));
    }
    return vOutput;
}

// Returns an array of abstract pathnames denoting the files and directories in
// the directory denoted by this abstract pathname that satisfy the specified
// filter. The behavior of this method is the same as that of the listFiles()
// method, except that the pathnames in the returned array must satisfy the
// filter. If the given filter is null then all pathnames are accepted.
// Otherwise, a pathname satisfies the filter if and only if the value true
// results when the FileFilter.accept(java.io.File) method of the filter is
// invoked on the pathname. Parameters: filter - A file filter Returns: An array
// of abstract pathnames denoting the files and directories in the directory
// denoted by this abstract pathname. The array will be empty if the directory
// is empty. Returns null if this abstract pathname does not denote a directory,
// or if an I/O error occurs.
std::vector<File*>* File::listFiles(FileFilter* filter) const {
    // TODO 4J Stu - Also need to check for I/O errors?
    if (!isDirectory()) return nullptr;

    std::vector<File*>* vOutput = new std::vector<File*>();

    std::error_code error;
    for (fs::directory_iterator it(ToFilesystemPath(getPath()), error);
         !error && it != fs::directory_iterator(); it.increment(error)) {
        File thisFile = File(*this, ToFilename(it->path()));
        if (filter->accept(&thisFile)) {
            vOutput->push_back(new File(thisFile));
        }
    }
    return vOutput;
}

// Tests whether the file denoted by this abstract pathname is a directory.
// Returns:
// true if and only if the file denoted by this abstract pathname exists and is
// a directory; false otherwise
bool File::isDirectory() const {
    std::error_code error;
    return fs::is_directory(ToFilesystemPath(getPath()), error);
}

// Returns the length of the file denoted by this abstract pathname. The return
// value is unspecified if this pathname denotes a directory. Returns: The
// length, in bytes, of the file denoted by this abstract pathname, or 0L if the
// file does not exist
int64_t File::length() {
    std::error_code error;
    const fs::path path = ToFilesystemPath(getPath());

    if (fs::is_regular_file(path, error)) {
        const auto size = fs::file_size(path, error);
        if (!error) {
            return static_cast<int64_t>(size);
        }
    }

    return 0;
}

// Returns the time that the file denoted by this abstract pathname was last
// modified. Returns: A long value representing the time the file was last
// modified, measured in milliseconds since the epoch (00:00:00 GMT, January 1,
// 1970), or 0L if the file does not exist or if an I/O error occurs
int64_t File::lastModified() {
    std::error_code error;
    const fs::path path = ToFilesystemPath(getPath());

    if (fs::is_regular_file(path, error)) {
        const fs::file_time_type lastWriteTime =
            fs::last_write_time(path, error);
        if (!error) {
            return ToEpochMilliseconds(lastWriteTime);
        }
    }

    return 0l;
}

const std::string File::getPath() const {
    /*
    std::string path;
    if ( parent != nullptr)
    path = parent->getPath();
    else
    path = std::string(pathRoot);

    path.push_back( pathSeparator );
    path.append(m_abstractPathName);
    */
    return m_abstractPathName;
}

std::string File::getName() const {
    unsigned int sep =
        (unsigned int)(m_abstractPathName.find_last_of(this->pathSeparator));
    return m_abstractPathName.substr(sep + 1, m_abstractPathName.length());
}

bool File::eq_test(const File& x, const File& y) {
    return x.getPath().compare(y.getPath()) == 0;
}

// 4J TODO JEV, a better hash function may be nessesary.
int File::hash_fnct(const File& k) {
    int hashCode = 0;

    // if (k->parent != nullptr)
    //	hashCode = hash_fnct(k->getParent());

    char* ref = (char*)k.m_abstractPathName.c_str();

    for (unsigned int i = 0; i < k.m_abstractPathName.length(); i++) {
        hashCode += ((hashCode * 33) + ref[i]) % 149;
    }

    return (int)hashCode;
}