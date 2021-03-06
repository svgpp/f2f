#ifndef _F2F_API_FILE_SYSTEM_H
#define _F2F_API_FILE_SYSTEM_H

#include <cstdint>
#include <memory>
#include "f2f/Common.hpp"
#include "f2f/Defs.hpp"
#include "f2f/DirectoryIterator.hpp"
#include "f2f/FileDescriptor.hpp"

namespace f2f
{

class IStorage;

class F2F_API_DECL FileSystem
{
public:
  FileSystem(std::unique_ptr<IStorage> && storage, bool format, OpenMode openMode = OpenMode::ReadWrite);
  ~FileSystem();

  FileSystem(FileSystem const &) = delete;
  void operator =(FileSystem const &) = delete;

  OpenMode openMode() const;

  FileDescriptor open(const char * path, OpenMode openMode, bool createIfRW = true);
  FileDescriptor open(const char * path) const; // Open file in read-only mode

  void createDirectory(const char * path);
  void remove(const char * path);

  bool exists(const char * path) const;
  FileType fileType(const char * path) const;

  DirectoryIterator directoryIterator(const char * path) const;

  void check();

private:
  struct Impl;
  Impl * m_impl;
};

}

#endif