#include "MetaStorage.h"

#include "DirectoryPoller.h"
#include "Index.h"
#include "IndexerQueue.h"
#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"
#include "folder/watcher/DirectoryWatcher.h"

namespace librevault {

MetaStorage::MetaStorage(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
                         StateCollector* state_collector, QObject* parent)
    : QObject(parent) {
  index_ = new Index(params, state_collector, this);
  indexer_ = new IndexerQueue(params, ignore_list, path_normalizer, state_collector, this);
  poller_ = new DirectoryPoller(params, ignore_list, path_normalizer, this);
  watcher_ = new DirectoryWatcher(params, ignore_list, path_normalizer, this);

  if (params.secret.get_type() <= Secret::Type::ReadWrite) {
    connect(poller_, &DirectoryPoller::newPath, indexer_, &IndexerQueue::addIndexing);
    connect(watcher_, &DirectoryWatcher::newPath, indexer_, &IndexerQueue::addIndexing);

    poller_->setEnabled(true);
  }

  connect(index_, &Index::metaAdded, this, &MetaStorage::metaAdded);
  connect(index_, &Index::metaAddedExternal, this, &MetaStorage::metaAddedExternal);
}

MetaStorage::~MetaStorage() {}

bool MetaStorage::haveMeta(const Meta::PathRevision& path_revision) noexcept { return index_->haveMeta(path_revision); }

SignedMeta MetaStorage::getMeta(const Meta::PathRevision& path_revision) { return index_->getMeta(path_revision); }

SignedMeta MetaStorage::getMeta(const QByteArray& path_id) { return index_->getMetaByPathId(path_id); }

QList<SignedMeta> MetaStorage::getMeta() { return index_->getAllMeta(); }

QList<SignedMeta> MetaStorage::getExistingMeta() { return index_->getExistingMeta(); }

QList<SignedMeta> MetaStorage::getIncompleteMeta() { return index_->getIncompleteMeta(); }

void MetaStorage::putMeta(const SignedMeta& signed_meta, bool fully_assembled) {
  return index_->putMeta(signed_meta, fully_assembled);
}

QList<SignedMeta> MetaStorage::containingChunk(const QByteArray& ct_hash) { return index_->containingChunk(ct_hash); }

void MetaStorage::markAssembled(const QByteArray& path_id) { index_->setAssembled(path_id); }

bool MetaStorage::isChunkAssembled(const QByteArray& ct_hash) { return index_->isAssembledChunk(ct_hash); }

QPair<quint32, QByteArray> MetaStorage::getChunkSizeIv(const QByteArray& ct_hash) {
  return index_->getChunkSizeIv(ct_hash);
}

bool MetaStorage::putAllowed(const Meta::PathRevision& path_revision) noexcept {
  return index_->putAllowed(path_revision);
}

void MetaStorage::prepareAssemble(QByteArray normpath, Meta::Type type, bool with_removal) {
  watcher_->prepareAssemble(normpath, type, with_removal);
}

}  // namespace librevault
