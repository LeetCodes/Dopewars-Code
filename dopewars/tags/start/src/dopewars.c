/* dopewars.c    dopewars - general purpose routines and initialisation */ 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dopewars.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <curses_client.h>
#include <gtk_client.h>
#include <win32_client.h>
#include <glib.h>
#include "dopeos.h"
#include "message.h"
#include "serverside.h"
#include "AIPlayer.h"

int ClientSock,ListenSock;     
char Network,Client,Server,NotifyMetaServer,AIPlayer;
/* dopewars acting as standalone TCP server:
             Network=Server=TRUE   Client=FALSE
   dopewars acting as client, connecting to standalone server:
             Network=Client=TRUE   Server=FALSE
   dopewars in single-player or antique mode:
             Network=Server=Client=FALSE
*/
int Port=7902,Sanitized=0,ConfigVerbose=0;
char *HiScoreFile=NULL,*ServerName=NULL,*Pager=NULL;
char WantHelp,WantVersion,WantAntique,WantColour,WantNetwork;
char WantedClient;
int NumLocation=0,NumGun=0,NumDrug=0,NumSubway=0,NumPlaying=0,NumStoppedTo=0;
Player Noone;
int LoanSharkLoc=DEFLOANSHARK,BankLoc=DEFBANK,GunShopLoc=DEFGUNSHOP,
    RoughPubLoc=DEFROUGHPUB;
int DrugSortMethod=DS_ATOZ;
int FightTimeout=5,IdleTimeout=14400,ConnectTimeout=300;
int MaxClients=20,AITurnPause=5;
price_t StartCash=2000,StartDebt=5500;
GSList *ServerList=NULL;

GScannerConfig ScannerConfig = {
   " \t\n",   /* Ignore these characters */
   G_CSET_a_2_z "_" G_CSET_A_2_Z,             /* Valid characters for starting 
                                                  an identifier */
   G_CSET_a_2_z "._-0123456789" G_CSET_A_2_Z, /* Valid characters for
                                                 continuing an identifier */
   "#\n", /* Single line comments start with # and end with \n */
   FALSE, /* Are symbols case sensitive? */
   TRUE,  /* Ignore C-style comments? */
   TRUE,  /* Ignore single-line comments? */
   TRUE,  /* Treat C-style comments as single tokens - do not break into
             words? */
   TRUE,  /* Read identifiers as tokens? */
   TRUE,  /* Read single-character identifiers as 1-character strings? */
   TRUE,  /* Allow the parsing of NULL as the G_TOKEN_IDENTIFIER_NULL ? */
   FALSE, /* Allow symbols (defined by g_scanner_scope_add_symbol) ? */
   TRUE,  /* Allow binary numbers in 0b1110 format ? */
   TRUE,  /* Allow octal numbers in C-style e.g. 034 ? */
   FALSE, /* Allow floats? */
   TRUE,  /* Allow hex numbers in C-style e.g. 0xFF ? */
   TRUE,  /* Allow hex numbers in $FF format ? */
   TRUE,  /* Allow '' strings (no escaping) ? */
   TRUE,  /* Allow "" strings (\ escapes parsed) ? */
   TRUE,  /* Convert octal, binary and hex to int? */
   FALSE, /* Convert ints to floats? */
   FALSE, /* Treat all identifiers as strings? */
   TRUE,  /* Leave single characters (e.g. {,=) unchanged, instead of
             returning G_TOKEN_CHAR ? */
   FALSE, /* Replace read symbols with the token given by their value, instead
             of G_TOKEN_SYMBOL ? */
   FALSE  /* scope_0_fallback... */
};

struct LOCATION StaticLocation,*Location=NULL;
struct DRUG StaticDrug,*Drug=NULL;
struct GUN StaticGun,*Gun=NULL;
struct COPS Cops = { 70,2,65,2,5,2,30 };
struct NAMES Names = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                       NULL,NULL,NULL,NULL };
struct NAMES DefaultNames = {
   N_("bitch"),N_("bitches"),N_("gun"),N_("guns"),N_("drug"),N_("drugs"),
   N_("12-"),N_("-1984"),
   N_("Hardass"),N_("Bob"),N_("the Loan Shark"),N_("the Bank"),
   N_("Dan\'s House of Guns"),N_("the pub")
};
struct PRICES Prices = {
   20000,10000
};
struct BITCH Bitch = {
   50000,150000
};
struct METASERVER MetaServer = { 0,0,0,NULL,NULL,NULL,NULL,NULL };
struct METASERVER DefaultMetaServer = {
   1,80,7802,"bellatrix.pcl.ox.ac.uk","/~ben/cgi-bin/server.pl","","",
   "dopewars server"
};
int NumTurns=31;

