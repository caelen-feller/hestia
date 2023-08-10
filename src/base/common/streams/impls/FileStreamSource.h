#pragma once

#include "File.h"
#include "StreamSource.h"

namespace hestia {
class FileStreamSource : public StreamSource {
  public:
    using Ptr = std::unique_ptr<FileStreamSource>;

    FileStreamSource(const File::Path& path);

    static Ptr create(const File::Path& path);

    virtual ~FileStreamSource();

    [[nodiscard]] IOResult read(WriteableBufferView& buffer) noexcept override;

    bool supports_seek() const override { return true; }

    void seek_to(std::size_t offset) override;

  private:
    void close();

    File m_file;
};
}  // namespace hestia