/* serverside.h   Server-side parts of Dopewars                         */
/* Copyright (C)  1998-2000  Ben Webb                                   */
/*                Email: ben@bellatrix.pcl.ox.ac.uk                     */
/*                WWW: http://bellatrix.pcl.ox.ac.uk/~ben/dopewars/     */

/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU General Public License          */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */

/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */

/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston,               */
/*                   MA  02111-1307, USA.                               */


#ifndef __SERVERSIDE_H__
#define __SERVERSIDE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dopewars.h"

#define NOFORCE    0
#define FORCECOPS  1
#define FORCEBITCH 2

#define AT_FIRST   1
#define AT_SHOOT   2

extern GSList *FirstServer;
extern char *PidFile;

void CleanUpServer();
void BreakHandle();
void ClientLeftServer(Player *Play);
void StartServer();
void StopServer();
gboolean HandleServerCommand(char *string);
void HandleNewConnection();
void ServerLoop();
void HandleServerPlayer(Player *Play);
void HandleServerMessage(gchar *buf,Player *ReallyFrom);
void FinishGame(Player *Play,char *Message);
void SendHighScores(Player *Play,char AddCurrent,char *Message);
void SendEvent(Player *To);
int SendCopOffer(Player *To,char Force);
int OfferObject(Player *To,char ForceBitch);
void SendDrugsHere(Player *To,char DisplayBusts);
void GenerateDrugsHere(Player *To,char *Deal);
void BuyObject(Player *From,char *data);
int RandomOffer(Player *To);
void HandleAnswer(Player *From,Player *To,char *answer);
void ClearPrices(Player *Play);
void StartOfficerHardass(Player *Play,int ResyncNum,
                         char *LoneMessage,char *DeputyMessage);
void OfficerHardass(Player *Play,char *LoneMessage,char *DeputyMessage);
void FireAtHardass(Player *Play,char FireType);
void FinishFightWithHardass(Player *Play,char *Message);
int LoseBitch(Player *Play,Inventory *Guns,Inventory *Drugs);
void GainBitch(Player *Play);
void AttackPlayer(Player *Attack,Player *Defend,char AttackType);
void BreakoffCombat(Player *Attack,char LeftGame);
void SetFightTimeout(Player *Play);
void ClearFightTimeout(Player *Play);
int GetMinimumTimeout(GSList *First);
GSList *HandleTimeouts(GSList *First);
gboolean CheckHighScoreFile();
int HighScoreRead(struct HISCORE *MultiScore,struct HISCORE *AntiqueScore);

#endif
