#include "FolderProperties.h"

#include <QClipboard>
#include <QFileIconProvider>
#include <QJsonArray>
#include <QShowEvent>

#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "control/RemoteState.h"
#include "model/FolderModel.h"
#include "model/PeerModel.h"
#include "util/human_size.h"

FolderProperties::FolderProperties(QByteArray folderid, Daemon* daemon, FolderModel* folder_model, QWidget* parent)
    : QDialog(parent), daemon_(daemon), folder_model_(folder_model), folderid_(folderid) {
  ui.setupUi(this);

  // peer_model_ = std::make_unique<PeerModel>(this);
  ui.peers_treeView->setModel(folder_model_->getPeerModel(folderid_));
  connect(ui.peers_treeView->model(), &QObject::destroyed, this, &QObject::deleteLater);

#ifdef Q_OS_MAC
  ui.tabWidget->setDocumentMode(false);
#endif

  ui.folder_icon->setPixmap(QFileIconProvider().icon(QFileIconProvider::Folder).pixmap(32, 32));
  ui.folder_name->setText(daemon_->config()->getFolderValue(folderid_, "path").toString());

  init_secrets();

  connect(ui.copy_rw, &QAbstractButton::clicked,
          [this]() { QApplication::clipboard()->setText(ui.secret_rw->text()); });
  connect(ui.copy_ro, &QAbstractButton::clicked,
          [this]() { QApplication::clipboard()->setText(ui.secret_ro->text()); });
  connect(ui.copy_do, &QAbstractButton::clicked,
          [this]() { QApplication::clipboard()->setText(ui.secret_do->text()); });

  this->setWindowFlags(Qt::Tool);
  ui.folder_icon->setPixmap(QFileIconProvider().icon(QFileIconProvider::Folder).pixmap(QSize(32, 32)));

  setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
  setAttribute(Qt::WA_DeleteOnClose, true);

  show();

  connect(daemon_->state(), &RemoteState::changed, this, &FolderProperties::refresh);
  connect(daemon_->config(), &RemoteConfig::changed, this, &FolderProperties::refresh);
  refresh();
}

FolderProperties::~FolderProperties() {}

void FolderProperties::init_secrets() {
  librevault::Secret secret = daemon_->config()->getFolderValue(folderid_, "secret").toString();

  if (secret.get_type() <= secret.ReadWrite)
    ui.secret_rw->setText(secret.string());
  else {
    ui.label_rw->setVisible(false);
    ui.secret_rw->setVisible(false);
    ui.copy_rw->setVisible(false);
  }

  if (secret.get_type() <= secret.ReadOnly)
    ui.secret_ro->setText(secret.derive(secret.ReadOnly).string());
  else {
    ui.label_ro->setVisible(false);
    ui.secret_ro->setVisible(false);
    ui.copy_ro->setVisible(false);
  }

  ui.secret_do->setText(secret.derive(secret.Download).string());
}

void FolderProperties::refresh() {
  QJsonObject index = daemon_->state()->getFolderValue(folderid_, "index").toObject();
  ui.folder_size->setText(tr("%n file(s)", "", index["0"].toInt()) + " " +
                          tr("%n directory(s)", "", index["1"].toInt()));
  ui.connected_counter->setText(
      tr("%n peer(s)", "", daemon_->state()->getFolderValue(folderid_, "peers").toArray().size()));
}
