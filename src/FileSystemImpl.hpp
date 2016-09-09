#pragma once

#include <map>
#include <memory>
#include <boost/filesystem/path.hpp>
#include "f2f/FileSystem.hpp"
#include "f2f/IStorage.hpp"
#include "BlockStorage.hpp"
#include "FileDescriptorImpl.hpp"

namespace f2f
{

namespace fs = boost::filesystem;

class FileSystemImpl: 
  public std::enable_shared_from_this<FileSystemImpl>
{
public:
  FileSystemImpl(std::unique_ptr<IStorage> && storage, bool format, OpenMode openMode);

  std::unique_ptr<IStorage> m_storage;
  BlockStorage m_blockStorage;
  OpenMode const m_openMode;

  boost::optional<std::pair<BlockAddress, FileType>> searchFile(fs::path const & path);

  FileDescriptor openFile(BlockAddress const & inodeAddress, OpenMode openMode, std::unique_ptr<File> && = std::unique_ptr<File>());

private:
  struct DescriptorRecord
  {
    DescriptorRecord()
      : refCount(0)
      , fileIsDeleted(false)
    {}

    OpenMode openMode;
    unsigned refCount;
    bool fileIsDeleted;
  };
  std::map<BlockAddress, DescriptorRecord> m_openedFiles; // key - inode block address
};

struct FileSystem::Impl
{
  std::shared_ptr<FileSystemImpl> ptr;
};

}