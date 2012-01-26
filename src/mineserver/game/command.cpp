/*
  Copyright (c) 2012, The Mineserver Project
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
#include <mineserver/game/command.h>
#include <mineserver/game/player.h>
#include <mineserver/game.h>
#include <mineserver/network/client.h>
#include <mineserver/network/message/chat.h>
#include <mineserver/network/message/kick.h>
#include <mineserver/network/message/positionandorientation.h>
#include <mineserver/network/message/timeupdate.h>
#include <mineserver/network/message/newstate.h>
#include <mineserver/network/message/chunkprepare.h>
#include <mineserver/network/message/chunk.h>
#include <mineserver/world.h>
#include <boost/shared_ptr.hpp> 
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <vector>
#include <fstream>

Mineserver::Command::Command(std::string cmd, boost::shared_ptr<Mineserver::Network_Client> cli, boost::shared_ptr<Mineserver::Game_Player> player, bool ig)
{
  
  command = cmd;
  inGame = ig;
  m_client = cli;
  size_t pos = 0;
  //create an argument array by looping through each word.
  while( true ) 
  {
    size_t nextPos = command.find(" ", pos);
    if( nextPos == command.npos )
    {
      args.push_back( std::string( command.substr(pos)) );
      break;
    }
    args.push_back( std::string( command.substr( pos, nextPos - pos ) ) );
    pos = nextPos + 1;
  }
  
  std::cout << "Player Command object <" << player->getName() << ">: " << command << " Arguments: ";
  argc = 0;
  for (std::vector<std::string>::iterator argIt = args.begin(); argIt != args.end(); ++argIt) 
  {
    std::cout << " [" << argc << "]=> ";
    std::cout << *argIt;
    ++argc;
  }
}

void Mineserver::Command::execute(Mineserver::Game::pointer_t game, Mineserver::Game::clientList_t m_clients, Mineserver::Network_Client::pointer_t client)
{
  boost::shared_ptr<Mineserver::Game_Player> player = game->getPlayerForClient(client);
  typedef Mineserver::Game::clientList_t clientList_t;
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
      
      for ( Mineserver::Game::clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
      {
        (*it)->outgoing().push_back(chatMessage);
      }

    } //end of /say server command

    //sends a 0xFF packet to all clients. Usage: /kickall [reason]
    else if (args[0] == "/kickall")
    {
      boost::shared_ptr<Network_Message_Kick> KickMessage = boost::make_shared<Mineserver::Network_Message_Kick>();
      KickMessage->mid = 0xFF;
      KickMessage->reason = "Everyone was kicked from the server!";
      //send the kick packet to all clients.
      for (clientList_t::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
      {
        (*it)->outgoing().push_back(KickMessage);
      }

    } //end of /kickall [reason]

    //FIXME works except player spawns at original height.
    else if (args[0] == "/spawn")
    {
      // I should be using world->spawnX.y.z.
      std::cout << "Sending player: " << player->getName() << " from " << player->getPosition().x <<", " << player->getPosition().y <<", " << player->getPosition().z << " to 0,65,0 EntityId: " << player->getEid() << std::endl;
      Mineserver::World::pointer_t world = game->getWorld(0);
      boost::shared_ptr<Mineserver::Network_Message_PositionAndOrientation> TeleportToSpawnMessage = boost::make_shared<Mineserver::Network_Message_PositionAndOrientation>();
      TeleportToSpawnMessage->mid = 0x0D;
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
        if (player->getName() == playername)
        {
          boost::shared_ptr<Mineserver::Network_Message_Chat> privateChatMessage = boost::make_shared<Mineserver::Network_Message_Chat>();
          privateChatMessage->mid = 0x03;
          privateChatMessage->message += sender;
          privateChatMessage->message += " -> ";
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
        tposMessageFail->message = "§cCorrect Usage: /tpc or /pos <x> <y> <z>";
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
          if(game->getPlayerForClient(*tphereIt)->getName() == tpherePlayerName)
          {
            bool foundPlayer = true;
            boost::shared_ptr<Mineserver::Network_Message_PositionAndOrientation> tphereTeleportMessage = boost::make_shared<Mineserver::Network_Message_PositionAndOrientation>();
            tphereTeleportMessage->mid = 0x0D;
            //we have to convert the values to Big endian before transmitting.
            std::cout << "Tphere player: " << game->getPlayerForClient(*tphereIt)->getName() << " from: ("
              << game->getPlayerForClient(*tphereIt)->getPosition().x << ", " 
              << game->getPlayerForClient(*tphereIt)->getPosition().y << ", " 
              << game->getPlayerForClient(*tphereIt)->getPosition().z << ") To: (" 
              << game->getPlayerForClient(client)->getPosition().x << ", " 
              << game->getPlayerForClient(client)->getPosition().y << ", " 
              << game->getPlayerForClient(client)->getPosition().z << ") End" << std::endl;
            int32_t sX = game->getPlayerForClient(client)->getPosition().x;
            int32_t sY = game->getPlayerForClient(client)->getPosition().y;
            int32_t sZ = game->getPlayerForClient(client)->getPosition().z;
            tphereTeleportMessage->x = sX;
            tphereTeleportMessage->y = sY;
            tphereTeleportMessage->z = sZ;
            tphereTeleportMessage->stance = game->getPlayerForClient(client)->getPosition().y + 1.62;
            tphereTeleportMessage->yaw = game->getPlayerForClient(*tphereIt)->getPosition().yaw;
            tphereTeleportMessage->pitch = game->getPlayerForClient(*tphereIt)->getPosition().pitch;
            tphereTeleportMessage->onGround = game->getPlayerForClient(*tphereIt)->getPosition().onGround;
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
      helpOptions.push_back("/say <message> - Broadcast a [Server] message");
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
      if(argc != 2)
      {
        boost::shared_ptr<Mineserver::Network_Message_Chat> tpMessageFail = boost::make_shared<Mineserver::Network_Message_Chat>();
        tpMessageFail->mid = 0x03;
        tpMessageFail->message = "§cCorrect Usage: /tp <player>";
        client->outgoing().push_back(tpMessageFail);
      }
      else
      {
        std::string teleportToPlayer = args[1];
        bool playerExists = false;
        //search for the player...
        for (clientList_t::iterator teleportPlayerToIt = m_clients.begin(); teleportPlayerToIt != m_clients.end(); ++teleportPlayerToIt)
        {
          Mineserver::Game_Player::pointer_t destplayer = game->getPlayerForClient((*teleportPlayerToIt));
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
          teleportChatMessageFail->message = "Could not find player: " + teleportToPlayer;
          client->outgoing().push_back(teleportChatMessageFail);
        }
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
        timeChangeMessageFail->message = "§cCorrect Usage: /time <day/night>";
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
        listChatMessage->message = game->getPlayerForClient(*listIt)->getName();
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
        gamemodeChangeFail->message = "§cCorrect Usage: /gamemode <0/1>";
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
        chunkPrepareFail->message = "§cCorrect Usage: /chunkprepare <x> <z> <true/false>";
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
        Mineserver::World::pointer_t world = game->getWorld(0);

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
          chunkDataFail->message = "§cCorrect Usage: /chunkdata <x> <z>";
          client->outgoing().push_back(chunkDataFail);
        }
      }

    }

    else if (args[0] == "/chunk")
    {
      Mineserver::Game_Player::pointer_t chunkplayer = player;
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
      chunkMessage->message = "§5You are in chunk: " + chunkx + ", " + chunkz;

      client->outgoing().push_back(chunkMessage);
    }
    
    else if (args[0] == "/save")
    {
      //save a chunk
      Mineserver::Game_Player::pointer_t saverplayer = game->getPlayerForClient((client));
      Mineserver::World::pointer_t world = game->getWorld(0);
      int tX = std::floor(player->getPosition().x / 16);
      int tZ = std::floor(player->getPosition().z / 16);
      Mineserver::World_Chunk::pointer_t chunk = world->getChunk(tX,tZ);
      std::cout << "\nsizeof" << sizeof *chunk << std::endl;
      std::string fileName = "c.";
      std::stringstream ssX;
      std::stringstream ssZ;
      ssX << tX;
      ssZ << tZ;
      fileName.append(ssX.str());
      fileName.append(".");
      fileName.append(ssZ.str());
      fileName.append(".cho");
      char *fileName_c = (char*)fileName.c_str();
      std::ofstream file(fileName_c, std::ios::binary);
      file.write((char*)&chunk, sizeof *chunk);
      file.close();

    }
    
    else if (args[0] == "/load")
    {
      Mineserver::Game_Player::pointer_t saverplayer = game->getPlayerForClient((client));
      Mineserver::World::pointer_t world = game->getWorld(0);
      int tX = std::floor(player->getPosition().x / 16);
      int tZ = std::floor(player->getPosition().z / 16);
      Mineserver::World_Chunk::pointer_t chunk = world->getChunk(tX,tZ);
      std::cout << "\nsizeof" << sizeof *chunk << std::endl;
      std::string fileName = "c.";
      std::stringstream ssX;
      std::stringstream ssZ;
      ssX << tX;
      ssZ << tZ;
      fileName.append(ssX.str());
      fileName.append(".");
      fileName.append(ssZ.str());
      fileName.append(".cho");
      char *fileName_c = (char*)fileName.c_str();
      std::ifstream file(fileName_c, std::ios::binary);
      std::cout << "\nAttempting to read: " << fileName_c << std::endl; 
      file.read((char*)&chunk, sizeof &chunk);
      //chunk prepare message -false - unload old
      boost::shared_ptr<Mineserver::Network_Message_ChunkPrepare> chunkPrepareUnloadMessage = boost::make_shared<Mineserver::Network_Message_ChunkPrepare>();
      chunkPrepareUnloadMessage->mid = 0x32;
      chunkPrepareUnloadMessage->x = tX;
      chunkPrepareUnloadMessage->z = tZ;
      chunkPrepareUnloadMessage->mode = 0;
      for (clientList_t::iterator chunkIt = m_clients.begin(); chunkIt != m_clients.end(); ++chunkIt)
      {
        (*chunkIt)->outgoing().push_back(chunkPrepareUnloadMessage);
      }
      //chunk prepare message -true - prepare new...
      boost::shared_ptr<Mineserver::Network_Message_ChunkPrepare> chunkPrepareReloadMessage = boost::make_shared<Mineserver::Network_Message_ChunkPrepare>();
      chunkPrepareReloadMessage->mid = 0x32;
      chunkPrepareReloadMessage->x = tX;
      chunkPrepareReloadMessage->z = tZ;
      chunkPrepareReloadMessage->mode = 1;
      for (clientList_t::iterator chunkIt = m_clients.begin(); chunkIt != m_clients.end(); ++chunkIt)
      {
        (*chunkIt)->outgoing().push_back(chunkPrepareReloadMessage);
      }
      //send the data...
      boost::shared_ptr<Mineserver::Network_Message_Chunk> chunkDataMessage = boost::make_shared<Mineserver::Network_Message_Chunk>();
      chunkDataMessage->mid = 0x33;
      chunkDataMessage->posX = tX * 16;
      chunkDataMessage->posY = 0;
      chunkDataMessage->posZ = tZ * 16;
      chunkDataMessage->sizeX = 15;
      chunkDataMessage->sizeY = 127;
      chunkDataMessage->sizeZ = 15;
      chunkDataMessage->chunk = chunk;
      for (clientList_t::iterator chunkIt = m_clients.begin(); chunkIt != m_clients.end(); ++chunkIt)
      {
        (*chunkIt)->outgoing().push_back(chunkDataMessage);
      }
    }
    
    else
    {
      boost::shared_ptr<Mineserver::Network_Message_Chat> chatMessageError = boost::make_shared<Mineserver::Network_Message_Chat>();
      chatMessageError->mid = 0x03;
      chatMessageError->message = "§cUnknown command. Try /help for a list.";
      client->outgoing().push_back(chatMessageError);
    }

}

Mineserver::Command::~Command() 
{
  
}

