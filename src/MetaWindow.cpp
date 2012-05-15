/**
 * Copyright 2011 Kurtis L. Nusbaum
 *
 * This file is part of UDJ.
 *
 * UDJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * UDJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UDJ.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "MetaWindow.hpp"
#include "MusicFinder.hpp"
#include "DataStore.hpp"
#include "LibraryWidget.hpp"
#include "ActivityList.hpp"
#include "ActivePlaylistView.hpp"
#include "PlayerCreateDialog.hpp"
#include "PlayerDashboard.hpp"
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAction>
#include <QTabWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMenuBar>
#include <QLabel>
#include <QStackedWidget>
#include <QSplitter>
#include <QMessageBox>


namespace UDJ{


MetaWindow::MetaWindow(
  const QString& username,
  const QString& password,
  const QByteArray& ticketHash,
  const user_id_t& userId,
  QWidget *parent,
  Qt::WindowFlags flags)
  :QMainWindow(parent,flags),
  isQuiting(false)
{
  dataStore = new DataStore(username, password, ticketHash, userId, this);
  createActions(); setupUi();
  setupMenus();
  QSettings settings(
    QSettings::UserScope,
    DataStore::getSettingsOrg(),
    DataStore::getSettingsApp());
  restoreGeometry(settings.value("metaWindowGeometry").toByteArray());
  restoreState(settings.value("metaWindowState").toByteArray());
  if(dataStore->hasPlayerId()){
    dataStore->setPlayerState(DataStore::getPlayingState());
  }
  else{
    PlayerCreateDialog *createDialog = new PlayerCreateDialog(dataStore, this);
    createDialog->show();
  }
}

void MetaWindow::closeEvent(QCloseEvent *event){
  if(!isQuiting){
    isQuiting = true;
    connect(
      dataStore,
      SIGNAL(playerStateChanged(const QString&)),
      this,
      SLOT(close()));
    quittingProgress = new QProgressDialog("Disconnecting...", "Cancel", 0, 0, this);
    quittingProgress->setWindowModality(Qt::WindowModal);
    dataStore->setPlayerState(DataStore::getInactiveState());
    event->ignore();
  }
  else{
    QSettings settings(
      QSettings::UserScope,
      DataStore::getSettingsOrg(),
      DataStore::getSettingsApp());
    settings.setValue("metaWindowGeometry", saveGeometry());
    settings.setValue("metaWindowState", saveState());
    QMainWindow::closeEvent(event);
  }
}

void MetaWindow::addMusicToLibrary(){
  //TODO: Check to see if musicDir is different than then current music dir
  QDir musicDir = QFileDialog::getExistingDirectory(this,
    tr("Pick folder to add"),
    QDir::homePath(),
    QFileDialog::ShowDirsOnly);
  QList<Phonon::MediaSource> musicToAdd =
    MusicFinder::findMusicInDir(musicDir.absolutePath());
  if(musicToAdd.isEmpty()){
    return;
  }
  int numNewFiles = musicToAdd.size();
  addingProgress = new QProgressDialog(
    "Loading Library...", "Cancel", 0, numNewFiles+1, this);
  addingProgress->setWindowModality(Qt::WindowModal);
  connect(
    dataStore,
    SIGNAL(libSongsModified()),
    this,
    SLOT(doneAdding()));

  connect(
    dataStore,
    SIGNAL(libModError(const QString&)),
    this,
    SLOT(errorAdding(const QString&)));

  dataStore->addMusicToLibrary(musicToAdd, addingProgress);
  addingProgress->setLabelText(tr("Syncing With Server"));
}

void MetaWindow::doneAdding(){
  disconnect(
    dataStore,
    SIGNAL(libSongsModified()),
    this,
    SLOT(doneAdding()));
  disconnect(
    dataStore,
    SIGNAL(libModError(const QString&)),
    this,
    SLOT(errorAdding(const QString&)));
  addingProgress->close();
}

void MetaWindow::errorAdding(const QString& errMessage){
  disconnect(
    dataStore,
    SIGNAL(libSongsModified()),
    this,
    SLOT(doneAdding()));
  disconnect(
    dataStore,
    SIGNAL(libModError(const QString&)),
    this,
    SLOT(errorAdding(const QString&)));
  addingProgress->close();
  QMessageBox::critical(this, "Error", "Error adding songs. Try again in a little bit.");
}


void MetaWindow::addSongToLibrary(){
  QString fileName = QFileDialog::getOpenFileName(
      this,
      tr("Pick song to add"),
      QDir::homePath(),
      tr("Audio Files ") + MusicFinder::getMusicFileExtFilter());
  QList<Phonon::MediaSource> songList;
  songList.append(Phonon::MediaSource(fileName));
  dataStore->addMusicToLibrary(songList);
}

void MetaWindow::setupUi(){

  playbackWidget = new PlaybackWidget(dataStore, this);

  libraryWidget = new LibraryWidget(dataStore, this);

  activityList = new ActivityList(dataStore);

  playlistView = new ActivePlaylistView(dataStore, this);

  QWidget* contentStackContainer = new QWidget(this);
  contentStack = new QStackedWidget(this);
  contentStack->addWidget(libraryWidget);
  contentStack->addWidget(playlistView);
  contentStack->setCurrentWidget(libraryWidget);
  QVBoxLayout *contentStackLayout = new QVBoxLayout;
  contentStackLayout->addWidget(contentStack, Qt::AlignCenter);
  contentStackContainer->setLayout(contentStackLayout);

  QSplitter *content = new QSplitter(Qt::Horizontal, this);
  content->addWidget(activityList);
  content->addWidget(contentStackContainer);
  content->setStretchFactor(1, 10);

  dashboard = new PlayerDashboard(dataStore, this);


  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(dashboard);
  mainLayout->addWidget(content,6);
  mainLayout->addWidget(playbackWidget);

  QWidget* widget = new QWidget;
  widget->setLayout(mainLayout);

  setCentralWidget(widget);
  setWindowTitle("UDJ");

  connect(
    activityList,
    SIGNAL(libraryClicked()),
    this,
    SLOT(displayLibrary()));

  connect(
    activityList,
    SIGNAL(playlistClicked()),
    this,
    SLOT(displayPlaylist()));

}

void MetaWindow::createActions(){
  quitAction = new QAction(tr("&Quit"), this);
  quitAction->setShortcuts(QKeySequence::Quit);
  addMusicAction = new QAction(tr("Add &Music"), this);
  addMusicAction->setShortcut(tr("Ctrl+M"));
  addSongAction = new QAction(tr("A&dd Song"), this);
  addSongAction->setShortcut(tr("Ctrl+D"));
  connect(addMusicAction, SIGNAL(triggered()), this, SLOT(addMusicToLibrary()));
  connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
  connect(addSongAction, SIGNAL(triggered()), this, SLOT(addSongToLibrary()));
}

void MetaWindow::setupMenus(){
  QMenu *musicMenu = menuBar()->addMenu(tr("&Music"));
  musicMenu->addAction(addMusicAction);
  musicMenu->addAction(addSongAction);
  musicMenu->addSeparator();
  musicMenu->addAction(quitAction);
}


void MetaWindow::displayLibrary(){
  contentStack->setCurrentWidget(libraryWidget);
}

void MetaWindow::displayPlaylist(){
  contentStack->setCurrentWidget(playlistView);
}


} //end namespace