#include "FileSystemImpl.hpp"
#include "Directory.hpp"
#include "Exception.hpp"
#include "File.hpp"

namespace f2f
{

static const BlockAddress RootDirectoryAddress = BlockAddress::fromBlockIndex(0);

FileSystem::FileSystem(std::unique_ptr<IStorage> && storage, bool format, OpenMode openMode)
  : m_impl(new Impl)
{
  // Check mode
  m_impl->ptr = std::make_shared<FileSystemImpl>(std::move(storage), format, openMode);
}

FileSystem::~FileSystem()
{
  delete m_impl;
}

OpenMode FileSystem::openMode() const
{
  return m_impl->ptr->m_openMode;
}

FileDescriptor FileSystem::open(const char * pathStr, OpenMode openMode, bool createIfRW)
{
  // checkOpenMode

  // Always treat relative path as relative to root
  fs::path path = fs::path(pathStr).relative_path();

  if (path.empty())
    return {};

  boost::optional<std::pair<BlockAddress, FileType>> parentDirectory = m_impl->ptr->searchFile(path.parent_path());
  if (!parentDirectory || parentDirectory->second != FileType::Directory)
    return {};

  Directory directory(m_impl->ptr->m_blockStorage, parentDirectory->first, openMode); // create option requires r/w
  // TODO: file name encoding
  std::string fileName = path.filename().generic_string();
  boost::optional<std::pair<BlockAddress, FileType>> directoryItem =
    directory.searchFile(fileName);
  if (directoryItem)
  {
    if (directoryItem->second == FileType::Directory)
      // Path is to directory
      return {};
    else
      return m_impl->ptr->openFile(directoryItem->first, openMode);
  }
  else // not found
  {
    if (openMode == OpenMode::ReadWrite && createIfRW)
    {
      std::unique_ptr<File> file(new File(m_impl->ptr->m_blockStorage));
      directory.addFile(file->inodeAddress(), FileType::Regular, fileName);
      return m_impl->ptr->openFile(file->inodeAddress(), openMode, std::move(file));
    }
    else
      // File not found
      return {};
  }
}

FileType FileSystem::fileType(const char * pathStr) const
{
  fs::path path = fs::path(pathStr).relative_path();

  if (path.empty())
    return FileType::Directory; // Root directory

  boost::optional<std::pair<BlockAddress, FileType>> file = m_impl->ptr->searchFile(path);
  if (!file)
    return FileType::NotFound;

  return file->second;
}

bool FileSystem::exists(const char * path) const
{
  return fileType(path) != FileType::NotFound;
}

void FileSystem::createDirectory(const char * pathStr)
{
  fs::path path = fs::path(pathStr).relative_path();

  if(path.empty())
    return; // Root directory already exists - ok

  boost::optional<std::pair<BlockAddress,FileType>> parentDirectory = m_impl->ptr->searchFile(path.parent_path());
  if(!parentDirectory || parentDirectory->second != FileType::Directory)
    throw std::runtime_error("Can't find path to the directory");

  std::string fileName = path.filename().generic_string();
  if (fileName == ".." || fileName == ".")
    return; 

  Directory newDirectory(m_impl->ptr->m_blockStorage, parentDirectory->first);
  Directory directory(m_impl->ptr->m_blockStorage, parentDirectory->first, OpenMode::ReadWrite); 
  try
  {
    directory.addFile(newDirectory.inodeAddress(), FileType::Directory, fileName);
  }
  catch (FileExistsError const & e)
  {
    newDirectory.remove([](BlockAddress, FileType){});
    if (e.fileType() == FileType::Directory)
      return;
    else
      throw std::runtime_error("Can't create directory. File with same name already exists");
  }
}

FileSystemImpl::FileSystemImpl(std::unique_ptr<IStorage> && storage, bool format, OpenMode openMode)
  : m_storage(std::move(storage))
  , m_blockStorage(*m_storage, format)
  , m_openMode(openMode)
{
  if (format)
  {
    Directory root(m_blockStorage, Directory::NoParentDirectory);
    assert(root.inodeAddress() == RootDirectoryAddress);
  }
}

boost::optional<std::pair<BlockAddress, FileType>> FileSystemImpl::searchFile(fs::path const & path)
{
  // Always treat relative path as relative to root
  BlockAddress currentDirectoryAddress = RootDirectoryAddress;
  for (auto pathElementIt = path.begin(); pathElementIt != path.end(); ++pathElementIt)
  {
    if (pathElementIt->generic_string() == ".")
      continue;
    bool isLast = pathElementIt == --path.end();
    Directory directory(m_blockStorage, currentDirectoryAddress, OpenMode::ReadOnly);
    // TODO: file name encoding
    boost::optional<std::pair<BlockAddress, FileType>> directoryItem =
      directory.searchFile(pathElementIt->generic_string());
    if (isLast)
      return directoryItem;

    if (!directoryItem)
      return {};
    
    if (directoryItem->second != FileType::Directory)
      // Regular file in the middle of the path
      return {};

    currentDirectoryAddress = directoryItem->first;
  }
  return std::make_pair(currentDirectoryAddress, FileType::Directory);
}

FileDescriptor FileSystemImpl::openFile(BlockAddress const & inodeAddress, OpenMode openMode, std::unique_ptr<File> && file)
{
  auto ins = m_openedFiles.insert(std::make_pair(inodeAddress, DescriptorRecord()));
  DescriptorRecord & record = ins.first->second;

  if (record.refCount > 0)
  {
    if (openMode == OpenMode::ReadWrite || record.openMode == OpenMode::ReadWrite)
      throw std::runtime_error("File is locked");
  }

  if (!file)
    file.reset(new File(m_blockStorage, inodeAddress, openMode));

  if (record.refCount++ == 0)
    record.openMode = openMode;

  return FileDescriptorFactory::create(
    std::make_shared<FileDescriptorImpl>(
      std::move(file),
      shared_from_this(),
      [this, ins, inodeAddress]() 
      {
        if (--ins.first->second.refCount == 0)
        {
          bool isDeleted = ins.first->second.fileIsDeleted;
          m_openedFiles.erase(ins.first);
          if (isDeleted)
          {
            File file(m_blockStorage, inodeAddress, OpenMode::ReadWrite);
            file.remove();
          }
        }
      }
  ));
}

}