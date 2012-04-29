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

#include "CommErrorHandler.hpp"
#include "DataStore.hpp"
#include "UDJServerConnection.hpp"

namespace UDJ{

CommErrorHandler::CommErrorHandler(
  DataStore *dataStore,
  UDJServerConnection *serverConnection)
  :QObject(dataStore),
  dataStore(dataStore),
  serverConnection(serverConnection),
  hasPendingReauthRequest(false),
  syncLibOnReauth(false),
  createPlayerOnReauth(false),
  refreshActivePlaylistOnReauth(false),
  syncPlaylistAddRequestsOnReauth(false),
  setCurrentSongOnReauth(false),
  syncPlaylistRemoveRequestsOnReauth(false),
  setPlayerActiveOnReauth(false)

{
  connect(
    serverConnection,
    SIGNAL(authenticated(const QByteArray&, const user_id_t&)),
    this,
    SLOT(onAuthenticated(const QByteArray&, const user_id_t&)));

  connect(
    serverConnection,
    SIGNAL(authFailed(const QString)),
    this,
    SIGNAL(hardAuthFailure(const QString)));

  connect(
    serverConnection,
    SIGNAL(commError(
      CommErrorHandler::OperationType,
      CommErrorHandler::CommErrorType,
      const QByteArray&)),
    this,
    SLOT(handleCommError(
      CommErrorHandler::OperationType,
      CommErrorHandler::CommErrorType,
      const QByteArray&)));

}

void CommErrorHandler::handleCommError(
  CommErrorHandler::OperationType opType,
  CommErrorHandler::CommErrorType errorType,
  const QByteArray& payload)
{
  DEBUG_MESSAGE("Handling error of type " << errorType << " for op type " <<
    opType);
  if(errorType == CommErrorHandler::AUTH){
    if(opType == LIB_SONG_ADD || opType == LIB_SONG_DELETE){
      syncLibOnReauth = true;
    }
    else if(opType == CREATE_PLAYER){
      DEBUG_MESSAGE("Flagging createEvent on reauth")
      createPlayerPayload = payload;
      createPlayerOnReauth = true;
    }
    else if(dataStore->isCurrentlyActive()){
      if(opType == PLAYLIST_UPDATE){
        refreshActivePlaylistOnReauth = true;
      }
      else if(opType == PLAYLIST_ADD){
        syncPlaylistAddRequestsOnReauth = true;
      }
      else if(opType == SET_CURRENT_SONG){
        setCurrentSongPayload = payload;
        setCurrentSongOnReauth = true;
      }
      else if(opType == PLAYLIST_REMOVE){
        syncPlaylistRemoveRequestsOnReauth = true;
      }
    }
    else if(opType == SET_PLAYER_ACTIVE){
      setPlayerActiveOnReauth = true;
    }
    requestReauth();
  }
  else if(errorType == CONFLICT){
  }
  else if(errorType == NOT_FOUND_ERROR){
    if(opType == LIB_SONG_DELETE){
      
    }
  }
  else if(errorType == UNKNOWN_ERROR || errorType == SERVER_ERROR){
    if(opType == CREATE_PLAYER){
      emit playerCreationFailed(tr("We're currently experiencing technical "
        "difficulties. Please try again in a minute."));
    }
  }
}

void CommErrorHandler::requestReauth(){
  if(!hasPendingReauthRequest){
    hasPendingReauthRequest = true;
    serverConnection->authenticate(
      dataStore->getUsername(),
      dataStore->getPassword());
  }
}

void CommErrorHandler::onAuthenticated(
  const QByteArray& ticket,
  const user_id_t& user_id)
{
  serverConnection->setTicket(ticket);
  serverConnection->setUserId(user_id);
  hasPendingReauthRequest = false;
  clearOnReauthFlags();
}

void CommErrorHandler::clearOnReauthFlags(){
  if(syncLibOnReauth){
    dataStore->syncLibrary();
    syncLibOnReauth=false;
  }
  if(createPlayerOnReauth){
    DEBUG_MESSAGE("Creating event on Reauth")
    serverConnection->createPlayer(createPlayerPayload);
    createPlayerOnReauth=false;
  }
  if(refreshActivePlaylistOnReauth){
    serverConnection->getActivePlaylist();
    refreshActivePlaylistOnReauth=false;
  }
  if(syncPlaylistAddRequestsOnReauth){
    //dataStore->syncPlaylistAddRequests();
    syncPlaylistAddRequestsOnReauth=false;
  }
  if(setCurrentSongOnReauth){
    serverConnection->setCurrentSong(setCurrentSongPayload);
    setCurrentSongOnReauth=false;
  }
  if(syncPlaylistRemoveRequestsOnReauth){
    dataStore->syncPlaylistRemoveRequests();
    syncPlaylistRemoveRequestsOnReauth=false;
  }
  if(setPlayerActiveOnReauth){
    dataStore->activatePlayer();
    setPlayerActiveOnReauth=false;
  }
}

void CommErrorHandler::onHardAuthFailure(const QString errMessage){
  emit hardAuthFailure(errMessage);

  if(syncLibOnReauth){
    emit libSyncError(errMessage);
  }
  if(createPlayerOnReauth){
    emit playerCreationFailed(errMessage);
  }
  if(refreshActivePlaylistOnReauth){
    emit refreshActivePlaylistError(errMessage);
  }
  if(syncPlaylistAddRequestsOnReauth){
    emit playlistAddRequestError(errMessage);
  }
  if(setCurrentSongOnReauth){
    emit setCurrentSongError(errMessage);
  }
  if(syncPlaylistRemoveRequestsOnReauth){
    emit playlistRemoveRequestError(errMessage);
  }
  if(setPlayerActiveOnReauth){
    emit setPlayerActiveError(errMessage);
  }
}


}