struct GLOBALS Globals[NUMGLOB] = {
   { &Port,NULL,NULL,NULL,"Port",N_("Network port to connect to"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&HiScoreFile,NULL,"HiScoreFile",
     N_("Name of the high score file"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&ServerName,NULL,"Server",N_("Name of the server to connect to"),
     NULL,NULL,0,"",NULL,NULL },
   { &MetaServer.Active,NULL,NULL,NULL,"MetaServer.Active",
     N_("Non-zero if server should report to a metaserver"),
     NULL,NULL,0,"",NULL,NULL },
   { &MetaServer.HttpPort,NULL,NULL,NULL,"MetaServer.HttpPort",
     N_("Port for metaserver communication (client)"),
     NULL,NULL,0,"",NULL,NULL },
   { &MetaServer.UdpPort,NULL,NULL,NULL,"MetaServer.UdpPort",
     N_("Port for metaserver communication (server)"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&MetaServer.Name,NULL,"MetaServer.Name",
     N_("Metaserver name to report server details to"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&MetaServer.Path,NULL,"MetaServer.Path",
     N_("Path of the CGI script on the metaserver (client)"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&MetaServer.LocalName,NULL,"MetaServer.LocalName",
     N_("Preferred hostname of your server machine"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&MetaServer.Password,NULL,"MetaServer.Password",
     N_("Authentication for LocalName with the metaserver"),NULL,NULL,0,"",NULL,
     NULL },
   { NULL,NULL,&MetaServer.Comment,NULL,"MetaServer.Comment",
     N_("Server description, reported to the metaserver"),NULL,NULL,0,"",NULL,
     NULL },
   { NULL,NULL,&Pager,NULL,"Pager",
     N_("Program used to display multi-page output"),NULL,NULL,0,"",NULL,NULL },
   { &NumTurns,NULL,NULL,NULL,"NumTurns",
     N_("No. of game turns (if 0, game never ends)"),
     NULL,NULL,0,"",NULL,NULL },
   { &Sanitized,NULL,NULL,NULL,"Sanitized",N_("Random events are sanitized"),
     NULL,NULL,0,"",NULL,NULL },
   { &ConfigVerbose,NULL,NULL,NULL,"ConfigVerbose",
     N_("Be verbose in processing config file"),NULL,NULL,0,"",NULL,NULL },
   { &NumLocation,NULL,NULL,NULL,"NumLocation",
     N_("Number of locations in the game"),
     (void **)(&Location),NULL,sizeof(struct LOCATION),"",NULL,
     ResizeLocations },
   { &NumGun,NULL,NULL,NULL,"NumGun",N_("Number of guns in the game"),
     (void **)(&Gun),NULL,sizeof(struct GUN),"",NULL,ResizeGuns },
   { &NumDrug,NULL,NULL,NULL,"NumDrug",N_("Number of drugs in the game"),
     (void **)(&Drug),NULL,sizeof(struct DRUG),"",NULL,ResizeDrugs },
   { &LoanSharkLoc,NULL,NULL,NULL,"LoanShark",N_("Location of the Loan Shark"),
     NULL,NULL,0,"",NULL,NULL },
   { &BankLoc,NULL,NULL,NULL,"Bank",N_("Location of the bank"),
     NULL,NULL,0,"",NULL,NULL },
   { &GunShopLoc,NULL,NULL,NULL,"GunShop",N_("Location of the gun shop"),
     NULL,NULL,0,"",NULL,NULL },
   { &RoughPubLoc,NULL,NULL,NULL,"RoughPub",N_("Location of the pub"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.LoanSharkName,NULL,"LoanSharkName",
     N_("Name of the loan shark"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.BankName,NULL,"BankName",
     N_("Name of the bank"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.GunShopName,NULL,"GunShopName",
     N_("Name of the gun shop"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.RoughPubName,NULL,"RoughPubName",
     N_("Name of the pub"),NULL,NULL,0,"",NULL,NULL },
   { &DrugSortMethod,NULL,NULL,NULL,"DrugSortMethod",
     N_("Sort key for listing available drugs"),
     NULL,NULL,0,"",NULL,NULL },
   { &FightTimeout,NULL,NULL,NULL,"FightTimeout",
     N_("No. of seconds in which to return fire"),
     NULL,NULL,0,"",NULL,NULL },
   { &IdleTimeout,NULL,NULL,NULL,"IdleTimeout",
     N_("Players are disconnected after this many seconds"),
     NULL,NULL,0,"",NULL,NULL },
   { &ConnectTimeout,NULL,NULL,NULL,"ConnectTimeout",
     N_("Time in seconds for connections to be made or broken"),
     NULL,NULL,0,"",NULL,NULL },
   { &MaxClients,NULL,NULL,NULL,"MaxClients",
     N_("Maximum number of TCP/IP connections"),
     NULL,NULL,0,"",NULL,NULL },
   { &AITurnPause,NULL,NULL,NULL,"AITurnPause",
     N_("Seconds between turns of AI players"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,&StartCash,NULL,NULL,"StartCash",
     N_("Amount of cash that each player starts with"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,&StartDebt,NULL,NULL,"StartDebt",
     N_("Amount of debt that each player starts with"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&StaticLocation.Name,NULL,"Name",N_("Name of each location"),
     (void **)(&Location),&StaticLocation,
     sizeof(struct LOCATION),"Location",&NumLocation,NULL },
   { &(StaticLocation.PolicePresence),NULL,NULL,NULL,"PolicePresence",
     N_("Police presence at each location (%)"),
     (void **)(&Location),&StaticLocation,
     sizeof(struct LOCATION),"Location",&NumLocation,NULL },
   { &(StaticLocation.MinDrug),NULL,NULL,NULL,"MinDrug",
     N_("Minimum number of drugs at each location"),
     (void **)(&Location),&StaticLocation,
     sizeof(struct LOCATION),"Location",&NumLocation,NULL },
   { &(StaticLocation.MaxDrug),NULL,NULL,NULL,"MaxDrug",
     N_("Maximum number of drugs at each location"),
     (void **)(&Location),&StaticLocation,
     sizeof(struct LOCATION),"Location",&NumLocation,NULL },
   { NULL,NULL,&StaticDrug.Name,NULL,"Name",
     N_("Name of each drug"),
     (void **)(&Drug),&StaticDrug,
     sizeof(struct DRUG),"Drug",&NumDrug,NULL },
   { NULL,&(StaticDrug.MinPrice),NULL,NULL,"MinPrice",
     N_("Minimum normal price of each drug"),
     (void **)(&Drug),&StaticDrug,
     sizeof(struct DRUG),"Drug",&NumDrug,NULL },
   { NULL,&(StaticDrug.MaxPrice),NULL,NULL,"MaxPrice",
     N_("Maximum normal price of each drug"),
     (void **)(&Drug),&StaticDrug,
     sizeof(struct DRUG),"Drug",&NumDrug,NULL },
   { &(StaticDrug.Cheap),NULL,NULL,NULL,"Cheap",
     N_("Non-zero if this drug can be specially cheap"),
     (void **)(&Drug),&StaticDrug,
     sizeof(struct DRUG),"Drug",&NumDrug,NULL },
   { &(StaticDrug.Expensive),NULL,NULL,NULL,"Expensive",
     N_("Non-zero if this drug can be specially expensive"),
     (void **)(&Drug),&StaticDrug,
     sizeof(struct DRUG),"Drug",&NumDrug,NULL },
   { NULL,NULL,&StaticDrug.CheapStr,NULL,"CheapStr",
     N_("Message displayed when this drug is specially cheap"),
     (void **)(&Drug),&StaticDrug,
     sizeof(struct DRUG),"Drug",&NumDrug,NULL },
   { NULL,NULL,&Drugs.ExpensiveStr1,NULL,"Drugs.ExpensiveStr1",
     N_("Format string used for expensive drugs 50% of time"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Drugs.ExpensiveStr2,NULL,"Drugs.ExpensiveStr2",
     N_("Format string used for expensive drugs 50% of time"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Drugs.CheapDivide),NULL,NULL,NULL,"Drugs.CheapDivide",
     N_("Divider for drug price when it's specially cheap"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Drugs.ExpensiveMultiply),NULL,NULL,NULL,"Drugs.ExpensiveMultiply",
     N_("Multiplier for specially expensive drug prices"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&StaticGun.Name,NULL,"Name",
     N_("Name of each gun"),
     (void **)(&Gun),&StaticGun,
     sizeof(struct GUN),"Gun",&NumGun,NULL },
   { NULL,&(StaticGun.Price),NULL,NULL,"Price",
     N_("Price of each gun"),
     (void **)(&Gun),&StaticGun,
     sizeof(struct GUN),"Gun",&NumGun,NULL },
   { &(StaticGun.Space),NULL,NULL,NULL,"Space",
     N_("Space taken by each gun"),
     (void **)(&Gun),&StaticGun,
     sizeof(struct GUN),"Gun",&NumGun,NULL },
   { &(StaticGun.Damage),NULL,NULL,NULL,"Damage",
     N_("Damage done by each gun"),
     (void **)(&Gun),&StaticGun,
     sizeof(struct GUN),"Gun",&NumGun,NULL },
   { &(Cops.EscapeProb),NULL,NULL,NULL,"Cops.EscapeProb",
     N_("% probability of escaping from Officer Hardass"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Cops.DeputyEscape),NULL,NULL,NULL,"Cops.DeputyEscape",
     N_("Modifier to EscapeProb for each extra deputy"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Cops.HitProb),NULL,NULL,NULL,"Cops.HitProb",
     N_("% probability that Officer Hardass hits you"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Cops.DeputyHit),NULL,NULL,NULL,"Cops.DeputyHit",
     N_("Modifier to HitProb for each extra deputy"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Cops.Damage),NULL,NULL,NULL,"Cops.Damage",
     N_("Maximum damage done to you by each cop"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Cops.Toughness),NULL,NULL,NULL,"Cops.Toughness",
     N_("Toughness of (difficulty of hitting) each cop"),
     NULL,NULL,0,"",NULL,NULL },
   { &(Cops.DropProb),NULL,NULL,NULL,"Cops.DropProb",
     N_("% probability that the cops catch you dropping drugs"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.Bitch,NULL,"Names.Bitch",
     N_("Word used to denote a single \"bitch\""),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.Bitches,NULL,"Names.Bitches",
     N_("Word used to denote two or more \"bitches\""),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.Gun,NULL,"Names.Gun",
     N_("Word used to denote a single gun or equivalent"),NULL,NULL,0,"",NULL,
     NULL },
   { NULL,NULL,&Names.Guns,NULL,"Names.Guns",
     N_("Word used to denote two or more guns"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.Drug,NULL,"Names.Drug",
     N_("Word used to denote a single drug or equivalent"),NULL,NULL,0,"",NULL,
     NULL },
   { NULL,NULL,&Names.Drugs,NULL,"Names.Drugs",
     N_("Word used to denote two or more drugs"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.Month,NULL,"Names.Month",
     N_("Text prefixed to the turn number (i.e. the month)"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,&Names.Year,NULL,"Names.Year",
     N_("Text appended to the turn number (i.e. the year)"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL, &Names.Officer,NULL,"Names.Officer",
     N_("Name of the police officer"),NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL, &Names.ReserveOfficer,NULL,"Names.ReserveOfficer",
     N_("Name of the reserve police officer"),NULL,NULL,0,"",NULL,NULL },
   { NULL,&Prices.Spy,NULL,NULL,"Prices.Spy",
     N_("Cost for a bitch to spy on the enemy"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,&Prices.Tipoff,NULL,NULL,"Prices.Tipoff",
     N_("Cost for a bitch to tipoff the cops to an enemy"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,&Bitch.MinPrice,NULL,NULL,"Bitch.MinPrice",
     N_("Minimum price to hire a bitch"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,&Bitch.MaxPrice,NULL,NULL,"Bitch.MaxPrice",
     N_("Maximum price to hire a bitch"),
     NULL,NULL,0,"",NULL,NULL },
   { NULL,NULL,NULL,&SubwaySaying,"SubwaySaying",
     N_("List of things which you overhear on the subway"),
     NULL,NULL,0,"",&NumSubway,ResizeSubway },
   { &NumSubway,NULL,NULL,NULL,"NumSubwaySaying",
     N_("Number of subway sayings"),
     NULL,NULL,0,"",NULL,ResizeSubway },
   { NULL,NULL,NULL,&Playing,"Playing",
     N_("List of songs which you can hear playing"),
     NULL,NULL,0,"",&NumPlaying,ResizePlaying },
   { &NumPlaying,NULL,NULL,NULL,"NumPlaying",
     N_("Number of playing songs"),
     NULL,NULL,0,"",NULL,ResizePlaying },
   { NULL,NULL,NULL,&StoppedTo,"StoppedTo",
     N_("List of things which you can stop to do"),
     NULL,NULL,0,"",&NumStoppedTo,ResizeStoppedTo },
   { &NumStoppedTo,NULL,NULL,NULL,"NumStoppedTo",
     N_("Number of things which you can stop to do"),
     NULL,NULL,0,"",NULL,ResizeStoppedTo }
};

char *Discover[NUMDISCOVER] = {
   N_("escaped"), N_("defected"), N_("was shot") };

char **Playing=NULL;
char *DefaultPlaying[NUMPLAYING] = {
   N_("`Are you Experienced` by Jimi Hendrix"),
   N_("`Cheeba Cheeba` by Tone Loc"),
   N_("`Comin` in to Los Angeles` by Arlo Guthrie"),
   N_("`Commercial` by Spanky and Our Gang"),
   N_("`Late in the Evening` by Paul Simon"),
   N_("`Light Up` by Styx"),
   N_("`Mexico` by Jefferson Airplane"),
   N_("`One toke over the line` by Brewer & Shipley"),
   N_("`The Smokeout` by Shel Silverstein"),
   N_("`White Rabbit` by Jefferson Airplane"),
   N_("`Itchycoo Park` by Small Faces"),
   N_("`White Punks on Dope` by the Tubes"),
   N_("`Legend of a Mind` by the Moody Blues"),
   N_("`Eight Miles High` by the Byrds"),
   N_("`Acapulco Gold` by Riders of the Purple Sage"),
   N_("`Kicks` by Paul Revere & the Raiders"),
   N_("the Nixon tapes"),
   N_("`Legalize It` by Mojo Nixon & Skid Roper")
};

char **StoppedTo=NULL;
char *DefaultStoppedTo[NUMSTOPPEDTO] = {
   N_("have a beer"),
   N_("smoke a joint"),
   N_("smoke a cigar"),
   N_("smoke a Djarum"),
   N_("smoke a cigarette")
};

struct GUN DefaultGun[NUMGUN] = {
   { N_("Baretta"),3000,4,5 },
   { N_(".38 Special"),3500,4,9 },
   { N_("Ruger"),2900,4,4 },
   { N_("Saturday Night Special"),3100,4,7 }
};

struct LOCATION DefaultLocation[NUMLOCATION] = {
   { N_("Bronx"),10,NUMDRUG/2+1,NUMDRUG },
   { N_("Ghetto"),5,NUMDRUG/2+2,NUMDRUG },
   { N_("Central Park"),15,NUMDRUG/2,NUMDRUG },
   { N_("Manhattan"),90,NUMDRUG/2-2,NUMDRUG-2 },
   { N_("Coney Island"),20,NUMDRUG/2,NUMDRUG },
   { N_("Brooklyn"),70,NUMDRUG/2-2,NUMDRUG-1 },
   { N_("Queens"),50,NUMDRUG/2,NUMDRUG },
   { N_("Staten Island"),20,NUMDRUG/2,NUMDRUG }
};

struct DRUG DefaultDrug[NUMDRUG] = {
   { N_("Acid"),1000,4400,1,0,
     N_("The market is flooded with cheap home-made acid!") },
   { N_("Cocaine"),15000,29000,0,1,"" },
   { N_("Hashish"),480,1280,1,0,N_("The Marrakesh Express has arrived!") },
   { N_("Heroin"),5500,13000,0,1,"" },
   { N_("Ludes"),11,60,1,0,
     N_("Rival drug dealers raided a pharmacy and are selling cheap ludes!") },
   { N_("MDA"),1500,4400,0,0,"" },
   { N_("Opium"),540,1250,0,1,"" },
   { N_("PCP"),1000,2500,0,0,"" },
   { N_("Peyote"),220,700,0,0,"" },
   { N_("Shrooms"),630,1300,0,0,"" },
   { N_("Speed"),90,250,0,1,"" },
   { N_("Weed"),315,890,1,0,N_("Columbian freighter dusted the Coast Guard! \
Weed prices have bottomed out!") }
};

struct DRUGS Drugs = { NULL,NULL,0,0 };
struct DRUGS DefaultDrugs = {
   N_("Cops made a big %s bust! Prices are outrageous!"),
   N_("Addicts are buying %s at ridiculous prices!"),
   4,4 };

char **SubwaySaying=NULL;
char *DefaultSubwaySaying[NUMSUBWAY] = {
   N_("Wouldn\'t it be funny if everyone suddenly quacked at once?"),
   N_("The Pope was once Jewish, you know"),
   N_("I\'ll bet you have some really interesting dreams"),
   N_("So I think I\'m going to Amsterdam this year"),
   N_("Son, you need a yellow haircut"),
   N_("I think it\'s wonderful what they\'re doing with incense these days"),
   N_("I wasn\'t always a woman, you know"),
   N_("Does your mother know you\'re a dope dealer?"),
   N_("Are you high on something?"),
   N_("Oh, you must be from California"),
   N_("I used to be a hippie, myself"),
   N_("There\'s nothing like having lots of money"),
   N_("You look like an aardvark!"),
   N_("I don\'t believe in Ronald Reagan"),
   N_("Courage!  Bush is a noodle!"),
   N_("Haven\'t I seen you on TV?"),
   N_("I think hemorrhoid commercials are really neat!"),
   N_("We\'re winning the war for drugs!"),
   N_("A day without dope is like night"),
   N_("We only use 20% of our brains, so why not burn out the other 80%"),
   N_("I\'m soliciting contributions for Zombies for Christ"),
   N_("I\'d like to sell you an edible poodle"),
   N_("Winners don\'t do drugs... unless they do"),
   N_("Kill a cop for Christ!"),
   N_("I am the walrus!"),
   N_("Jesus loves you more than you will know"),
   N_("I feel an unaccountable urge to dye my hair blue"),
   N_("Wasn\'t Jane Fonda wonderful in Barbarella"),
   N_("Just say No... well, maybe... ok, what the hell!"),
   N_("Would you like a jelly baby?"),
   N_("Drugs can be your friend!")
};

int brandom(int bot,int top) {
/* Returns a random integer not less than bot and less than top */
   return (int)((float)(top-bot)*rand()/(RAND_MAX+1.0))+bot;
}

int CountPlayers(GSList *First) {
/* Returns the total numbers of players in the list starting at "First"; */
/* players still in the process of connecting or leaving are ignored.    */
   GSList *list;
   Player *Play;
   int count=0;
   for (list=First;list;list=g_slist_next(list)) {
      Play=(Player *)list->data;
      if (strlen(GetPlayerName(Play))>0) count++;
   }
   return count;
}

GSList *AddPlayer(int fd,Player *NewPlayer,GSList *First) {
/* Adds the new Player structure "NewPlayer" to the linked list       */
/* pointed to by "First", and initialises all fields. Returns the new */
/* start of the list. If this function is called by the server, then  */
/* it should pass the file descriptor of the socket used to           */
/* communicate with the client player.                                */
   NewPlayer->fd=-1;
   NewPlayer->Name=NULL;
   SetPlayerName(NewPlayer,NULL);
   NewPlayer->IsAt=0;
   NewPlayer->Attacked=NULL;
   NewPlayer->EventNum=E_NONE;
   NewPlayer->FightTimeout=NewPlayer->ConnectTimeout=NewPlayer->IdleTimeout=0;
   NewPlayer->Guns=(Inventory *)g_malloc0(NumGun*sizeof(Inventory));
   NewPlayer->Drugs=(Inventory *)g_malloc0(NumDrug*sizeof(Inventory));
   InitList(&(NewPlayer->SpyList));
   InitList(&(NewPlayer->TipList));
   NewPlayer->Turn=1;
   NewPlayer->Cash=StartCash;
   NewPlayer->Debt=StartDebt;
   NewPlayer->Bank=0;
   NewPlayer->Health=100;
   NewPlayer->CoatSize=100;
   NewPlayer->Bitches.Carried=8;
   NewPlayer->Flags=0;
   NewPlayer->ReadBuf.Data=NewPlayer->WriteBuf.Data=NULL;
   NewPlayer->ReadBuf.Length=NewPlayer->WriteBuf.Length=0;
   NewPlayer->ReadBuf.DataPresent=NewPlayer->WriteBuf.DataPresent=0;
   if (Server) NewPlayer->fd=fd;
   return g_slist_append(First,(gpointer)NewPlayer);
}

void UpdatePlayer(Player *Play) {
/* Redimensions the Gun and Drug lists for "Play" */
   Play->Guns=(Inventory *)g_realloc(Play->Guns,NumGun*sizeof(Inventory));
   Play->Drugs=(Inventory *)g_realloc(Play->Drugs,NumDrug*sizeof(Inventory));
}

GSList *RemovePlayer(Player *Play,GSList *First) {
/* Removes the Player structure pointed to by "Play" from the linked */
/* list starting at "First". The client socket is freed if called    */
/* from the server. The new start of the list is returned.           */
   g_assert(Play);
   g_assert(First);

   First=g_slist_remove(First,(gpointer)Play);
   if (Server && Play->fd>=0) {
      CloseSocket(Play->fd);
   }
   ClearList(&(Play->SpyList));
   ClearList(&(Play->TipList));
   g_free(Play->ReadBuf.Data);
   g_free(Play->WriteBuf.Data);
   g_free(Play->Name);
   g_free(Play);
   return First;
}

void CopyPlayer(Player *Dest,Player *Src) {
/* Copies player "Src" to player "Dest"        */
   if (!Dest || !Src) return;
   Dest->Turn=Src->Turn;
   Dest->Cash=Src->Cash;
   Dest->Debt=Src->Debt;
   Dest->Bank=Src->Bank;
   Dest->Health=Src->Health;
   ClearInventory(Dest->Guns,Dest->Drugs);
   AddInventory(Dest->Guns,Src->Guns,NumGun);
   AddInventory(Dest->Drugs,Src->Drugs,NumDrug);
   Dest->CoatSize=Src->CoatSize;
   Dest->IsAt=Src->IsAt;
   g_free(Dest->Name);
   Dest->Name=g_strdup(Src->Name);
   Dest->Bitches.Carried=Src->Bitches.Carried;
   Dest->Flags=Src->Flags;
}

char *GetPlayerName(Player *Play) {
   if (Play->Name) return Play->Name;
   else return "";
}

void SetPlayerName(Player *Play,char *Name) {
   if (Play->Name) g_free(Play->Name);
   if (!Name) Play->Name=g_strdup("");
   else Play->Name = g_strdup(Name);
}

Player *GetPlayerByName(char *Name,GSList *First) {
/* Searches the linked list starting at "First" for a Player structure */
/* with the name "Name". Returns a pointer to this structure, or NULL  */
/* if no match can be found.                                           */
   GSList *list;
   Player *Play;
   if (Name==NULL || Name[0]==0) return &Noone;
   for (list=First;list;list=g_slist_next(list)) {
      Play=(Player *)list->data;
      if (strcmp(GetPlayerName(Play),Name)==0) return Play;
   }
   return NULL;
}

price_t strtoprice(char *buf) {
/* Forms a price based on the string representation in "buf"  */
   int i,buflen,FracNum;
   char digit,minus,suffix;
   gboolean InFrac;
   price_t val=0;
   minus=0;
   InFrac=FALSE;
   if (!buf) return 0;
   buflen=strlen(buf);
   suffix=buf[buflen-1];
   suffix=toupper(suffix);
   if (suffix=='M') FracNum=6;
   else if (suffix=='K') FracNum=3;
   else FracNum=0;
   for (i=0;i<strlen(buf);i++) {
      digit=buf[i];
      if (digit=='.' || digit==',') {
         InFrac=TRUE;
      } else if (digit>='0' && digit<='9') {
         if (InFrac && FracNum<=0) break;
         else if (InFrac) FracNum--;
         val*=10;
         val+=(digit-'0');
      } else if (digit=='-') minus=1;
   }
   for (i=0;i<FracNum;i++) val*=10;
   if (minus) val=-val;
   return val;
}

gchar *pricetostr(price_t price) {
/* Prints "price" directly into a dynamically-allocated string buffer  */
/* and returns a pointer to this buffer. It is the responsbility of    */
/* the user to g_free this buffer when it is finished with.            */
   GString *PriceStr;
   gchar *NewBuffer;
   price_t absprice;

   if (price<0) absprice=-price; else absprice=price;
   PriceStr=g_string_new(NULL);
   while (absprice!=0) {
      g_string_prepend_c(PriceStr,'0'+(absprice%10));
      absprice /= 10;
      if (absprice==0) {
         if (price<0) g_string_prepend_c(PriceStr,'-');
      }
   }
   NewBuffer=PriceStr->str;
   /* Free the string structure, but not the actual char array */
   g_string_free(PriceStr,FALSE);
   return NewBuffer;
}

gchar *FormatPrice(price_t price) {
/* Takes the number in "price" and prints it into a dynamically-allocated */
/* string, adding commas to split up thousands, and adding a currency     */
/* symbol to the start. Returns a pointer to the string, which must be    */
/* g_free'd by the user when it is finished with.                         */
   GString *PriceStr;
   gchar *NewBuffer;
   char thou[10];
   gboolean First=TRUE;
   price_t absprice;
   PriceStr=g_string_new(NULL);
   if (price<0) absprice=-price; else absprice=price;
   while (First || absprice>0) {
      if (absprice>=1000) sprintf(thou,"%03d",(int)(absprice%1000l));
      else sprintf(thou,"%d",(int)(price%1000l));
      price/=1000l;
      absprice/=1000l;
      if (!First) g_string_prepend_c(PriceStr,',');
      g_string_prepend(PriceStr,thou);
      First=FALSE;
   }
   g_string_prepend_c(PriceStr,'$');
   NewBuffer=PriceStr->str;
   /* Free the string structure only, not the char data */
   g_string_free(PriceStr,FALSE);
   return NewBuffer;
}

int TotalGunsCarried(Player *Play) {
/* Returns the total number of guns being carried by "Play" */
   int i,c;
   c=0;
   for (i=0;i<NumGun;i++) c+=Play->Guns[i].Carried;
   return c;
}

gchar *InitialCaps(gchar *string) {
/* Capitalises the first character of "string" and writes the resultant */
/* string into a dynamically-allocated copy; the user must g_free this  */
/* string (a pointer to which is returned) when it is no longer needed. */
   gchar *buf;
   if (!string) return NULL;
   buf=g_strdup(string);
   if (strlen(buf)>=1) buf[0]=toupper(buf[0]);
   return buf;
}

char StartsWithVowel(char *string) {
/* Returns TRUE if "string" starts with a vowel */
   int c;
   if (!string || strlen(string)<1) return FALSE;
   c=toupper(string[0]);
   return (c=='A' || c=='E' || c=='I' || c=='O' || c=='U');
}

int read_string(FILE *fp,char **buf) {
/* Reads a NULL-terminated string into the buffer "buf" from file "fp".  */
/* buf is sized to hold the string; this is a dynamic string and must be */
/* freed by the calling routine. Returns 0 on success, EOF on failure.   */
   int c;
   GString *text;
   text=g_string_new("");
   c=0;
   while (1) {
      c=fgetc(fp);
      if (c==EOF || c==0) break;
      else { g_string_append_c(text,(char)c); }
   }
   *buf=text->str;
   /* Free the GString, but not the actual data text->str */
   g_string_free(text,FALSE);
   if (c==EOF) return EOF; else return 0;
}

void ClearInventory(Inventory *Guns,Inventory *Drugs) {
/* This function simply clears the given inventories "Guns" */
/* and "Drugs" if they are non-NULL                         */
   int i;
   if (Guns) for (i=0;i<NumGun;i++) Guns[i].Carried=0;
   if (Drugs) for (i=0;i<NumDrug;i++) Drugs[i].Carried=0;
}

char IsInventoryClear(Inventory *Guns,Inventory *Drugs) {
/* Returns TRUE only if "Guns" and "Drugs" contain no objects */
   int i;
   if (Guns) for (i=0;i<NumGun;i++) if (Guns[i].Carried > 0) return FALSE;
   if (Drugs) for (i=0;i<NumDrug;i++) if (Drugs[i].Carried > 0) return FALSE;
   return TRUE;
}

void AddInventory(Inventory *Cumul,Inventory *Add,int Length) {
/* Adds inventory "Add" into the contents of inventory "Cumul" */
/* Each inventory is of length "Length"                        */
   int i;
   for (i=0;i<Length;i++) Cumul[i].Carried+=Add[i].Carried;
}

void ChangeSpaceForInventory(Inventory *Guns,Inventory *Drugs,
                             Player *Play) {
/* Given the lists of "Guns" and "Drugs" (which the given player "Play" */
/* must have sufficient room to carry) updates the player's space to    */
/* reflect carrying them.                                               */
   int i;
   if (Guns) for (i=0;i<NumGun;i++) {
      Play->CoatSize-=Guns[i].Carried*Gun[i].Space;
   }
   if (Drugs) for (i=0;i<NumDrug;i++) {
      Play->CoatSize-=Drugs[i].Carried;
   }
}

void TruncateInventoryFor(Inventory *Guns,Inventory *Drugs,
                          Player *Play) {
/* Discards items from "Guns" and/or "Drugs" (if non-NULL) if necessary */
/* such that player "Play" is able to carry them all. The cheapest      */
/* objects are discarded.                                               */
   int i,Total,CheapIndex;
   int CheapestGun;
   Total=0;
   if (Guns) for (i=0;i<NumGun;i++) Total+=Guns[i].Carried;
   Total+=TotalGunsCarried(Play);
   while (Guns && Total > Play->Bitches.Carried+2) {
      CheapIndex=-1;  
      for (i=0;i<NumGun;i++) if (Guns[i].Carried && (CheapIndex==-1 || 
                                 Gun[i].Price <= Gun[CheapIndex].Price)) {
         CheapIndex=i;
      }
      i=Total-Play->Bitches.Carried-2;
      if (Guns[CheapIndex].Carried > i) {
         Guns[CheapIndex].Carried-=i; Total-=i;
      } else {
         Total-=Guns[CheapIndex].Carried; Guns[CheapIndex].Carried=0;
      }
   }

   Total=Play->CoatSize;
   if (Guns) for (i=0;i<NumGun;i++) Total-=Guns[i].Carried*Gun[i].Space;
   if (Drugs) for (i=0;i<NumDrug;i++) Total-=Drugs[i].Carried;
   while (Total < 0) {
      CheapestGun=-1;
      CheapIndex=-1;
      if (Guns) for (i=0;i<NumGun;i++) if (Guns[i].Carried && (CheapIndex==-1 ||
                                 Gun[i].Price <= Gun[CheapIndex].Price)) {
         CheapIndex=i; CheapestGun=Gun[i].Price/Gun[i].Space;
      }
      if (Drugs) for (i=0;i<NumDrug;i++) if (Drugs[i].Carried &&
         (CheapIndex==-1 ||
          (CheapestGun==-1 && Drug[i].MinPrice<=Drug[CheapIndex].MinPrice) ||
          (CheapestGun>=0 && Drug[i].MinPrice<=CheapestGun))) {
         CheapIndex=i; CheapestGun=-1;
      }
      if (Guns && CheapestGun>=0) { 
         Guns[CheapIndex].Carried--;  
         Total+=Gun[CheapIndex].Space;
      } else {
         if (Drugs && Drugs[CheapIndex].Carried >= -Total) {
            Drugs[CheapIndex].Carried += Total; Total=0;
         } else {
            Total+=Drugs[CheapIndex].Carried; Drugs[CheapIndex].Carried=0;
         }
      }
   }
} 

int IsCarryingRandom(Player *Play,int amount) {
/* Returns an index into the drugs array of a random drug that "Play" is */
/* carrying at least "amount" of. If no suitable drug is found after 5   */
/* attempts, returns -1.                                                 */ 
   int i,ind;
   for (i=0;i<5;i++) {
      ind=brandom(0,NumDrug);
      if (Play->Drugs[ind].Carried >= amount) {
         return ind;
      }
   }
   return -1;
}

int GetNextDrugIndex(int OldIndex,Player *Play) {
/* Returns an index into the "Drugs" array maintained by player "Play"  */
/* of the next available drug after "OldIndex", following the current   */
/* sort method (defined globally as "DrugSortMethod")                   */
   int i,MaxIndex;
   MaxIndex=-1;
   for (i=0;i<NumDrug;i++) {
      if (Play->Drugs[i].Price!=0 && i!=OldIndex && i!=MaxIndex && 
          (MaxIndex==-1 || 
              (DrugSortMethod==DS_ATOZ && 
               strcasecmp(Drug[MaxIndex].Name,Drug[i].Name)>0) || 
              (DrugSortMethod==DS_ZTOA && 
               strcasecmp(Drug[MaxIndex].Name,Drug[i].Name)<0) || 
              (DrugSortMethod==DS_CHEAPFIRST && 
               Play->Drugs[MaxIndex].Price > Play->Drugs[i].Price) ||
              (DrugSortMethod==DS_CHEAPLAST && 
               Play->Drugs[MaxIndex].Price < Play->Drugs[i].Price)) &&
          (OldIndex==-1 || 
              (DrugSortMethod==DS_ATOZ && 
               strcasecmp(Drug[OldIndex].Name,Drug[i].Name)<=0) ||
              (DrugSortMethod==DS_ZTOA && 
               strcasecmp(Drug[OldIndex].Name,Drug[i].Name)>=0) ||
              (DrugSortMethod==DS_CHEAPFIRST && 
               Play->Drugs[OldIndex].Price <= Play->Drugs[i].Price) ||
              (DrugSortMethod==DS_CHEAPLAST && 
               Play->Drugs[OldIndex].Price >= Play->Drugs[i].Price))) {
         MaxIndex=i;
      }
   }
   return MaxIndex;
}

void InitList(DopeList *List) {
/* A DopeList is akin to a Vector class; it is a list of DopeEntry   */
/* structures, which can be dynamically extended or compressed. This */
/* function initialises the newly-created list pointed to by "List"  */
/* (A DopeEntry contains a Player pointer and a counter, and is used */
/* by the server to keep track of tipoffs and spies.)                */
   List->Data=NULL;
   List->Number=0;
}

void ClearList(DopeList *List) {
/* Clears the list pointed to by "List" */
   free(List->Data);
   InitList(List);
}

void AddListEntry(DopeList *List,DopeEntry *NewEntry) {
/* Adds a new DopeEntry (pointed to by "NewEntry") to the list "List". */
/* A copy of NewEntry is placed into the list, so the original         */
/* structure pointed to by NewEntry can be reused.                     */
   if (!NewEntry || !List) return;
   List->Number++;
   List->Data = (DopeEntry *)g_realloc(List->Data,List->Number*
                                                  sizeof(DopeEntry));
   g_memmove(&(List->Data[List->Number-1]),NewEntry,sizeof(DopeEntry));
}

void RemoveListEntry(DopeList *List,int Index) {
/* Removes the DopeEntry at index "Index" from list "List" */
   if (!List || Index<0 || Index>=List->Number) return;

   g_memmove(&(List->Data[Index]),&(List->Data[Index+1]),
             (List->Number-1-Index)*sizeof(DopeEntry));
   List->Number--;
   List->Data = (DopeEntry *)g_realloc(List->Data,List->Number*
                                                  sizeof(DopeEntry));
   if (List->Number==0) List->Data=NULL;
}

int GetListEntry(DopeList *List,Player *Play) {
/* Returns the index of the DopeEntry matching "Play" in list "List" */
/* or -1 if this is not found.                                       */
   int i;
   for (i=List->Number-1;i>=0;i--) {
      if (List->Data[i].Play==Play) return i;
   }
   return -1;
}

void RemoveListPlayer(DopeList *List,Player *Play) {
/* Removes (if it exists) the DopeEntry in list "List" matching "Play" */
   RemoveListEntry(List,GetListEntry(List,Play));
}

void RemoveAllEntries(DopeList *List,Player *Play) {
/* Similar to RemoveListPlayer, except that if the list contains "Play"     */
/* more than once, all the matching entries are removed, not just the first */
   int i=0;
   while (1) {
      i=GetListEntry(List,Play);
      if (i==-1) break;
      RemoveListEntry(List,i);
   }
}
     
void ResizeLocations(int NewNum) {
   int i;
   if (NewNum<NumLocation) for (i=NewNum;i<NumLocation;i++) {
      g_free(Location[i].Name);
   }
   Location=g_realloc(Location,sizeof(struct LOCATION)*NewNum);
   if (NewNum>NumLocation) {
      memset(&Location[NumLocation],0,
             (NewNum-NumLocation)*sizeof(struct LOCATION));
      for (i=NumLocation;i<NewNum;i++) {
         Location[i].Name=g_strdup("");
      }
   }
   NumLocation=NewNum;
}

void ResizeGuns(int NewNum) {
   int i;
   if (NewNum<NumGun) for (i=NewNum;i<NumGun;i++) {
      g_free(Gun[i].Name);
   }
   Gun=g_realloc(Gun,sizeof(struct GUN)*NewNum);
   if (NewNum>NumGun) {
      memset(&Gun[NumGun],0,(NewNum-NumGun)*sizeof(struct GUN));
      for (i=NumGun;i<NewNum;i++) {
         Gun[i].Name=g_strdup("");
      }
   }
   NumGun=NewNum;
}

void ResizeDrugs(int NewNum) {
   int i;
   if (NewNum<NumDrug) for (i=NewNum;i<NumDrug;i++) {
      g_free(Drug[i].Name); g_free(Drug[i].CheapStr);
   }
   Drug=g_realloc(Drug,sizeof(struct DRUG)*NewNum);
   if (NewNum>NumDrug) {
      memset(&Drug[NumDrug],0,
             (NewNum-NumDrug)*sizeof(struct DRUG));
      for (i=NumDrug;i<NewNum;i++) {
         Drug[i].Name=g_strdup(""); Drug[i].CheapStr=g_strdup("");
      }
   }
   NumDrug=NewNum;
}

void ResizeSubway(int NewNum) {
   int i;
   if (NewNum<NumSubway) for (i=NewNum;i<NumSubway;i++) {
      g_free(SubwaySaying[i]);
   }
   SubwaySaying=g_realloc(SubwaySaying,sizeof(char *)*NewNum);
   if (NewNum>NumSubway) for (i=NumSubway;i<NewNum;i++) {
      SubwaySaying[i]=g_strdup("");
   }
   NumSubway=NewNum;
}

void ResizePlaying(int NewNum) {
   int i;
   if (NewNum<NumPlaying) for (i=NewNum;i<NumPlaying;i++) {
      g_free(Playing[i]);
   }
   Playing=g_realloc(Playing,sizeof(char *)*NewNum);
   if (NewNum>NumPlaying) for (i=NumPlaying;i<NewNum;i++) {
      Playing[i]=g_strdup("");
   }
   NumPlaying=NewNum;
}

void ResizeStoppedTo(int NewNum) {
   int i;
   if (NewNum<NumStoppedTo) for (i=NewNum;i<NumStoppedTo;i++) {
      g_free(StoppedTo[i]);
   }
   StoppedTo=g_realloc(StoppedTo,sizeof(char *)*NewNum);
   if (NewNum>NumStoppedTo) for (i=NumStoppedTo;i<NewNum;i++) {
      StoppedTo[i]=g_strdup("");
   }
   NumStoppedTo=NewNum;
}

void AssignName(gchar **dest,gchar *src) {
   g_free(*dest);
   *dest=g_strdup(src);
}

void CopyNames(struct NAMES *dest,struct NAMES *src) {
   AssignName(&dest->Bitch,_(src->Bitch));
   AssignName(&dest->Bitches,_(src->Bitches));
   AssignName(&dest->Gun,_(src->Gun));
   AssignName(&dest->Guns,_(src->Guns));
   AssignName(&dest->Drug,_(src->Drug));
   AssignName(&dest->Drugs,_(src->Drugs));
   AssignName(&dest->Month,_(src->Month));
   AssignName(&dest->Year,_(src->Year));
   AssignName(&dest->Officer,_(src->Officer));
   AssignName(&dest->ReserveOfficer,_(src->ReserveOfficer));
   AssignName(&dest->LoanSharkName,_(src->LoanSharkName));
   AssignName(&dest->BankName,_(src->BankName));
   AssignName(&dest->GunShopName,_(src->GunShopName));
   AssignName(&dest->RoughPubName,_(src->RoughPubName));
}

void CopyMetaServer(struct METASERVER *dest,struct METASERVER *src) {
   dest->Active=src->Active;
   dest->HttpPort=src->HttpPort;
   dest->UdpPort=src->UdpPort;
   AssignName(&dest->Name,src->Name);
   AssignName(&dest->Path,src->Path);
   AssignName(&dest->LocalName,src->LocalName);
   AssignName(&dest->Password,src->Password);
   AssignName(&dest->Comment,src->Comment);
}

void CopyLocation(struct LOCATION *dest,struct LOCATION *src) {
   AssignName(&dest->Name,_(src->Name));
   dest->PolicePresence=src->PolicePresence;
   dest->MinDrug=src->MinDrug; dest->MaxDrug=src->MaxDrug;
}

void CopyGun(struct GUN *dest,struct GUN *src) {
   AssignName(&dest->Name,_(src->Name));
   dest->Price=src->Price;
   dest->Space=src->Space;
   dest->Damage=src->Damage;
}

void CopyDrug(struct DRUG *dest,struct DRUG *src) {
   AssignName(&dest->Name,_(src->Name));
   dest->MinPrice=src->MinPrice;
   dest->MaxPrice=src->MaxPrice;
   dest->Cheap=src->Cheap;
   dest->Expensive=src->Expensive;
   AssignName(&dest->CheapStr,_(src->CheapStr));
}

void CopyDrugs(struct DRUGS *dest,struct DRUGS *src) {
   AssignName(&dest->ExpensiveStr1,_(src->ExpensiveStr1));
   AssignName(&dest->ExpensiveStr2,_(src->ExpensiveStr2));
   dest->CheapDivide=src->CheapDivide;
   dest->ExpensiveMultiply=src->ExpensiveMultiply;
}

void ReadConfigFile(char *FileName) {
   FILE *fp;
   GScanner *scanner;
   fp=fopen(FileName,"r");
   if (fp) {
      scanner=g_scanner_new(&ScannerConfig);
      scanner->input_name=FileName;
      g_scanner_input_file(scanner,fileno(fp));
      while (!g_scanner_eof(scanner)) if (!ParseNextConfig(scanner)) {
         g_scanner_error(scanner,
                         _("Unable to process configuration file line"));
      }
      g_scanner_destroy(scanner);
      fclose(fp);
   }
}

gboolean ParseNextConfig(GScanner *scanner) {
   GTokenType token;
   gchar *ID1,*ID2;
   gulong index=0;
   int GlobalIndex;
   gboolean IndexGiven=FALSE;

   ID1=ID2=NULL;
   token=g_scanner_get_next_token(scanner);
   if (token==G_TOKEN_EOF) return TRUE;
   if (token!=G_TOKEN_IDENTIFIER) {
      g_scanner_unexp_token(scanner,G_TOKEN_IDENTIFIER,NULL,NULL,
                            NULL,NULL,FALSE);
      return FALSE;
   }
   ID1=g_strdup(scanner->value.v_identifier);
   token=g_scanner_get_next_token(scanner);
   if (token==G_TOKEN_LEFT_BRACE) {
      token=g_scanner_get_next_token(scanner);
      if (token!=G_TOKEN_INT) {
         g_scanner_unexp_token(scanner,G_TOKEN_INT,NULL,NULL,
                               NULL,NULL,FALSE);
         return FALSE;
      }
      index=scanner->value.v_int;
      IndexGiven=TRUE;
      token=g_scanner_get_next_token(scanner);
      if (token!=G_TOKEN_RIGHT_BRACE) {
         g_scanner_unexp_token(scanner,G_TOKEN_RIGHT_BRACE,NULL,NULL,
                               NULL,NULL,FALSE);
         return FALSE;
      }
      token=g_scanner_get_next_token(scanner);
      if (token=='.') {
         token=g_scanner_get_next_token(scanner);
         if (token!=G_TOKEN_IDENTIFIER) {
            g_scanner_unexp_token(scanner,G_TOKEN_IDENTIFIER,NULL,NULL,
                                  NULL,NULL,FALSE);
            return FALSE;
         }
         ID2=g_strdup(scanner->value.v_identifier);
         token=g_scanner_get_next_token(scanner);
      }
   }
   GlobalIndex=GetGlobalIndex(ID1,ID2);
   g_free(ID1); g_free(ID2);
   if (GlobalIndex==-1) return FALSE;
   if (token==G_TOKEN_EOF) {
      PrintConfigValue(GlobalIndex,index,IndexGiven,scanner);
      return TRUE;
   } else if (token==G_TOKEN_EQUAL_SIGN) {
      token=g_scanner_get_next_token(scanner);
      if (CountPlayers(FirstServer)>0) {
         g_warning(
_("Configuration can only be changed interactively when no\n"
"players are logged on. Wait for all players to log off, or remove\n"
"them with the push or kill commands, and try again."));
      } else {
         SetConfigValue(GlobalIndex,index,IndexGiven,scanner);
      }
      return TRUE;
   } else {
      return FALSE;
   }
   return FALSE;
}

int GetGlobalIndex(gchar *ID1,gchar *ID2) {
   int i;
   if (!ID1) return -1;
   for (i=0;i<NUMGLOB;i++) {
      if (strcasecmp(ID1,Globals[i].Name)==0 && !Globals[i].NameStruct[0]) {
/* Just a bog-standard ID1=value */
         return i;
      }
      if (strcasecmp(ID1,Globals[i].NameStruct)==0 && ID2 &&
          strcasecmp(ID2,Globals[i].Name)==0 &&
          Globals[i].StructStaticPt && Globals[i].StructListPt) {
/* ID1[index].ID2=value */
         return i;
      }
   }
   return -1;
}

void *GetGlobalPointer(int GlobalIndex,int StructIndex) {
   void *ValPt=NULL;

   if (Globals[GlobalIndex].IntVal) {
      ValPt=(void *)Globals[GlobalIndex].IntVal;
   } else if (Globals[GlobalIndex].PriceVal) {
      ValPt=(void *)Globals[GlobalIndex].PriceVal;
   } else if (Globals[GlobalIndex].StringVal) {
      ValPt=(void *)Globals[GlobalIndex].StringVal;
   }
   if (!ValPt) return NULL;

   if (Globals[GlobalIndex].StructStaticPt &&
       Globals[GlobalIndex].StructListPt) {
      return ValPt-Globals[GlobalIndex].StructStaticPt +
             *(Globals[GlobalIndex].StructListPt) +
             (StructIndex-1)*Globals[GlobalIndex].LenStruct;
   } else {
      return ValPt;
   }
}

gboolean CheckMaxIndex(GScanner *scanner,int GlobalIndex,int StructIndex,
                       gboolean IndexGiven) {
   if (!Globals[GlobalIndex].MaxIndex ||
       (Globals[GlobalIndex].StringList && !IndexGiven) ||
      (IndexGiven && StructIndex>=1 &&
       StructIndex <= *(Globals[GlobalIndex].MaxIndex))) {
      return TRUE;
   }
   g_scanner_error(scanner,_("Index into %s array should be between 1 and %d"),
                   (Globals[GlobalIndex].NameStruct &&
                    Globals[GlobalIndex].NameStruct[0]) ?
                       Globals[GlobalIndex].NameStruct :
                       Globals[GlobalIndex].Name,
                    *(Globals[GlobalIndex].MaxIndex));
   return FALSE;
}

void PrintConfigValue(int GlobalIndex,int StructIndex,gboolean IndexGiven,
                      GScanner *scanner) {
   gchar *prstr,*GlobalName;
   int i;
   if (!CheckMaxIndex(scanner,GlobalIndex,StructIndex,IndexGiven)) return;
   if (Globals[GlobalIndex].NameStruct[0]) {
      GlobalName=g_strdup_printf("%s[%d].%s",Globals[GlobalIndex].NameStruct,
                                 StructIndex,Globals[GlobalIndex].Name);
   } else GlobalName=Globals[GlobalIndex].Name;
   if (Globals[GlobalIndex].IntVal) {
      g_print(_("%s is %d\n"),GlobalName,
              *((int *)GetGlobalPointer(GlobalIndex,StructIndex)));
   } else if (Globals[GlobalIndex].PriceVal) {
      prstr=FormatPrice(*((price_t *)GetGlobalPointer(GlobalIndex,
                                                      StructIndex)));
      g_print(_("%s is %s\n"),GlobalName,prstr);
      g_free(prstr);
   } else if (Globals[GlobalIndex].StringVal) {
      g_print(_("%s is \"%s\"\n"),GlobalName,
              *((gchar **)GetGlobalPointer(GlobalIndex,StructIndex)));
   } else if (Globals[GlobalIndex].StringList) {
      if (IndexGiven) {
         g_print(_("%s[%d] is %s\n"),GlobalName,StructIndex,
                 (*(Globals[GlobalIndex].StringList))[StructIndex-1]);
      } else {
         g_print(_("%s is { "),GlobalName);
         if (Globals[GlobalIndex].MaxIndex) {
            for (i=0;i<*(Globals[GlobalIndex].MaxIndex);i++) {
               if (i>0) g_print(", ");
               g_print("\"%s\"",(*(Globals[GlobalIndex].StringList))[i]);
            }
         }
         g_print(" }\n");
      }
   }
   if (Globals[GlobalIndex].NameStruct[0]) g_free(GlobalName);
}

void SetConfigValue(int GlobalIndex,int StructIndex,gboolean IndexGiven,
                    GScanner *scanner) {
   gchar *GlobalName,*tmpstr;
   GTokenType token;
   int IntVal,NewNum;
   Player *tmp;
   GSList *list,*StartList;
   token=scanner->token;
   if (!CheckMaxIndex(scanner,GlobalIndex,StructIndex,IndexGiven)) return;
   if (Globals[GlobalIndex].NameStruct[0]) {
      GlobalName=g_strdup_printf("%s[%d].%s",Globals[GlobalIndex].NameStruct,
                                 StructIndex,Globals[GlobalIndex].Name);
   } else GlobalName=Globals[GlobalIndex].Name;
   if (Globals[GlobalIndex].IntVal) {
      if (token==G_TOKEN_INT) {
         IntVal=(int)scanner->value.v_int;
         if (Globals[GlobalIndex].ResizeFunc) {
            (*(Globals[GlobalIndex].ResizeFunc))(IntVal);
            g_print(_("Resized structure list to %d elements\n"),IntVal);
            for (list=FirstClient;list;list=g_slist_next(list)) {
               tmp=(Player *)list->data;
               UpdatePlayer(tmp);
            }
            for (list=FirstServer;list;list=g_slist_next(list)) {
               tmp=(Player *)list->data;
               UpdatePlayer(tmp);
            }
         }
         *((int *)GetGlobalPointer(GlobalIndex,StructIndex))=IntVal;
      } else {
         g_scanner_unexp_token(scanner,G_TOKEN_INT,NULL,NULL,
                               NULL,NULL,FALSE); return;
      }
   } else if (Globals[GlobalIndex].PriceVal) {
      if (token==G_TOKEN_INT) {
         *((price_t *)GetGlobalPointer(GlobalIndex,StructIndex))=
                          (price_t)scanner->value.v_int;
      } else {
         g_scanner_unexp_token(scanner,G_TOKEN_INT,NULL,NULL,
                               NULL,NULL,FALSE); return;
      }
   } else if (Globals[GlobalIndex].StringVal) {
      if (token==G_TOKEN_STRING) {
         AssignName((gchar **)GetGlobalPointer(GlobalIndex,StructIndex),
                    scanner->value.v_string);
      } else {
         g_scanner_unexp_token(scanner,G_TOKEN_STRING,NULL,NULL,
                               NULL,NULL,FALSE); return;
      }
   } else if (Globals[GlobalIndex].StringList) {
      if (IndexGiven) {
         if (token==G_TOKEN_STRING) {
            AssignName(&(*(Globals[GlobalIndex].StringList))[StructIndex-1],
                       scanner->value.v_string);
         } else {
            g_scanner_unexp_token(scanner,G_TOKEN_STRING,NULL,NULL,
                                  NULL,NULL,FALSE); return;
         }
      } else {
         StartList=NULL;
         if (token!=G_TOKEN_LEFT_CURLY) {
            g_scanner_unexp_token(scanner,G_TOKEN_LEFT_CURLY,NULL,NULL,
                                  NULL,NULL,FALSE); return;
         }
         NewNum=0;
         while(1) {
            token=g_scanner_get_next_token(scanner);
            tmpstr=NULL;
            if (token==G_TOKEN_STRING) {
               tmpstr=g_strdup(scanner->value.v_string);
            } else if (token==G_TOKEN_RIGHT_CURLY) {
               break;
            } else if (token==G_TOKEN_COMMA) {
            } else {
               g_scanner_unexp_token(scanner,G_TOKEN_STRING,NULL,NULL,
                                     NULL,NULL,FALSE); return;
            }
            if (tmpstr) {
               NewNum++; StartList=g_slist_append(StartList,tmpstr);
            }
         }
         (*Globals[GlobalIndex].ResizeFunc)(NewNum);
         NewNum=0;
         for (list=StartList;list;NewNum++,list=g_slist_next(list)) {
            AssignName(&(*(Globals[GlobalIndex].StringList))[NewNum],
                       (char *)list->data);
            g_free(list->data);
         }
         g_slist_free(StartList);
      }
   }
   if (Globals[GlobalIndex].NameStruct[0]) g_free(GlobalName);
}

void SetupParameters() {
/* Sets up data - such as the location of the high score file - to    */
/* hard-coded internal values, and then processes the global and      */
/* user-specific configuration files                                  */
   char *ConfigFile,*pt;
   int i;

/* Initialise variables */
   srand(time(NULL));
   PidFile=NULL;
   Location=NULL;
   Gun=NULL;
   Drug=NULL;
   SubwaySaying=Playing=StoppedTo=NULL;
   NumLocation=NumGun=NumDrug=0;
   FirstClient=FirstServer=NULL;
   Noone.Name=g_strdup("Noone");
   WantColour=WantNetwork=1;
   WantHelp=WantVersion=WantAntique=0;
   WantedClient=CLIENT_AUTO;
   Server=AIPlayer=Client=Network=FALSE;

/* Set hard-coded default values */
   g_free(HiScoreFile); g_free(ServerName); g_free(Pager);
   HiScoreFile=g_strdup_printf("%s/dopewars.sco",DATADIR);
   ServerName=g_strdup("localhost");
   Pager=g_strdup("more");

   CopyNames(&Names,&DefaultNames);
   CopyMetaServer(&MetaServer,&DefaultMetaServer);
   CopyDrugs(&Drugs,&DefaultDrugs);

   ResizeLocations(NUMLOCATION);
   for (i=0;i<NumLocation;i++) CopyLocation(&Location[i],&DefaultLocation[i]);
   ResizeGuns(NUMGUN);
   for (i=0;i<NumGun;i++) CopyGun(&Gun[i],&DefaultGun[i]);
   ResizeDrugs(NUMDRUG);
   for (i=0;i<NumDrug;i++) CopyDrug(&Drug[i],&DefaultDrug[i]);
   ResizeSubway(NUMSUBWAY);
   for (i=0;i<NumSubway;i++) {
      AssignName(&SubwaySaying[i],_(DefaultSubwaySaying[i]));
   }
   ResizePlaying(NUMPLAYING);
   for (i=0;i<NumPlaying;i++) {
      AssignName(&Playing[i],_(DefaultPlaying[i]));
   }
   ResizeStoppedTo(NUMSTOPPEDTO);
   for (i=0;i<NumStoppedTo;i++) {
      AssignName(&StoppedTo[i],_(DefaultStoppedTo[i]));
   }

/* Now read in the global configuration file */
   ReadConfigFile("/etc/dopewars");

/* Finally, try to read in the .dopewars file in the user's home directory */
   pt=getenv("HOME");
   if (!pt) return;
   ConfigFile=g_strdup_printf("%s/.dopewars",pt);
   ReadConfigFile(ConfigFile);
   g_free(ConfigFile);
}

void HandleHelpTexts() {
   g_print("dopewars version %s\n",VERSION);
   if (!WantHelp) return;

   g_print(
_("Usage: dopewars [OPTION]...\n\
Drug dealing game based on \"Drug Wars\" by John E. Dell\n\
  -b       \"black and white\" - i.e. do not use pretty colours\n\
              (by default colours are used where the terminal supports them)\n\
  -n       be boring and don't connect to any available dopewars servers\n\
              (i.e. single player mode)\n\
  -a       \"antique\" dopewars - keep as closely to the original version as\n\
              possible (this also disables any networking)\n\
  -f file  specify a file to use as the high score table\n\
              (by default %s/dopewars.sco is used)\n\
  -o addr  specify a hostname where the server for multiplayer dopewars\n\
              can be found (in human-readable - e.g. nowhere.com - format)\n\
  -s       run in server mode (note: for a \"non-interactive\" server, simply\n\
              run as dopewars -s < /dev/null >> logfile & )\n\
  -S       run a \"private\" server (i.e. do not notify the metaserver)\n\
  -p       specify the network port to use (default: 7902)\n\
  -g file  specify the pathname of a dopewars configuration file. This file\n\
              is read immediately when the -g option is encountered\n\
  -r file  maintain pid file \"file\" while running the server\n\
  -c       create and run a computer player\n\
  -w       force the use of a graphical (windowed) client (GTK+ or Win32)\n\
  -t       force the use of a text-mode client (curses)\n\
              (by default, a windowed client is used when possible)\n\
  -h       display this help information\n\
  -v       output version information and exit\n\n\
dopewars is Copyright (C) Ben Webb 1998-2000, and released under the GNU GPL\n\
Report bugs to the author at ben@bellatrix.pcl.ox.ac.uk\n"),DATADIR);
}

void HandleCmdLine(int argc,char *argv[]) {
   int c;
   while (1) {
      c=getopt(argc,argv,"anbchvf:o:sSp:g:r:wt");
      if (c==EOF) break;
      switch(c) {
         case 'n': WantNetwork=0; break;
         case 'b': WantColour=0; break;
         case 'c': AIPlayer=1; break;
         case 'a': WantAntique=1; WantNetwork=0; break;
         case 'v': WantVersion=1; break;
         case 'h':
         case '?': WantHelp=1; break;
         case 'f': AssignName(&HiScoreFile,optarg); break;
         case 'o': AssignName(&ServerName,optarg); break;
         case 's': Server=TRUE; NotifyMetaServer=TRUE; break;
         case 'S': Server=TRUE; NotifyMetaServer=FALSE; break;
         case 'p': Port=atoi(optarg); break;
         case 'g': ReadConfigFile(optarg); break;
         case 'r': AssignName(&PidFile,optarg); break;
         case 'w': WantedClient=CLIENT_WINDOW; break;
         case 't': WantedClient=CLIENT_CURSES; break;
      }
   }
}

#ifdef CYGWIN

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                     LPSTR lpszCmdParam,int nCmdShow) {
   return Win32Loop(hInstance,hPrevInstance,lpszCmdParam,nCmdShow);
}

#else /* !CYGWIN */

int main(int argc,char *argv[]) {
#ifdef ENABLE_NLS
   setlocale(LC_ALL,"");
   bindtextdomain(PACKAGE,LOCALEDIR);
   textdomain(PACKAGE);
#endif
   SetupParameters();
   HandleCmdLine(argc,argv);
   if (WantVersion || WantHelp) {
      HandleHelpTexts();
   } else {
      StartNetworking();
      if (Server) {
         ServerLoop();
      } else switch(WantedClient) {
         case CLIENT_AUTO:
            if (!GtkLoop(&argc,&argv,TRUE)) CursesLoop();
            break;
         case CLIENT_WINDOW:
            GtkLoop(&argc,&argv,FALSE); break;
         case CLIENT_CURSES:
            CursesLoop(); break;
      }
      StopNetworking();
   }
   g_free(PidFile);
   return 0;
}

#endif /* CYGWIN */
