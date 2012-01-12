/*
 Copyright (c) 2011, The Mineserver Project
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the The Mineserver Project nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <cmath>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <mineserver/localization.h>
#include <mineserver/network/client.h>
#include <mineserver/network/message/chat.h>
#include <mineserver/network/message/login.h>
#include <mineserver/network/message/handshake.h>
#include <mineserver/network/message/chunkprepare.h>
#include <mineserver/network/message/chunk.h>
#include <mineserver/network/message/spawnposition.h>
#include <mineserver/network/message/windowitems.h>
#include <mineserver/network/message/position.h>
#include <mineserver/network/message/orientation.h>
#include <mineserver/network/message/positionandorientation.h>
#include <mineserver/network/message/namedentityspawn.h>

#include <mineserver/network/message/mobspawn.h>
#include <mineserver/network/message/pickupspawn.h>
#include <mineserver/network/message/entity.h>
#include <mineserver/network/message/entityvelocity.h>
#include <mineserver/network/message/updatehealth.h>
#include <mineserver/network/message/timeupdate.h>
#include <mineserver/network/message/newstate.h>


#include <mineserver/network/message/destroyentity.h>
#include <mineserver/network/message/entityteleport.h>
#include <mineserver/network/message/entitylook.h>
#include <mineserver/network/message/entityrelativemove.h>
#include <mineserver/network/message/entityrelativemoveandlook.h>
#include <mineserver/network/message/digging.h>
#include <mineserver/network/message/blockplacement.h>
#include <mineserver/network/message/blockchange.h>
#include <mineserver/network/message/playerlistitem.h>
#include <mineserver/network/message/serverlistping.h>
#include <mineserver/network/message/kick.h>
#include <mineserver/game.h>
#include <mineserver/game/object/slot.h>

bool is_dead(Mineserver::Network_Client::pointer_t client) {
  return client->alive() == false;
}

void Mineserver::Game::run()
{
  for (clientList_t::iterator client_it=m_clients.begin();client_it!=m_clients.end();++client_it) {
    Mineserver::Network_Client::pointer_t client(*client_it);
    
    if (!client) {
      std::cout << "Got an invalid pointer somehow..." << std::endl;
      continue;
    }
    
    // 1200 in-game ticks = timed out, inactive ticks = ticks past since last keep-alive:
    if (client->inactiveTicks() > timeOutTicks) {
      std::cout << "Client timed-out." << std::endl;
      client->timedOut();
    }
    
    if (!client->alive()) {
      std::cout << "Found a dead client." << std::endl;
      
      if (m_clientMap.find(client) != m_clientMap.end()) {
        Mineserver::Game_Player::pointer_t player(m_clientMap[client]);
        int remaining = 0;
        int dbg = 0;
        
        std::cout << "Player " << m_clientMap[client]->getName() << " had " << m_playerMap[player].size() << " clients";
        m_playerMap[player].erase(std::remove(m_playerMap[player].begin(), m_playerMap[player].end(), client), m_playerMap[player].end());
        std::cout << ", now has " << m_playerMap[player].size() << " clients" << std::endl;
        
        if(m_playerMap[player].size() == 0) { // last client closed the connection
          m_players.erase(m_clientMap[client]->getName()); // drop player
          this->leavingPostWatcher(shared_from_this(), player);
        }
        
        m_clientMap.erase(client);
      }
      
      continue;
    }
    
    if (m_lastPlayerListItemUpdate >= playerListItemUpdateInterval)
    {
      m_lastPlayerListItemUpdate = 0;
      for (clientList_t::iterator client2_it=m_clients.begin();client2_it!=m_clients.end();++client2_it) {
        Mineserver::Network_Client::pointer_t client2(*client2_it);
        if (m_clientMap.find(client2) != m_clientMap.end()) {
          Mineserver::Game_Player::pointer_t player(m_clientMap[client2]);
          
          boost::shared_ptr<Mineserver::Network_Message_PlayerListItem> response = boost::make_shared<Mineserver::Network_Message_PlayerListItem>();
          response->mid = 0xC9;
          response->name = player->getName();
          response->online = true;
          response->ping = 0; // TODO: Calculate a player's ping
          client->outgoing().push_back(response);
          
        }
      }
    }
    else
    {
      m_lastPlayerListItemUpdate++;
    }
    
    std::cout << "There are " << client->incoming().size() << " messages." << std::endl;
    
    for (std::vector<Mineserver::Network_Message::pointer_t>::iterator message_it=client->incoming().begin();message_it!=client->incoming().end();++message_it) {
      Mineserver::Network_Message::pointer_t message = *message_it;
      m_messageWatchers[message->mid](shared_from_this(), client, message);
    }
    
    std::cout << "Watchers done." << std::endl;
    
    client->incoming().clear();
    
    client->run();
    
    client->write();
  }
  
  m_clients.erase(remove_if(m_clients.begin(), m_clients.end(), is_dead), m_clients.end());
  
  for (playerList_t::iterator player_it=m_players.begin();player_it!=m_players.end();++player_it) {
    Mineserver::Game_Player::pointer_t player(player_it->second);
    player->run();
  }
}

void Mineserver::Game::messageWatcherKeepAlive(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "KeepAlive watcher called!" << std::endl;
  
  client->resetInactiveTicks();
}

void Mineserver::Game::messageWatcherHandshake(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "Handshake watcher called!" << std::endl;
  
  boost::shared_ptr<Mineserver::Network_Message_Handshake> response = boost::make_shared<Mineserver::Network_Message_Handshake>();
  response->mid = 0x02;
  response->username = "-";
  client->outgoing().push_back(response);
}

void Mineserver::Game::messageWatcherChat(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "Chat watcher called!" << std::endl;
  
  const Mineserver::Network_Message_Chat* msg = reinterpret_cast<Mineserver::Network_Message_Chat*>(&(*message));
  
  //process the chat message to see if it's a command and transfer it to the parser
  std::string command = msg->message;
  
  Mineserver::Network_Client::pointer_t cclient = client;
  Mineserver::Game_Player::pointer_t player = getPlayerForClient(client);
  if(command[0] == '/')
  {
    std::vector<std::string> args;
    size_t pos = 0;
    while( true ) {
      size_t nextPos = command.find(" ", pos);
      if( nextPos == command.npos ){
        args.push_back( std::string( command.substr(pos)) );
        break;
      }
      args.push_back( std::string( command.substr( pos, nextPos - pos ) ) );
      pos = nextPos + 1;
    }
    std::cout << "Player Command <" << player->getName() << ">: " << command << " Arguments: ";
    unsigned short int argc = 0; 
    for (std::vector<std::string>::iterator argIt = args.begin(); argIt != args.end(); ++argIt) {
      std::cout << " [" << argc << "]=> ";
      std::cout << *argIt;
      argc++;
    }
    //begin the big messy chain of if commands
    //say works :) Usage: /say <message> - broadcasts message to all players
    if (args[0] == "/say")
    {
      
      boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
      chatMessage->mid = 0x03;
      chatMessage->message += "§5";
      chatMessage->message += "[Server]";
      for (std::vector<std::string>::iterator sayIt = args.begin() + 1; sayIt != args.end(); ++sayIt)
      {
        chatMessage->message += " " ;
        chatMessage->message += *sayIt;
      }
      
      for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) 
      {
        (*it)->outgoing().push_back(chatMessage);
      }
    } //end of /say server command
    
    //cow currently does not appear :(
    else if (args[0] == "/cow") 
    {
      for (int r = 0; r <=10; r++)
      {
        std::cout << "Attempting to spawn a cow." << std::endl;
        boost::shared_ptr<Mineserver::Network_Message_MobSpawn> MobSpawn = boost::make_shared<Mineserver::Network_Message_MobSpawn>();
        MobSpawn->mid = 0x18;
        MobSpawn->entityId = 1034;
        MobSpawn->type = 92; //a cow
        MobSpawn->x = 0;
        MobSpawn->y = 56;
        MobSpawn->z = 0;
        MobSpawn->yaw = 10;
        MobSpawn->pitch = 15;
        MobSpawn->data; //just trying to copy the metadata bits for a cow. A raw messy byte array.
        MobSpawn->data.push_back(0x00); //type byte, index 0
        MobSpawn->data.push_back(0x00); //boolean flags for fire, drowning, eating etc
        MobSpawn->data.push_back(0x21); //type short, index 1
        MobSpawn->data.push_back(0x01); //drowning counter. http://wiki.vg/Entities#Index_1.2C_short:_Drowning_counter
        MobSpawn->data.push_back(0x2c);
        MobSpawn->data.push_back(0x48); //int index 8 - potion effects(colour)
        MobSpawn->data.push_back(0x00);
        MobSpawn->data.push_back(0x00);
        MobSpawn->data.push_back(0x00);
        MobSpawn->data.push_back(0x00);
        MobSpawn->data.push_back(0x4c); //int index 12 - mob age. baby animails start at -23999
        MobSpawn->data.push_back(0x00);
        MobSpawn->data.push_back(0x00);
        MobSpawn->data.push_back(0x00);
        MobSpawn->data.push_back(0x00);
        boost::shared_ptr<Mineserver::Network_Message_EntityVelocity> EntityVelocity = boost::make_shared<Mineserver::Network_Message_EntityVelocity>();
        EntityVelocity->mid = 0x1c;
        EntityVelocity->entityId = 1034;
        EntityVelocity->velocityX = 219;
        EntityVelocity->velocityY = 0;
        EntityVelocity->velocityZ = 273;
        for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) 
        {
          (*it)->outgoing().push_back(MobSpawn);
          (*it)->outgoing().push_back(EntityVelocity);
        }
      }
    }//end of /cow
    
    //block pickups do not appear :(
    else if (args[0] == "/dirt")
    {
      boost::shared_ptr<Network_Message_Entity> Entity = boost::make_shared<Mineserver::Network_Message_Entity>();
      boost::shared_ptr<Network_Message_PickupSpawn> PickupSpawn = boost::make_shared<Mineserver::Network_Message_PickupSpawn>();
      Entity->mid = 0x1e;
      Entity->entityId = 1453;
      PickupSpawn->mid = 0x15;
      PickupSpawn->entityId = 1453; //random number out of my head;
      PickupSpawn->itemId = 3; //dirt;
      PickupSpawn->count = 1;
      PickupSpawn->data = 0; //is this needed for dirt?
      PickupSpawn->x = 1;
      PickupSpawn->y = 65;
      PickupSpawn->z = 1;
      PickupSpawn->rotation = 26;
      PickupSpawn->pitch = 20;
      PickupSpawn->roll = 8;
      
      //send the entity velocity for the block.
      boost::shared_ptr<Mineserver::Network_Message_EntityVelocity> PickupVelocity = boost::make_shared<Mineserver::Network_Message_EntityVelocity>();
      PickupVelocity->mid = 0x1c;
      PickupVelocity->entityId = 1453;
      PickupVelocity->velocityX = 1000;
      PickupVelocity->velocityY = 3344;
      PickupVelocity->velocityZ = 3434;
      for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) 
      {
        (*it)->outgoing().push_back(Entity);
        (*it)->outgoing().push_back(PickupSpawn);
        (*it)->outgoing().push_back(PickupVelocity);
      }
      
    }
    
    //sends a 0xFF packet to all clients. Usage: /kickall [reason]
    else if (args[0] == "/kickall")
    {
      boost::shared_ptr<Network_Message_Kick> KickMessage = boost::make_shared<Mineserver::Network_Message_Kick>();
      KickMessage->mid = 0xFF;
      KickMessage->reason = "§4Everyone was kicked from the server!";
      //send the kick packet to all clients.
      for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) 
      {
        (*it)->outgoing().push_back(KickMessage);
      }
      
    } //end of /kickall [reason]
    
    //doesn't work :(
    else if (args[0] == "/health") 
    {
      boost::shared_ptr<Network_Message_UpdateHealth> UpdateHealth = boost::make_shared<Mineserver::Network_Message_UpdateHealth>();
      UpdateHealth->health = 4;
      UpdateHealth->food = 3;
      UpdateHealth->foodSaturation = 4.0;
      client->outgoing().push_back(UpdateHealth);
    }
    
    //FIXME works except player spawns at original height.
    else if (args[0] == "/spawn") 
    {
      // I should be using world->spawnX.y.z.
      std::cout << "Sending player: " << player->getName() << " from " << player->getPosition().x <<", " << player->getPosition().y <<", " << player->getPosition().z << " to 0,65,0 EntityId: " << player->getEid() << std::endl;
      
      boost::shared_ptr<Mineserver::Network_Message_PositionAndOrientation> TeleportToSpawnMessage = boost::make_shared<Mineserver::Network_Message_PositionAndOrientation>();
      TeleportToSpawnMessage->mid = 0x0D;
      Mineserver::World::pointer_t world = getWorld(0);
      player->getPosition().x = world->getSpawnPosition().x; //0 --defaults
      player->getPosition().y = world->getSpawnPosition().y; //61
      player->getPosition().z = world->getSpawnPosition().z; //0 
      player->getPosition().stance = world->getSpawnPosition().y + 1.62;
      player->getPosition().yaw = 180.0;
      player->getPosition().pitch = 0;
      player->getPosition().onGround = false;
      TeleportToSpawnMessage->x = player->getPosition().x;
      TeleportToSpawnMessage->y = player->getPosition().y;
      TeleportToSpawnMessage->z = player->getPosition().z;
      TeleportToSpawnMessage->stance = player->getPosition().stance; //world->getSpawnPosition().y + 1.62;
      TeleportToSpawnMessage->yaw = player->getPosition().yaw;
      TeleportToSpawnMessage->pitch = player->getPosition().pitch;
      TeleportToSpawnMessage->onGround = player->getPosition().onGround;
      client->outgoing().push_back(TeleportToSpawnMessage);
      std::cout << "Sending Player is now at: " << player->getPosition().x <<", " << player->getPosition().y <<", " << player->getPosition().z << std::endl;
      
    }
    
    else if (args[0] == "/t" || args[0] == "/tell" || args[0] == "/msg")
    {
      std::string playername = args[1];
      std::string sender = player->getName();
      std::string message;
      for (std::vector<std::string>::iterator pmsgIt = args.begin() + 2; pmsgIt != args.end(); ++pmsgIt)
      {
        message += *pmsgIt;
        message += " ";
      }
      bool sentMessage = false;
      for (clientList_t::iterator playerIt = m_clients.begin(); playerIt != m_clients.end(); ++playerIt)
      {
        player = getPlayerForClient((*playerIt));
        if (player->getName() == playername) 
        {
          boost::shared_ptr<Mineserver::Network_Message_Chat> privateChatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
          privateChatMessage->mid = 0x03;
          privateChatMessage->message += sender;
          privateChatMessage->message += "§8 -> ";
          privateChatMessage->message += message;
          (*playerIt)->outgoing().push_back(privateChatMessage);
          sentMessage = true;
          break;
        }
        
      }
      if (!sentMessage) {
        boost::shared_ptr<Mineserver::Network_Message_Chat> privateChatMessageFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        privateChatMessageFail->mid = 0x03;
        privateChatMessageFail->message = "§cCould not find player: " + playername;
        client->outgoing().push_back(privateChatMessageFail);      }
      
    }// end of private message commands
    
    else if (args[0] == "/tpc" || args[0] == "/pos")
    {
      if (argc == 4) {
        
        int32_t tpX = atoi(args[1].c_str());
        int32_t tpY = atoi(args[2].c_str());
        int32_t tpZ = atoi(args[3].c_str());
        player->getPosition().x = tpX;
        player->getPosition().y = tpY;
        player->getPosition().z = tpZ;
        player->getPosition().stance = tpY + 1.62;
        player->getPosition().yaw = 180.0;
        player->getPosition().pitch = 0.0;
        player->getPosition().onGround = false;
        std::cout << "PositionP " << tpX << ", " << tpY << ", " << tpZ <<std::endl;
        boost::shared_ptr<Mineserver::Network_Message_PositionAndOrientation> tposTeleportMessage = boost::make_shared<Mineserver::Network_Message_PositionAndOrientation>();
        tposTeleportMessage->mid = 0x0D;
        tposTeleportMessage->x = player->getPosition().x;
        tposTeleportMessage->y = player->getPosition().y;
        tposTeleportMessage->z = player->getPosition().z;
        tposTeleportMessage->stance = player->getPosition().stance;
        tposTeleportMessage->yaw = player->getPosition().yaw;
        tposTeleportMessage->pitch = player->getPosition().pitch;
        tposTeleportMessage->onGround = player->getPosition().onGround;
        client->outgoing().push_back(tposTeleportMessage);
      }
      else 
      {
        boost::shared_ptr<Mineserver::Network_Message_Chat> tposMessageFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        tposMessageFail->mid = 0x03;
        tposMessageFail->message = "§cCorrect Usage: /tpc or /pos <x> <y> <z> *Use Interger values only*";
        client->outgoing().push_back(tposMessageFail);
      }
    }
    
    else if (args[0] == "/s" || args[0] == "/tphere")
    {
      if (argc == 2)
      {
        std::string tpherePlayerName = args[1];
        bool foundPlayer;
        for (clientList_t::iterator tphereIt = m_clients.begin(); tphereIt != m_clients.end(); ++tphereIt)
        {
          if(getPlayerForClient(*tphereIt)->getName() == tpherePlayerName)
          {
            bool foundPlayer = true;
            boost::shared_ptr<Mineserver::Network_Message_PositionAndOrientation> tphereTeleportMessage = boost::make_shared<Mineserver::Network_Message_PositionAndOrientation>();
            tphereTeleportMessage->mid = 0x0D;
            //we have to convert the values to Big endian before transmitting.
            std::cout << "Tphere player: " << getPlayerForClient(*tphereIt)->getName() << " from: (" << getPlayerForClient(*tphereIt)->getPosition().x << ", " << getPlayerForClient(*tphereIt)->getPosition().y << ", " << getPlayerForClient(*tphereIt)->getPosition().z << ") To: (" << getPlayerForClient(client)->getPosition().x << ", " << getPlayerForClient(client)->getPosition().y << ", " << getPlayerForClient(client)->getPosition().z << ") End" << std::endl;
            int32_t sX = getPlayerForClient(client)->getPosition().x;
            int32_t sY = getPlayerForClient(client)->getPosition().y;
            int32_t sZ = getPlayerForClient(client)->getPosition().z;
            tphereTeleportMessage->x = sX;
            tphereTeleportMessage->y = sY;
            tphereTeleportMessage->z = sZ;
            tphereTeleportMessage->stance = getPlayerForClient(client)->getPosition().y + 1.62;
            tphereTeleportMessage->yaw = getPlayerForClient(*tphereIt)->getPosition().yaw;
            tphereTeleportMessage->pitch = getPlayerForClient(*tphereIt)->getPosition().pitch;
            tphereTeleportMessage->onGround = getPlayerForClient(*tphereIt)->getPosition().onGround;
            (*tphereIt)->outgoing().push_back(tphereTeleportMessage);
          }
        }
        if (foundPlayer == false) 
        {
          boost::shared_ptr<Mineserver::Network_Message_Chat> tphereTeleportMessageFail = boost::make_shared<Mineserver::Network_Message_Chat>();
          tphereTeleportMessageFail->mid = 0x03;
          tphereTeleportMessageFail->message = "§cTphere: Could not find player: " + tpherePlayerName;
          client->outgoing().push_back(tphereTeleportMessageFail);
        }
      }
      else 
      {
        boost::shared_ptr<Mineserver::Network_Message_Chat> tphereTeleportMessageFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        tphereTeleportMessageFail->mid = 0x03;
        tphereTeleportMessageFail->message = "§cCorrect Usage: /s or /tphere <playername>";
        client->outgoing().push_back(tphereTeleportMessageFail);
      }
    }
    
    else if (args[0] == "/help") 
    {
      std::vector<std::string> helpOptions;
      helpOptions.push_back("/spawn - Go to 0, 61 ,0");
      helpOptions.push_back("/kickall - Kick all players from the server");
      helpOptions.push_back("/t or /msg <playername> <message> - send a private message");
      helpOptions.push_back("/tp <player> - Teleport to a player");
      helpOptions.push_back("/say <message> - Broadcast a §5[Server]§c message");
      helpOptions.push_back("/time <day/night> - Change the world time. Affects all players");
      helpOptions.push_back("/tpc or /pos <x> <y> <z> - Teleport to exact coordinates");
      helpOptions.push_back("/list - list players online. You can also use TAB");
      helpOptions.push_back("/s or /tphere <player> - Teleport a player to you");
      helpOptions.push_back("/gamemode <0 or 1> - Change to Survival(0) or Creative(1)");
      for (std::vector<std::string>::iterator helpIt = helpOptions.begin(); helpIt != helpOptions.end(); ++helpIt) {
        boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessageHelp = boost::make_shared<Mineserver::Network_Message_Chat>();
        chatMessageHelp->mid = 0x03;
        chatMessageHelp->message += "§c";
        chatMessageHelp->message += *helpIt;
        client->outgoing().push_back(chatMessageHelp);
      }
    }
    //teleporting works :)
    else if (args[0] == "/tp")
    {
      std::string teleportToPlayer = args[1];
      bool playerExists = false;
      //search for the player...
      for (clientList_t::iterator teleportPlayerToIt = m_clients.begin(); teleportPlayerToIt != m_clients.end(); ++teleportPlayerToIt)
      {
        Mineserver::Game_Player::pointer_t destplayer = getPlayerForClient((*teleportPlayerToIt));
        if (destplayer->getName() == teleportToPlayer) 
        {
          playerExists = true;
          boost::shared_ptr<Mineserver::Network_Message_PositionAndOrientation> teleportToPlayerMessage = boost::make_shared<Mineserver::Network_Message_PositionAndOrientation>();
          teleportToPlayerMessage->mid = 0x0D;
          teleportToPlayerMessage->x = destplayer->getPosition().x;
          teleportToPlayerMessage->y = destplayer->getPosition().y;
          teleportToPlayerMessage->z = destplayer->getPosition().z;
          teleportToPlayerMessage->stance = destplayer->getPosition().stance;
          teleportToPlayerMessage->yaw = destplayer->getPosition().yaw;
          teleportToPlayerMessage->pitch = destplayer->getPosition().pitch;
          teleportToPlayerMessage->onGround = destplayer->getPosition().onGround;
          client->outgoing().push_back(teleportToPlayerMessage);
          break;
        }
      }
      if (!playerExists) 
      {
        boost::shared_ptr<Mineserver::Network_Message_Chat> teleportChatMessageFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        teleportChatMessageFail->mid = 0x03;
        teleportChatMessageFail->message = "§cCould not find player: " + teleportToPlayer;
        client->outgoing().push_back(teleportChatMessageFail);
      }
      
    }
    
    else if (args[0] == "/time" && argc == 2)
    {
      int64_t tickTime;
      if (args[1] == "day") {
        tickTime = 6000; // noon
      }
      else if (args[1] == "night") {
        tickTime = 18000; //night.
      }
      else {
        boost::shared_ptr<Mineserver::Network_Message_Chat> timeChangeMessageFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        timeChangeMessageFail->mid = 0x03;
        timeChangeMessageFail->message = "Correct Usage: /time <day/night>";
        client->outgoing().push_back(timeChangeMessageFail);
      }
      
      boost::shared_ptr<Mineserver::Network_Message_TimeUpdate> timeUpdateMessage = boost::make_shared<Mineserver::Network_Message_TimeUpdate>();
      timeUpdateMessage->mid = 0x04;
      timeUpdateMessage->time = tickTime;
      for (clientList_t::iterator timeIt = m_clients.begin(); timeIt != m_clients.end(); ++timeIt) 
      {
        (*timeIt)->outgoing().push_back(timeUpdateMessage);
      }
      
    }
    
    else if (args[0] == "/list")
    {
      for (clientList_t::iterator listIt = m_clients.begin(); listIt != m_clients.end(); ++listIt)
      {
        boost::shared_ptr<Mineserver::Network_Message_Chat> listChatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
        listChatMessage->mid = 0x03;
        listChatMessage->message = getPlayerForClient(*listIt)->getName();
        for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) 
        {
          (*it)->outgoing().push_back(listChatMessage);
        }
      }
    }
    
    else if (args[0] == "/gamemode")
    {
      if (argc == 2)
      {
        uint8_t gameMode;
        if (args[1] == "0") {
          gameMode = 0;
        }
        else {
          gameMode = 1;
        }
        
        boost::shared_ptr<Mineserver::Network_Message_NewState> gamemodeChangeMessage = boost::make_shared<Mineserver::Network_Message_NewState>();
        gamemodeChangeMessage->mid = 0x46;
        gamemodeChangeMessage->reason = 3;
        gamemodeChangeMessage->mode = gameMode;
        client->outgoing().push_back(gamemodeChangeMessage);
      }
      
      else 
      {
        boost::shared_ptr<Mineserver::Network_Message_Chat> gamemodeChangeFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        gamemodeChangeFail->mid = 0x03;
        gamemodeChangeFail->message = "Correct Usage: /gamemode <0/1>";
        client->outgoing().push_back(gamemodeChangeFail);
      }
      
    } // end of if /gamemode <mode>
    
    else if (args[0] == "/chunkprepare")
    {
      if (argc == 4)
      {
        int32_t cX = atoi(args[1].c_str());
        int32_t cZ = atoi(args[2].c_str());
        bool mode;
        if (args[3] == "true")
        {
          mode = true;
        }
        else 
        {
          mode = false;
        }
        boost::shared_ptr<Mineserver::Network_Message_ChunkPrepare> chunkPrepareMessage = boost::make_shared<Mineserver::Network_Message_ChunkPrepare>();
        chunkPrepareMessage->mid = 0x32;
        chunkPrepareMessage->x = cX;
        chunkPrepareMessage->z = cZ;
        chunkPrepareMessage->mode = mode;
        for (clientList_t::iterator chunkIt = m_clients.begin(); chunkIt != m_clients.end(); ++chunkIt)
        {
          (*chunkIt)->outgoing().push_back(chunkPrepareMessage);
        }
      }
      else 
      {
        boost::shared_ptr<Mineserver::Network_Message_Chat> chunkPrepareFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        chunkPrepareFail->mid = 0x03;
        chunkPrepareFail->message = "Correct Usage: /chunkprepare <x> <z> <true/false>";
        client->outgoing().push_back(chunkPrepareFail);
      }
    }
    else if (args[0] == "/chunkdata")
    {
      if (argc == 3)
      {
        int32_t cX = atoi(args[1].c_str());
        int32_t cZ = atoi(args[2].c_str());
        boost::shared_ptr<Mineserver::Network_Message_Chunk> chunkDataMessage = boost::make_shared<Mineserver::Network_Message_Chunk>();
        Mineserver::World::pointer_t world = getWorld(0);
        
        chunkDataMessage->mid = 0x33;
        chunkDataMessage->posX = cX * 16;
        chunkDataMessage->posY = 0;
        chunkDataMessage->posZ = cZ * 16;
        chunkDataMessage->sizeX = 15;
        chunkDataMessage->sizeY = 127;
        chunkDataMessage->sizeZ = 15;
        chunkDataMessage->chunk = world->generateChunk(cX, cZ);
        for (clientList_t::iterator chunkIt = m_clients.begin(); chunkIt != m_clients.end(); ++chunkIt)
        {
          (*chunkIt)->outgoing().push_back(chunkDataMessage);
        }
      }
      else {
        {
          boost::shared_ptr<Mineserver::Network_Message_Chat> chunkDataFail = boost::make_shared<Mineserver::Network_Message_Chat>();
          chunkDataFail->mid = 0x03;
          chunkDataFail->message = "Correct Usage: /chunkdata <x> <z>";
          client->outgoing().push_back(chunkDataFail);
        }
      }
      
    }
    
    else if (args[0] == "/chunk")
    {
      Mineserver::Game_Player::pointer_t chunkplayer = getPlayerForClient((client));
      boost::shared_ptr<Mineserver::Network_Message_Chat> chunkMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
      chunkMessage->mid = 0x03;
      std::string chunkx;
      std::stringstream outx;
      outx << std::floor(chunkplayer->getPosition().x / 16);
      chunkx = outx.str();
      std::string chunkz;
      std::stringstream outz;
      outz << std::floor(chunkplayer->getPosition().z / 16);
      chunkz = outz.str();
      chunkMessage->message = "You are in chunk: " + chunkx + ", " + chunkz;
      
      client->outgoing().push_back(chunkMessage);
    }
    
    else 
    {
      boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessageError = boost::make_shared<Mineserver::Network_Message_Chat>();
      chatMessageError->mid = 0x03;
      chatMessageError->message = "§cUnknown command. Try /help for a list.";
      client->outgoing().push_back(chatMessageError);
    }
    
    
    
    
  } //end of if /command
  
  else //send the message as chat!
  {
    chatPostWatcher(shared_from_this(), getPlayerForClient(client), msg->message);
  }
  
}//end of chat watcher

void Mineserver::Game::messageWatcherLogin(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "Login watcher called!" << std::endl;
  
  const Mineserver::Network_Message_Login* msg = reinterpret_cast<Mineserver::Network_Message_Login*>(&(*message));
  
  Mineserver::World::pointer_t world = getWorld(0);
  
  std::cout << "Player login v." << msg->version << ": " << msg->username << std::endl;
  
  Mineserver::Game_Player::pointer_t player;
  if (m_players.find(msg->username) == m_players.end()) {
    player = boost::make_shared<Mineserver::Game_Player>();
    player->setName(msg->username);
    player->setEid(getNextEid());
    player->getPosition().x = world->getSpawnPosition().x;
    player->getPosition().y = world->getSpawnPosition().y;
    player->getPosition().z = world->getSpawnPosition().z;
    player->getPosition().stance = player->getPosition().y + 1.62;
    player->getPosition().onGround = true;
    addPlayer(player);
  } else {
    player = m_players[msg->username];
  }
  
  associateClient(client, player);
  
  boost::shared_ptr<Mineserver::Network_Message_Login> loginMessage = boost::make_shared<Mineserver::Network_Message_Login>();
  loginMessage->mid = 0x01;
  loginMessage->version = 22;
  loginMessage->seed = world->getWorldSeed();
  loginMessage->mode = world->getGameMode();
  loginMessage->dimension = world->getDimension();
  loginMessage->difficulty = world->getDifficulty();
  loginMessage->worldHeight = world->getWorldHeight();
  loginMessage->maxPlayers = 32; // this determines how many slots the tab window will have
  client->outgoing().push_back(loginMessage);
  
  for (int x = -15; x <= 15; ++x) {
    for (int z = -15; z <= 15; ++z) {
      boost::shared_ptr<Mineserver::Network_Message_ChunkPrepare> chunkPrepareMessage = boost::make_shared<Mineserver::Network_Message_ChunkPrepare>();
      chunkPrepareMessage->mid = 0x32;
      chunkPrepareMessage->x = x;
      chunkPrepareMessage->z = z;
      chunkPrepareMessage->mode = 1;
      client->outgoing().push_back(chunkPrepareMessage);
    }
  }
  
  for (int x = -15; x <= 15; ++x) {
    for (int z = -15; z <= 15; ++z) {
      boost::shared_ptr<Mineserver::Network_Message_Chunk> chunkMessage = boost::make_shared<Mineserver::Network_Message_Chunk>();
      chunkMessage->mid = 0x33;
      chunkMessage->posX = x * 16;
      chunkMessage->posY = 0;
      chunkMessage->posZ = z * 16;
      chunkMessage->sizeX = 15;
      chunkMessage->sizeY = 127;
      chunkMessage->sizeZ = 15;
      chunkMessage->chunk = world->generateChunk(x, z);
      client->outgoing().push_back(chunkMessage);
    }
  }
  
  boost::shared_ptr<Mineserver::Network_Message_SpawnPosition> spawnPositionMessage = boost::make_shared<Mineserver::Network_Message_SpawnPosition>();
  spawnPositionMessage->mid = 0x06;
  spawnPositionMessage->x = world->getSpawnPosition().x;
  spawnPositionMessage->y = world->getSpawnPosition().y;
  spawnPositionMessage->z = world->getSpawnPosition().z;
  client->outgoing().push_back(spawnPositionMessage);
  
  boost::shared_ptr<Mineserver::Network_Message_WindowItems> windowItemsMessage = boost::make_shared<Mineserver::Network_Message_WindowItems>();
  windowItemsMessage->mid = 0x68;
  windowItemsMessage->windowId = 0;
  windowItemsMessage->count = 44;
  windowItemsMessage->slots.resize(windowItemsMessage->count);
  //for (Network_Message_WindowItems::slotList_t::iterator it = windowItemsMessage->slots.begin(); it != windowItemsMessage->slots.end(); ++it) {
  //  (*it) = Game_Object_Slot(-1, 0, 0);
  //}
  //windowItemsMessage->slots[36].setItemId(278); windowItemsMessage->slots[36].setCount(1); windowItemsMessage->slots[36].setEnchantable(true);
  //windowItemsMessage->slots[37].setItemId(277); windowItemsMessage->slots[37].setCount(1); windowItemsMessage->slots[36].setEnchantable(true);
  //windowItemsMessage->slots[38].setItemId(279); windowItemsMessage->slots[38].setCount(1); windowItemsMessage->slots[36].setEnchantable(true);
  windowItemsMessage->slots[39].setItemId(1); windowItemsMessage->slots[39].setCount(64);
  windowItemsMessage->slots[40].setItemId(3); windowItemsMessage->slots[40].setCount(64);
  windowItemsMessage->slots[41].setItemId(5); windowItemsMessage->slots[41].setCount(64);
  windowItemsMessage->slots[42].setItemId(58); windowItemsMessage->slots[42].setCount(64);
  windowItemsMessage->slots[43].setItemId(54); windowItemsMessage->slots[43].setCount(64);
  windowItemsMessage->slots[44].setItemId(102); windowItemsMessage->slots[44].setCount(64);
  client->outgoing().push_back(windowItemsMessage);
  
  std::cout << "Spawning player at " << player->getPosition().x << "," << player->getPosition().y << "," << player->getPosition().z << std::endl;
  
  boost::shared_ptr<Mineserver::Network_Message_PositionAndOrientation> positionAndOrientationMessage = boost::make_shared<Mineserver::Network_Message_PositionAndOrientation>();
  positionAndOrientationMessage->mid = 0x0D;
  positionAndOrientationMessage->x = player->getPosition().x;
  positionAndOrientationMessage->y = player->getPosition().y;
  positionAndOrientationMessage->z = player->getPosition().z;
  positionAndOrientationMessage->stance = player->getPosition().stance;
  positionAndOrientationMessage->yaw = player->getPosition().yaw;
  positionAndOrientationMessage->pitch = player->getPosition().pitch;
  positionAndOrientationMessage->onGround = player->getPosition().onGround;
  client->outgoing().push_back(positionAndOrientationMessage);
  
  for(clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
  {
    Mineserver::Network_Client::pointer_t cclient = *it;
    boost::shared_ptr<Mineserver::Network_Message_PlayerListItem> playerListItemMessage = boost::make_shared<Mineserver::Network_Message_PlayerListItem>();
    playerListItemMessage->mid = 0xC9;
    playerListItemMessage->name = player->getName();
    playerListItemMessage->online = true;
    playerListItemMessage->ping = -1; // Note: this player shouldn't have a ping yet, so we should leave this -1
    cclient->outgoing().push_back(playerListItemMessage);
    boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
    chatMessage->mid = 0x03;
    chatMessage->message += "§e";
    chatMessage->message += msg->username;
    chatMessage->message += " joined the game.";
    cclient->outgoing().push_back(chatMessage);
  }
}

void Mineserver::Game::messageWatcherPosition(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "Position watcher called!" << std::endl;
  
  const Mineserver::Network_Message_Position* msg = reinterpret_cast<Mineserver::Network_Message_Position*>(&(*message));
  
  if (clientIsAssociated(client)) {
    Mineserver::Game_Player::pointer_t player = getPlayerForClient(client);
    Mineserver::Game_PlayerPosition position(msg->x, msg->y, msg->z, msg->stance, player->getPosition().pitch, player->getPosition().yaw, msg->onGround);
    movementPostWatcher(shared_from_this(), getPlayerForClient(client), position);
  }
}


void Mineserver::Game::messageWatcherOrientation(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "Orientation watcher called!" << std::endl;
  
  const Mineserver::Network_Message_Orientation* msg = reinterpret_cast<Mineserver::Network_Message_Orientation*>(&(*message));
  
  if (clientIsAssociated(client)) {
    Mineserver::Game_Player::pointer_t player = getPlayerForClient(client);
    Mineserver::Game_PlayerPosition position(player->getPosition().x, player->getPosition().y, player->getPosition().z, player->getPosition().stance, msg->pitch, msg->yaw, msg->onGround);
    movementPostWatcher(shared_from_this(), getPlayerForClient(client), position);
  }
}

void Mineserver::Game::messageWatcherPositionAndOrientation(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "PositionAndOrientation watcher called!" << std::endl;
  
  const Mineserver::Network_Message_PositionAndOrientation* msg = reinterpret_cast<Mineserver::Network_Message_PositionAndOrientation*>(&(*message));
  
  if (clientIsAssociated(client)) {
    Mineserver::Game_Player::pointer_t player = getPlayerForClient(client);
    Mineserver::Game_PlayerPosition position(msg->x, msg->y, msg->z, msg->stance, msg->pitch, msg->yaw, msg->onGround);
    movementPostWatcher(shared_from_this(), getPlayerForClient(client), position);
    int oldX = player->getPosition().x, oldY = player->getPosition().y, oldZ = player->getPosition().z;
    int lastChunkX = std::floor(oldX / 16);
    int lastChunkZ = std::floor(oldY / 16);
    Mineserver::World::pointer_t world = getWorld(0);
    int newX = player->getPosition().x;
    int newZ = player->getPosition().z;
    int chunkX = std::floor(oldX / 16);
    int chunkZ = std::floor(oldZ / 16);
    
    boost::shared_ptr<Mineserver::Network_Message_ChunkPrepare> chunkPrepareMessage = boost::make_shared<Mineserver::Network_Message_ChunkPrepare>();
    chunkPrepareMessage->mid = 0x32;
    chunkPrepareMessage->x = chunkX;
    chunkPrepareMessage->z = chunkZ;
    chunkPrepareMessage->mode = 1;
    client->outgoing().push_back(chunkPrepareMessage);
    
    boost::shared_ptr<Mineserver::Network_Message_Chunk> chunkMessage = boost::make_shared<Mineserver::Network_Message_Chunk>();
    chunkMessage->mid = 0x33;
    chunkMessage->posX = chunkX * 16;
    chunkMessage->posY = 0;
    chunkMessage->posZ = chunkZ * 16;
    chunkMessage->sizeX = 15;
    chunkMessage->sizeY = 127;
    chunkMessage->sizeZ = 15;
    chunkMessage->chunk = world->generateChunk(chunkX, chunkZ);
    client->outgoing().push_back(chunkMessage);
    
    std::string chunkx;
    std::stringstream outx;
    outx << chunkX;
    chunkx = outx.str();
    std::string chunkz;
    std::stringstream outz;
    outz << chunkZ;
    chunkz = outz.str();
    boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
    chatMessage->mid = 0x03;
    chatMessage->message += "§eYou generated a chunk at: " + chunkx + ", " + chunkz;
    client->outgoing().push_back(chatMessage);
  }
}

void Mineserver::Game::messageWatcherDigging(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "Digging watcher called!" << std::endl;
  
  const Mineserver::Network_Message_Digging* msg = reinterpret_cast<Mineserver::Network_Message_Digging*>(&(*message));
  if (!clientIsAssociated(client)) { return; }
  
  // status 0x00: start digging
  // status 0x02: finish digging
  // status 0x04: drop item
  // status 0x05: shoot arrow
  
  if (msg->status != 0 && msg->status != 2) { return; }
  
  Mineserver::World::pointer_t world = getWorld(0);
  
  int chunk_x, chunk_z;
  chunk_x = ((msg->x) >> 4);
  chunk_z = ((msg->z) >> 4);
  
  if (!world->hasChunk(chunk_x, chunk_z))
  {
    std::cout << "Chunk " << chunk_x << "," << chunk_z << " not found!" << std::endl;
  }
  else
  {
    Mineserver::World_Chunk::pointer_t chunk = world->getChunk(chunk_x, chunk_z);
    Mineserver::World_ChunkPosition cPosition = Mineserver::World_ChunkPosition(msg->x & 15, msg->y, msg->z & 15);
    Mineserver::WorldBlockPosition wPosition = Mineserver::WorldBlockPosition(msg->x, msg->y, msg->z);
    
    uint8_t type = chunk->getBlockType(cPosition.x, cPosition.y, cPosition.z);
    
    if (type != 0x07) // if type is not bedrock
    {
      chunk->setBlockType(cPosition.x, cPosition.y, cPosition.z, 0);
      chunk->setBlockMeta(cPosition.x, cPosition.y, cPosition.z, 0);
      
      blockBreakPostWatcher(shared_from_this(), getPlayerForClient(client), world, wPosition, chunk, cPosition);
    }
    
  }
}

void Mineserver::Game::messageWatcherBlockPlacement(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  
  const Mineserver::Network_Message_BlockPlacement* msg = reinterpret_cast<Mineserver::Network_Message_BlockPlacement*>(&(*message));
  std::cout << "BlockPlacement watcher called! " << "Item id " << int(msg->itemId);
  
  if (!clientIsAssociated(client)) { return; }
  
  Mineserver::World::pointer_t world = game->getWorld(0);
  
  int32_t translatedX = msg->x;
  int32_t translatedY = msg->y;
  int32_t translatedZ = msg->z;
  
  switch (msg->direction)
  {
    case -1:
      // Block interaction, not placement
      break;
    case 0:
      translatedY--;
      break;
    case 1:
      translatedY++;
      break;
    case 2:
      translatedZ--;
      break;
    case 3:
      translatedZ++;
      break;
    case 4:
      translatedX--;
      break;
    case 5:
      translatedX++;
      break;
  }
  
  std::cout << " at {" << int(msg->x) << "," << int(msg->y) << "," << int(msg->z) <<
  "} direction " << int(msg->direction) << " -> translated to {" <<
  int(translatedX) << "," << int(translatedY) << "," << int(translatedZ) <<
  "}" << std::endl;
  
  int chunk_x, chunk_z;
  chunk_x = ((translatedX) >> 4);
  chunk_z = ((translatedZ) >> 4);
  
  if (!world->hasChunk(chunk_x, chunk_z))
  {
    std::cout << "Chunk " << chunk_x << "," << chunk_z << " not found!" << std::endl;
  }
  else
  {
    Mineserver::World_Chunk::pointer_t chunk = world->getChunk(chunk_x, chunk_z);
    Mineserver::World_ChunkPosition cPosition = Mineserver::World_ChunkPosition(translatedX & 15, translatedY, translatedZ & 15);
    Mineserver::WorldBlockPosition wPosition = Mineserver::WorldBlockPosition(translatedX, translatedY, translatedZ);
    
    uint8_t itemId = chunk->getBlockType(cPosition.x, cPosition.y, cPosition.z);
    uint8_t itemMeta = chunk->getBlockMeta(cPosition.x, cPosition.y, cPosition.z);
    
    if (msg->y != -1 && msg->direction != -1 && msg->itemId != -1)
    {
      // TODO: We will check if there's air here or not, but later we need to move this into blockPlacePreWatcher
      if (itemId == 0)
      {
        chunk->setBlockType(cPosition.x, cPosition.y, cPosition.z, msg->itemId);
        chunk->setBlockMeta(cPosition.x, cPosition.y, cPosition.z, msg->damage);
        
        blockPlacePostWatcher(shared_from_this(), getPlayerForClient(client), world, wPosition, chunk, cPosition, msg->itemId, msg->damage);
      }
      else
      {
        std::cout << "Player tried to place a block where one already exists." << std::endl;
      }
    }
    else if (msg->x == -1 && msg->y == -1 && msg->z == -1 && msg->direction == -1)
    {
      // TODO: Do pre/post block interaction
    }
    else
    {
      std::cout << "BlockPlacement watcher is doing something funny." << std::endl;
    }
    
  }
}

void Mineserver::Game::messageWatcherBlockChange(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "BlockChange watcher called!" << std::endl;
  const Mineserver::Network_Message_BlockChange* msg = reinterpret_cast<Mineserver::Network_Message_BlockChange*>(&(*message));
  
  Mineserver::World::pointer_t world = game->getWorld(0);
  
  int chunk_x, chunk_z;
  chunk_x = ((msg->x) >> 4);
  chunk_z = ((msg->z) >> 4);
  
  if (!world->hasChunk(chunk_x, chunk_z))
  {
    std::cout << "Chunk " << chunk_x << "," << chunk_z << " not found!" << std::endl;
  }
  else
  {
    Mineserver::World_Chunk::pointer_t chunk = world->getChunk(chunk_x, chunk_z);
    Mineserver::World_ChunkPosition cPosition = Mineserver::World_ChunkPosition(msg->x & 15, msg->y, msg->z & 15);
    Mineserver::WorldBlockPosition wPosition = Mineserver::WorldBlockPosition(msg->x, msg->y, msg->z);
    
    // TODO: What is this watcher supposed to do? Minecraft doesn't seem to send this packet to us,
    //       even though the http://wiki.vg/Protocol says it can go both ways... :S
  }
}

void Mineserver::Game::messageWatcherServerListPing(Mineserver::Game::pointer_t game, Mineserver::Network_Client::pointer_t client, Mineserver::Network_Message::pointer_t message)
{
  std::cout << "ServerListPing watcher called!" << std::endl;
  
  std::stringstream reason;
  reason << "Mineserver 2.0§" << game->countPlayers() << "§" << 32; // TODO: Get max players
  
  boost::shared_ptr<Mineserver::Network_Message_Kick> response = boost::make_shared<Mineserver::Network_Message_Kick>();
  response->mid = 0xFF;
  response->reason = reason.str(); 
  client->outgoing().push_back(response);
}

bool Mineserver::Game::chatPostWatcher(Mineserver::Game::pointer_t game, Mineserver::Game_Player::pointer_t player, std::string message)
{
  boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
  chatMessage->mid = 0x03;
  chatMessage->message = "<" + player->getName() + "> " + message;
  
  for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) {
    (*it)->outgoing().push_back(chatMessage);
  }
  
  return true;
}

// START TODO:
// This entire function needs rewriting.
//
bool Mineserver::Game::movementPostWatcher(Mineserver::Game::pointer_t game, Mineserver::Game_Player::pointer_t player, Mineserver::Game_PlayerPosition position)
{
  std::cout << "movementPostWatcher called!" << std::endl;
  
  Mineserver::Game_PlayerPosition oldPos = player->getPosition();
  player->setPosition(position);
  
  boost::shared_ptr<Mineserver::Network_Message> player_move;
  double dX = (position.x - oldPos.x);
  double dY = (position.y - oldPos.y);
  double dZ = (position.z - oldPos.z);
  if( dX > 4 || dX < -4 || dY > 4 || dY < -4 || dZ > 4 || dZ < -4 )
  {
    // Entity Teleport
    boost::shared_ptr<Mineserver::Network_Message_EntityTeleport> tmp = boost::make_shared<Mineserver::Network_Message_EntityTeleport>();
    tmp->mid = 0x22;
    tmp->entityId = player->getEid();
    tmp->x      = (int32_t)(position.x * 32);
    tmp->y      = (int32_t)(position.y * 32);
    tmp->z      = (int32_t)(position.z * 32);
    tmp->pitch  = (int8_t)(position.pitch / 360 * 256);
    tmp->yaw    = (int8_t)(position.yaw / 360 * 256);
    player_move = tmp;
  }
  else if ((oldPos.yaw != position.yaw || oldPos.pitch != position.pitch)
           && (oldPos.x == position.x && oldPos.y == position.y && oldPos.z == position.z))
  {
    // Entity Look
    boost::shared_ptr<Mineserver::Network_Message_EntityLook> tmp = boost::make_shared<Mineserver::Network_Message_EntityLook>();
    tmp->mid = 0x20;
    tmp->entityId = player->getEid();
    tmp->yaw    = (int8_t)(position.yaw / 360 * 256);
    tmp->pitch  = (int8_t)(position.pitch / 360 * 256);
    player_move = tmp;
  }
  else if ((oldPos.yaw == position.yaw && oldPos.pitch == position.pitch)
           && (oldPos.x != position.x || oldPos.y != position.y || oldPos.z != position.z))
  {
    // Entity Relative Move
    boost::shared_ptr<Mineserver::Network_Message_EntityRelativeMove> tmp = boost::make_shared<Mineserver::Network_Message_EntityRelativeMove>();
    tmp->mid = 0x1F;
    tmp->entityId = player->getEid();
    tmp->x      = (int8_t)(dX * 32);
    tmp->y      = (int8_t)(dY * 32);
    tmp->z      = (int8_t)(dZ * 32);
    player_move = tmp;
  }
  else
  {
    // Entity Relative Move & Look
    boost::shared_ptr<Mineserver::Network_Message_EntityRelativeMoveAndLook> tmp = boost::make_shared<Mineserver::Network_Message_EntityRelativeMoveAndLook>();
    tmp->mid = 0x21;
    tmp->entityId = player->getEid();
    tmp->x      = (int8_t)(dX * 32);
    tmp->y      = (int8_t)(dY * 32);
    tmp->z      = (int8_t)(dZ * 32);
    tmp->yaw    = (int8_t)(position.yaw / 360 * 256);
    tmp->pitch  = (int8_t)(position.pitch / 360 * 256);
    player_move = tmp;
  }
  
  uint8_t in_distance = (16 * 3);    // 160 => 10 chunks
  uint8_t out_distance = (16 * 5);   // 192 => 12 chunks
  
  // check if we are within range of another player now
  
  double delta_x, delta_y, old_distance, new_distance;
  
  entityIdSet_t others = m_playerInRange[player];
  clientList_t other_clients;
  clientList_t my_clients = getClientsForPlayer(player);
  
  for (playerList_t::iterator player_it=m_players.begin();player_it!=m_players.end();++player_it) {
    Mineserver::Game_Player::pointer_t other(player_it->second);
    if(other == player) { continue; }
    other_clients = getClientsForPlayer(other);
    
    // calc new distance
    delta_x = position.x - other->getPosition().x;
    delta_y = position.y - other->getPosition().y;
    new_distance = sqrt(delta_x*delta_x+delta_y*delta_y);
    std::cout << " [" << other->getEid() << "] in range of [" << player->getEid() << "]?" << std::endl;
    std::cout << " [" << other->getEid() << "] distance to [" << player->getEid() << "]: " << new_distance << std::endl;
    if(others.count(other->getEid()) >= 1) {  // we are in range of this one
      if(new_distance > out_distance) { // but now we are out
        // send destroy entity 
        boost::shared_ptr<Mineserver::Network_Message_DestroyEntity> destroyEntity = boost::make_shared<Mineserver::Network_Message_DestroyEntity>();
        destroyEntity->mid = 0x1D;
        destroyEntity->entityId = player->getEid();
        for(clientList_t::iterator it=other_clients.begin();it != other_clients.end(); it++) {
          std::cout << " [" << other->getEid() << "] << destroy entity #" << player->getEid() << std::endl;
          (*it)->outgoing().push_back(destroyEntity);
        }
        // destroy entity on both sides!
        destroyEntity = boost::make_shared<Mineserver::Network_Message_DestroyEntity>();
        destroyEntity->mid = 0x1D;
        destroyEntity->entityId = other->getEid();
        for(clientList_t::iterator it=my_clients.begin();it!=my_clients.end();it++) {
          std::cout << " [" << player->getEid() << "] << destroy entity #" << other->getEid() << std::endl;
          (*it)->outgoing().push_back(destroyEntity);
        }
        // remove player from set
        others.erase(other->getEid());
      } else { // still range
        // update entity position => send 
        for(clientList_t::iterator it=other_clients.begin();it != other_clients.end(); it++) {
          (*it)->outgoing().push_back(player_move);
        }
      }
    } else { // player is NOT in range of this one
      if(new_distance < in_distance) { // but we just entered the range
        boost::shared_ptr<Mineserver::Network_Message_NamedEntitySpawn> spawnEntity = boost::make_shared<Mineserver::Network_Message_NamedEntitySpawn>();
        spawnEntity->mid = 0x14;
        spawnEntity->entityId = player->getEid();
        spawnEntity->name     = player->getName();
        spawnEntity->x        = (int32_t)position.x*32;
        spawnEntity->y        = (int32_t)position.y*32;
        spawnEntity->z        = (int32_t)position.z*32;
        spawnEntity->rotation = (int8_t)(position.yaw / 360 * 256);
        spawnEntity->pitch    = (int8_t)(position.pitch / 360 * 256);
        spawnEntity->currentItem = 0;
        for(clientList_t::iterator it=other_clients.begin();it != other_clients.end(); it++) {
          std::cout << " [" << other->getEid() << "] << spawn entity #" << player->getEid() << std::endl;
          (*it)->outgoing().push_back(spawnEntity);
        }
        spawnEntity = boost::make_shared<Mineserver::Network_Message_NamedEntitySpawn>();
        spawnEntity->mid = 0x14;
        spawnEntity->entityId = other->getEid();
        spawnEntity->name     = other->getName();
        spawnEntity->x        = (int32_t)other->getPosition().x*32;
        spawnEntity->y        = (int32_t)other->getPosition().y*32;
        spawnEntity->z        = (int32_t)other->getPosition().z*32;
        spawnEntity->rotation = (int8_t)(other->getPosition().yaw / 360 * 256);
        spawnEntity->pitch    = (int8_t)(other->getPosition().pitch / 360 * 256);
        spawnEntity->currentItem = 0;
        for(clientList_t::iterator it=my_clients.begin();it != my_clients.end(); it++) {
          std::cout << " [" << player->getEid() << "] << spawn entity #" << other->getEid() << std::endl;
          (*it)->outgoing().push_back(spawnEntity);
        }
        others.insert(other->getEid());
        m_playerInRange[other].insert(player->getEid());
      }
    }
  }
  m_playerInRange[player] = others;
  return true;
}
// END TODO

bool Mineserver::Game::blockBreakPostWatcher(Mineserver::Game::pointer_t game, Mineserver::Game_Player::pointer_t player, Mineserver::World::pointer_t world, Mineserver::WorldBlockPosition wPosition, Mineserver::World_Chunk::pointer_t chunk, Mineserver::World_ChunkPosition cPosition)
{
  std::cout << "blockBreakPostWatcher called!" << std::endl;
  
  boost::shared_ptr<Mineserver::Network_Message_BlockChange> blockChangeMessage = boost::make_shared<Mineserver::Network_Message_BlockChange>();
  blockChangeMessage->mid = 0x35;
  blockChangeMessage->x = wPosition.x;
  blockChangeMessage->y = wPosition.y;
  blockChangeMessage->z = wPosition.z;
  blockChangeMessage->type = 0;
  blockChangeMessage->meta = 0;
  
  for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) {
    (*it)->outgoing().push_back(blockChangeMessage);
  }
  
  return true;
}

bool Mineserver::Game::blockPlacePostWatcher(Mineserver::Game::pointer_t game, Mineserver::Game_Player::pointer_t player, Mineserver::World::pointer_t world, Mineserver::WorldBlockPosition wPosition, Mineserver::World_Chunk::pointer_t chunk, Mineserver::World_ChunkPosition cPosition, uint8_t type, uint8_t meta)
{
  std::cout << "blockPlacePostWatcher called!" << std::endl;
  
  boost::shared_ptr<Mineserver::Network_Message_BlockChange> blockChangeMessage = boost::make_shared<Mineserver::Network_Message_BlockChange>();
  blockChangeMessage->mid = 0x35;
  blockChangeMessage->x = wPosition.x;
  blockChangeMessage->y = wPosition.y;
  blockChangeMessage->z = wPosition.z;
  blockChangeMessage->type = type;
  blockChangeMessage->meta = meta;
  
  for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it) {
    (*it)->outgoing().push_back(blockChangeMessage);
  }
  
  return true;
}

void Mineserver::Game::leavingPostWatcher(Mineserver::Game::pointer_t game, Mineserver::Game_Player::pointer_t player)
{
  std::cout << "leavingPostWatcher called! Player: " << player->getName() << std::endl;
  
  for(clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
  {
    Mineserver::Network_Client::pointer_t cclient = *it;
    boost::shared_ptr<Mineserver::Network_Message_PlayerListItem> playerListItemMessage = boost::make_shared<Mineserver::Network_Message_PlayerListItem>();
    playerListItemMessage->mid = 0xC9;
    playerListItemMessage->name = player->getName();
    playerListItemMessage->online = false;
    playerListItemMessage->ping = -1;
    cclient->outgoing().push_back(playerListItemMessage);
    boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
    chatMessage->mid = 0x03;
    chatMessage->message += "§e";
    chatMessage->message += player->getName();
    chatMessage->message += " left the game.";
    cclient->outgoing().push_back(chatMessage);
  }
  
  clientList_t other_clients;
  for(playerList_t::iterator player_it=m_players.begin();player_it!=m_players.end();++player_it) {
    Mineserver::Game_Player::pointer_t other(player_it->second);
    if(other == player) {
      continue;
    }
    if(m_playerInRange[player].count(other->getEid()) < 1) {  // we are not in range of this one
      continue;
    }
    
    // send destroy entity
    boost::shared_ptr<Mineserver::Network_Message_DestroyEntity> destroyEntity = boost::make_shared<Mineserver::Network_Message_DestroyEntity>();
    destroyEntity->mid = 0x1D;
    destroyEntity->entityId = player->getEid();
    other_clients = getClientsForPlayer(other);
    for(clientList_t::iterator it=other_clients.begin();it != other_clients.end(); it++) {
      std::cout << " [" << other->getEid() << "] << destroy entity #" <<  player->getEid() << std::endl;
      (*it)->outgoing().push_back(destroyEntity);
    }
  }
}
