/* message.c  Message-handling routines for dopewars                    */
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <errno.h>
#include "dopeos.h"
#include "dopewars.h"
#include "serverside.h"
#include "message.h"

/* Maximum sizes (in bytes) of read and write buffers - connections should
   be dropped if either buffer is filled */
#define MAXREADBUF   (32768)
#define MAXWRITEBUF  (65536)

/* dopewars is built around a client-server model. Each client handles the
   user interface, but all the important calculation and processing is
   handled by the server. All communication is conducted via. TCP by means
   of plain text newline-delimited messages.

   New message structure:-
       Other^ACData

   Other:   The ID of the player at the "other end" of the connection;
            for messages received by the client, this is the player from
            which the message originates, while for messages received by
            the server, it's the player to which to deliver the message.
   A:       One-letter code; used by AI players to identify the message subtype
                                       (check AIPlayer.h)
   C:       One-letter code to identify the message type (check message.h)
   Data:    Message-dependent information


   For compatibility with old clients and servers, the old message structure
   is still supported:-
       From^To^ACData

   From,To:  Player names identifying the sender and intended recipient of
             the message. Either field may be blank, although the server will
             usually reject incoming messages if they are not properly
             identified with a correct "From" field.
   A,C,Data: As above, for the new message structure

   For example, a common message is the "printmessage" message (message code 
   C is C_PRINTMESSAGE), which simply instructs the client to display "Data". 
   Any ^ characters within Data are replaced by newlines on output. So in order 
   for the server to instruct player "Fred" (ID 1) to display "Hello world" it
   would send the message:-
       ^AAHello world                   (new format)
       ^Fred^AAHello world              (old format)
   Note that the server has left the Other (or From) field blank, and has
   specified the AI code 'A' - defined in AIPlayer.h as C_NONE (i.e. an
   "unimportant" message) as well as the main code 'A', defined as
   C_PRINTMESSAGE in message.h. Note also that the destination player (Fred)
   is not specified in the new format; the player is identified by the socket
   through which the message is transmitted.

   When the network is down, a server is simulated locally. Messages from
   the client are passed directly to the server message handling routine,
   and vice versa.  */

GSList *FirstClient;

void (*ClientMessageHandlerPt) (char *,Player *) = NULL;
void (*SocketWriteTestPt) (Player *,gboolean) = NULL;

void SendClientMessage(Player *From,char AICode,char Code,
                       Player *To,char *Data) {
/* Sends a message from player "From" to player "To" via. the server.     */
/* AICode, Code and Data define the message.                              */
   DoSendClientMessage(From,AICode,Code,To,Data,From);
}

void SendNullClientMessage(Player *From,char AICode,char Code,
                           Player *To,char *Data) {
/* Sends a message from player "From" to player "To" via. the server,     */
/* sending a blank name for "From" (this is required with the old message */
/* format, up to and including the first successful C_NAME message, but   */
/* has no effect with the new format. AICode, Code and Data define the    */
/* message.                                                               */
   DoSendClientMessage(NULL,AICode,Code,To,Data,From);
}

void DoSendClientMessage(Player *From,char AICode,char Code,
                         Player *To,char *Data,Player *BufOwn) {
/* Send a message from client player "From" with computer code "AICode",  */
/* human-readable code "Code" and data "Data". The message is sent to the */
/* server, identifying itself as for "To". "BufOwn" identifies the player */
/* which owns the buffers used for the actual wire connection. With the   */
/* new message format, "From" is ignored. From, To, or Data may be NULL.  */
   GString *text;
   Player *ServerFrom;
   g_assert(BufOwn!=NULL);
   text=g_string_new(NULL);
   if (HaveAbility(BufOwn,A_PLAYERID)) {
      if (To) g_string_sprintfa(text,"%d",To->ID);
      g_string_sprintfa(text,"^%c%c%s",AICode,Code,Data ? Data : "");
   } else {
      g_string_sprintf(text,"%s^%s^%c%c%s",From ? GetPlayerName(From) : "",
                       To ? GetPlayerName(To) : "",AICode,Code,
                       Data ? Data : "");
   }
#if NETWORKING
   if (!Network) {
#endif
      if (From) ServerFrom=GetPlayerByName(GetPlayerName(From),FirstServer);
      else if (FirstServer) ServerFrom=(Player *)(FirstServer->data);
      else {
         ServerFrom=g_new(Player,1);
         FirstServer=AddPlayer(0,ServerFrom,FirstServer);
      }
      HandleServerMessage(text->str,ServerFrom);
#if NETWORKING
   } else {
      WriteToConnectionBuffer(BufOwn,text->str);
      if (SocketWriteTestPt) (*SocketWriteTestPt)(BufOwn,TRUE);
   }
#endif /* NETWORKING */
   g_string_free(text,TRUE);
}

void SendPrintMessage(Player *From,char AICode,
                      Player *To,char *Data) {
/* Shorthand for the server sending a "printmessage"; instructs the */
/* client "To" to display "Data"                                    */
   SendServerMessage(From,AICode,C_PRINTMESSAGE,To,Data);
}

void SendQuestion(Player *From,char AICode,
                  Player *To,char *Data) {
/* Shorthand for the server sending a "question"; instructs the client  */
/* "To" to display the second word of Data and accept any letter within */
/* the first word of Data as suitable reply                             */
   SendServerMessage(From,AICode,C_QUESTION,To,Data);
}

void SendServerMessage(Player *From,char AICode,char Code,
                       Player *To,char *Data) {
/* Sends a message from the server to client player "To" with computer    */
/* code "AICode", human-readable code "Code" and data "Data", claiming    */
/* to be from player "From"                                               */
   GString *text;
   text=g_string_new(NULL);
   if (HaveAbility(To,A_PLAYERID)) {
      if (From) g_string_sprintfa(text,"%d",From->ID);
      g_string_sprintfa(text,"^%c%c%s",AICode,Code,Data ? Data : "");
   } else {
      g_string_sprintf(text,"%s^%s^%c%c%s",From ? GetPlayerName(From) : "",
                       To ? GetPlayerName(To) : "",AICode,Code,
                       Data ? Data : "");
   }
#if NETWORKING
   if (!Network) {
#endif
      if (ClientMessageHandlerPt) {
         (*ClientMessageHandlerPt)(text->str,(Player *)(FirstClient->data));
      }
#if NETWORKING
   } else {
      WriteToConnectionBuffer(To,text->str);
      if (SocketWriteTestPt) (*SocketWriteTestPt)(To,TRUE);
   }
#endif
   g_string_free(text,TRUE);
}

void InitAbilities(Player *Play) {
/* Sets up the "abilities" of player "Play". Abilities are used to extend */
/* the protocol; if both the client and server share an ability, then the */
/* protocol extension can be used.                                        */
   int i;
/* Clear all abilities */
   for (i=0;i<A_NUM;i++) {
      Play->Abil.Local[i]= FALSE;
      Play->Abil.Remote[i]=FALSE;
      Play->Abil.Shared[i]=FALSE;
   }
/* Set local abilities */
   Play->Abil.Local[A_PLAYERID]=TRUE;
   if (!Network) for (i=0;i<A_NUM;i++) {
      Play->Abil.Remote[i]=Play->Abil.Shared[i]=Play->Abil.Local[i];
   }
}

void SendAbilities(Player *Play) {
/* Sends abilities of player "Play" to the other end of the client-server */
/* connection.                                                            */
   int i;
   gchar Data[A_NUM+1];
   if (!Network) return;
   for (i=0;i<A_NUM;i++) Data[i]= (Play->Abil.Local[i] ? '1' : '0');
   Data[A_NUM]='\0';
   if (Server) SendServerMessage(NULL,C_NONE,C_ABILITIES,Play,Data);
   else SendClientMessage(Play,C_NONE,C_ABILITIES,NULL,Data);
}

void ReceiveAbilities(Player *Play,gchar *Data) {
/* Fills in the "remote" abilities of player "Play" using the message data */
/* in "Data". These are the abilities of the server/client at the other    */
/* end of the connection.                                                  */
   int i,Length;
   InitAbilities(Play);
   if (!Network) return;
   Length=MIN(strlen(Data),A_NUM);
   for (i=0;i<Length;i++) {
      Play->Abil.Remote[i]= (Data[i]=='1' ? TRUE : FALSE);
   }
}

void CombineAbilities(Player *Play) {
/* Combines the local and remote abilities of player "Play". The resulting */
/* shared abilities are used to determine when protocol extensions can be  */
/* used.                                                                   */
   int i;
   for (i=0;i<A_NUM;i++) {
      Play->Abil.Shared[i]= (Play->Abil.Remote[i] && Play->Abil.Local[i]);
   }
}

gboolean HaveAbility(Player *Play,gint Type) {
/* Returns TRUE if ability "Type" is one of player "Play"'s shared abilities */
   if (Type<0 || Type>=A_NUM) return FALSE;
   else return (Play->Abil.Shared[Type]);
}

#if NETWORKING
gchar *ReadFromConnectionBuffer(Player *Play) {
/* Reads a newline-terminated message from "Play"'s read buffer. The message */
/* is removed from the buffer, and returned as a null-terminated string (the */
/* terminating newline is removed). If no complete message is waiting, NULL  */
/* is returned. The string is dynamically allocated, and must be g_free'd by */
/* the caller.                                                               */
   ConnBuf *conn;
   int MessageLen;
   char *SepPt;
   gchar *NewMessage;
   conn=&Play->ReadBuf;
   if (!conn->Data || !conn->DataPresent) return NULL;
   SepPt=memchr(conn->Data,'\n',conn->DataPresent);
   if (!SepPt) return NULL;
   *SepPt='\0';
   MessageLen=SepPt-conn->Data+1;
   NewMessage=g_new(gchar,MessageLen);
   memcpy(NewMessage,conn->Data,MessageLen);
   if (MessageLen<conn->DataPresent) {
      memmove(&conn->Data[0],&conn->Data[MessageLen],
              conn->DataPresent-MessageLen);
   }
   conn->DataPresent-=MessageLen;
   return NewMessage;
}

gboolean ReadConnectionBufferFromWire(Player *Play) {
/* Reads any waiting data on the TCP/IP connection for player "Play" into */
/* the player's read buffer. Returns FALSE if the connection was closed,  */
/* or if the read buffer's maximum size was reached.                      */
   ConnBuf *conn;
   int CurrentPosition,BytesRead;
   conn=&Play->ReadBuf;
   CurrentPosition=conn->DataPresent;
   while(1) {
      if (CurrentPosition>=conn->Length) {
         if (conn->Length==MAXREADBUF) {
            return FALSE; /* drop connection */
         }
         if (conn->Length==0) conn->Length=256; else conn->Length*=2;
         if (conn->Length>MAXREADBUF) conn->Length=MAXREADBUF;
         conn->Data=g_realloc(conn->Data,conn->Length);
      }
      BytesRead=recv(Play->fd,&conn->Data[CurrentPosition],
                     conn->Length-CurrentPosition,0);
      if (BytesRead==SOCKET_ERROR) {
         break;
      } else if (BytesRead==0) {
         return FALSE;
      } else {
         CurrentPosition+=BytesRead;
      }
   }
   conn->DataPresent=CurrentPosition;
   return TRUE;
}

void WriteToConnectionBuffer(Player *Play,gchar *data) {
/* Writes the null-terminated string "data" to "Play"'s connection buffer. */
/* The message is automatically newline-terminated. Fails to write the     */
/* message without error if the buffer reaches its maximum size (although  */
/* this error will be detected when the buffer is attempted to be written  */
/* to the wire, below)                                                     */
   int AddLength,NewLength;
   ConnBuf *conn;
   conn=&Play->WriteBuf;
   AddLength=strlen(data)+1;
   NewLength=conn->DataPresent+AddLength;
   if (NewLength > conn->Length) {
      conn->Length*=2;
      conn->Length=MAX(conn->Length,NewLength);
      if (conn->Length > MAXWRITEBUF) conn->Length=MAXWRITEBUF;
      if (NewLength > conn->Length) return;
      conn->Data=g_realloc(conn->Data,conn->Length);
   }
   memcpy(&conn->Data[conn->DataPresent],data,AddLength);
   conn->DataPresent=NewLength;
   conn->Data[NewLength-1]='\n';
}

gboolean WriteConnectionBufferToWire(Player *Play) {
/* Writes any waiting data in "Play"'s connection buffer to the wire.  */
/* Returns TRUE on success, or FALSE if the buffer's maximum length is */
/* reached, or the remote end has closed the connection.               */
   ConnBuf *conn;
   int CurrentPosition,BytesSent;
   conn=&Play->WriteBuf;
   if (!conn->Data || !conn->DataPresent) return TRUE;
   if (conn->Length==MAXWRITEBUF) return FALSE;
   CurrentPosition=0;
   while (CurrentPosition<conn->DataPresent) {
      BytesSent=send(Play->fd,&conn->Data[CurrentPosition],
                     conn->DataPresent-CurrentPosition,0);
      if (BytesSent==SOCKET_ERROR) {
         if (errno==EPIPE) return FALSE;
         break;
      } else {
         CurrentPosition+=BytesSent;
      }
   }
   if (CurrentPosition>0 && CurrentPosition<conn->DataPresent) {
      memmove(&conn->Data[0],&conn->Data[CurrentPosition],
              conn->DataPresent-CurrentPosition);
   }
   conn->DataPresent-=CurrentPosition;
   return TRUE;
}

gchar *bgets(int fd) {
/* Drop-in substitute for fgets; reads a newline-terminated string from  */
/* file descriptor fd, into a dynamically-allocated buffer. Returns a    */
/* pointer to the buffer, or NULL if an error occurred. It is the user's */
/* responsibility to g_free the pointer when it is no longer needed.     */
/* Used for non-blocking read from TCP sockets.                          */
/* N.B. The terminating newline is _not_ returned in the string.         */
   ssize_t len;
   unsigned TotalLen=0;
   GString *text;
   gchar *buffer;
   char tmp[10];
   text=g_string_new(NULL);
   for (;;) {
      len=recv(fd,tmp,1,0);
      if (len==SOCKET_ERROR) { g_string_free(text,TRUE); return NULL; }
      if (len==0) { g_string_free(text,TRUE); return NULL; }
      if (tmp[0]=='\n') {
         buffer=text->str;
         g_string_free(text,FALSE);  /* Just free the g_string, not the data */
         return buffer;
      } else {
         g_string_append_c(text,tmp[0]);
         TotalLen++;
         /* Test to make sure dodgy clients don't eat all of our nice memory */
         if (TotalLen > 64000) {
            g_warning("Abnormally large packet");
            g_string_free(text,TRUE); return NULL;
         }
      }
   }
}
#endif /* NETWORKING */

void chomp(char *str) {
/* Removes a terminating newline from "str", if one is present. */
   int len=strlen(str);
   if (str[len-1]=='\n') str[len-1]=0;
}

void BroadcastToClients(char AICode,char Code,char *Data,
                        Player *From,Player *Except) {
/* Sends the message made up of AICode,Code and Data to all players except */
/* "Except" (if non-NULL). It will be sent by the server, and on behalf of */
/* player "From"                                                           */
   Player *tmp;
   GSList *list;
   for (list=FirstServer;list;list=g_slist_next(list)) {
      tmp=(Player *)list->data;
      if (tmp!=Except) SendServerMessage(From,AICode,Code,tmp,Data);
   }
}

void SendInventory(Player *From,char AICode,char Code,
                   Player *To,Inventory *Guns,Inventory *Drugs) {
/* Encodes an Inventory structure into a string, and sends it as the data */
/* with a server message constructed from the other arguments.            */ 
   int i;
   GString *text;
   text=g_string_new(NULL);
   for (i=0;i<NumGun;i++) {
      g_string_sprintfa(text,"%d:",Guns ? Guns[i].Carried : 0);
   }
   for (i=0;i<NumDrug;i++) {
      g_string_sprintfa(text,"%d:",Drugs ? Drugs[i].Carried : 0);
   }
   SendServerMessage(From,AICode,Code,To,text->str);
   g_string_free(text,TRUE);
}

void ReceiveInventory(char *Data,Inventory *Guns,Inventory *Drugs) {
/* Decodes a string representation (in "Data") to its original Inventory */
/* contents, and stores it in "Guns" and "Drugs" if non-NULL             */
   int i,val;
   char *pt;
   pt=Data;
   for (i=0;i<NumGun;i++) {
      val=GetNextInt(&pt,0);
      if (Guns) Guns[i].Carried=val;
   }
   for (i=0;i<NumDrug;i++) {
      val=GetNextInt(&pt,0);
      if (Drugs) Drugs[i].Carried=val;
   }
}

void SendPlayerData(Player *To) {
/* Sends all pertinent data about player "To" from the server to player "To" */
   SendSpyReport(NULL,To);
}

void SendSpyReport(Player *To,Player *SpiedOn) {
/* Sends pertinent data about player "SpiedOn" from the server to player "To" */
   gchar *cashstr,*debtstr,*bankstr;
   GString *text;
   int i;
   text=g_string_new(NULL);
   g_string_sprintf(text,"%s^%s^%s^%d^%d^%d^%d^%d^",
                    (cashstr=pricetostr(SpiedOn->Cash)),
                    (debtstr=pricetostr(SpiedOn->Debt)),
                    (bankstr=pricetostr(SpiedOn->Bank)),
                    SpiedOn->Health,SpiedOn->CoatSize,
                    SpiedOn->IsAt,SpiedOn->Turn,SpiedOn->Flags);
   g_free(cashstr); g_free(debtstr); g_free(bankstr);
   for (i=0;i<NumGun;i++) {
      g_string_sprintfa(text,"%d^",SpiedOn->Guns[i].Carried);
   }
   for (i=0;i<NumDrug;i++) {
      g_string_sprintfa(text,"%d^",SpiedOn->Drugs[i].Carried);
   }
   g_string_sprintfa(text,"%d",SpiedOn->Bitches.Carried);
   if (To) SendServerMessage(SpiedOn,C_NONE,C_UPDATE,To,text->str);
   else SendServerMessage(NULL,C_NONE,C_UPDATE,SpiedOn,text->str);
   g_string_free(text,TRUE);
}

void SendInitialData(Player *To) {
   GString *text;
   if (!Network) return;
   text=g_string_new("");
   g_string_sprintf(text,"%s^%d^%d^%d^%s^%s^%s^%s^%s^%s^%s^%s^",
                         VERSION,NumLocation,NumGun,NumDrug,
                         Names.Bitch,Names.Bitches,Names.Gun,Names.Guns,
                         Names.Drug,Names.Drugs,Names.Month,Names.Year);
   if (HaveAbility(To,A_PLAYERID)) g_string_sprintfa(text,"%d^",To->ID);
   SendServerMessage(NULL,C_NONE,C_INIT,To,text->str);
   g_string_free(text,TRUE);
}

void ReceiveInitialData(Player *Play,char *Data) {
   char *pt,*ServerVersion;
   GSList *list;
   pt=Data;
   ServerVersion=GetNextWord(&pt,"(unknown)");
   ResizeLocations(GetNextInt(&pt,NumLocation));
   ResizeGuns(GetNextInt(&pt,NumGun));
   ResizeDrugs(GetNextInt(&pt,NumDrug));
   for (list=FirstClient;list;list=g_slist_next(list)) {
      UpdatePlayer((Player*)list->data);
   }
   AssignName(&Names.Bitch,GetNextWord(&pt,""));
   AssignName(&Names.Bitches,GetNextWord(&pt,""));
   AssignName(&Names.Gun,GetNextWord(&pt,""));
   AssignName(&Names.Guns,GetNextWord(&pt,""));
   AssignName(&Names.Drug,GetNextWord(&pt,""));
   AssignName(&Names.Drugs,GetNextWord(&pt,""));
   AssignName(&Names.Month,GetNextWord(&pt,""));
   AssignName(&Names.Year,GetNextWord(&pt,""));
   if (HaveAbility(Play,A_PLAYERID)) Play->ID=GetNextInt(&pt,0);
   if (strcmp(VERSION,ServerVersion)!=0) {
      g_message(_("This server is version %s, while your client is "
"version %s.\nBe warned that different versions may not be fully compatible!\n"
"Refer to the website at http://bellatrix.pcl.ox.ac.uk/~ben/dopewars/\n"
"for the latest version."),ServerVersion,VERSION);
   }
}

void SendMiscData(Player *To) {
   gchar *text,*prstr[2];
   int i;
   if (!Network) return;
   text=g_strdup_printf("0^%c%s^%s^",DT_PRICES,
                        (prstr[0]=pricetostr(Prices.Spy)),
                        (prstr[1]=pricetostr(Prices.Tipoff)));
   SendServerMessage(NULL,C_NONE,C_DATA,To,text);
   g_free(prstr[0]); g_free(prstr[1]); g_free(text);
   for (i=0;i<NumGun;i++) {
      text=g_strdup_printf("%d^%c%s^%s^%d^%d^",i,DT_GUN,Gun[i].Name,
                           (prstr[0]=pricetostr(Gun[i].Price)),
                           Gun[i].Space,Gun[i].Damage);
      SendServerMessage(NULL,C_NONE,C_DATA,To,text);
      g_free(prstr[0]); g_free(text);
   }
   for (i=0;i<NumDrug;i++) {
      text=g_strdup_printf("%d^%c%s^%s^%s^",i,DT_DRUG,Drug[i].Name,
                           (prstr[0]=pricetostr(Drug[i].MinPrice)),
                           (prstr[1]=pricetostr(Drug[i].MaxPrice)));
      SendServerMessage(NULL,C_NONE,C_DATA,To,text);
      g_free(prstr[0]); g_free(prstr[1]); g_free(text);
   }
   for (i=0;i<NumLocation;i++) {
      text=g_strdup_printf("%d^%c%s^",i,DT_LOCATION,Location[i].Name);
      SendServerMessage(NULL,C_NONE,C_DATA,To,text);
      g_free(text);
   }
}

void ReceiveMiscData(char *Data) {
/* Decodes information about locations, drugs, prices, etc. in "Data" */
   char *pt,*Name,Type;
   int i;
   pt=Data;
   i=GetNextInt(&pt,0);
   Name=GetNextWord(&pt,"");
   Type=Name[0];
   if (strlen(Name)>1) switch(Type) {
      case DT_LOCATION:
         if (i>=0 && i<NumLocation) {
            AssignName(&Location[i].Name,&Name[1]);
            Location[i].PolicePresence=10;
            Location[i].MinDrug=NumDrug/2+1;
            Location[i].MaxDrug=NumDrug;
         }
         break;
      case DT_GUN:
         if (i>=0 && i<NumGun) {
            AssignName(&Gun[i].Name,&Name[1]);
            Gun[i].Price=GetNextPrice(&pt,0);
            Gun[i].Space=GetNextInt(&pt,0);
            Gun[i].Damage=GetNextInt(&pt,0);
         }
         break;
      case DT_DRUG:
         if (i>=0 && i<NumDrug) {
            AssignName(&Drug[i].Name,&Name[1]);
            Drug[i].MinPrice=GetNextPrice(&pt,0);
            Drug[i].MaxPrice=GetNextPrice(&pt,0);
         }
         break;
      case DT_PRICES:
         Prices.Spy=strtoprice(&Name[1]);
         Prices.Tipoff=GetNextPrice(&pt,0);
         break;
   }
}

void ReceivePlayerData(char *text,Player *From) {
/* Decode player data from the string "text" into player "From" */
   char *cp;
   int i;
   cp=text;
   From->Cash=GetNextPrice(&cp,0);
   From->Debt=GetNextPrice(&cp,0);
   From->Bank=GetNextPrice(&cp,0);
   From->Health=GetNextInt(&cp,100);
   From->CoatSize=GetNextInt(&cp,0);
   From->IsAt=GetNextInt(&cp,0);
   From->Turn=GetNextInt(&cp,0);
   From->Flags=GetNextInt(&cp,0);
   for (i=0;i<NumGun;i++) {
      From->Guns[i].Carried=GetNextInt(&cp,0);
   }
   for (i=0;i<NumDrug;i++) {
      From->Drugs[i].Carried=GetNextInt(&cp,0);
   }
   From->Bitches.Carried=GetNextInt(&cp,0);
}

gchar *GetNextWord(gchar **Data,gchar *Default) {
   gchar *Word;
   if (*Data==NULL || **Data=='\0') return Default;
   Word=*Data;
   while (**Data!='\0' && **Data!='^') (*Data)++;
   if (**Data=='\0') {
      *Data=NULL;
   } else {
      **Data='\0'; (*Data)++;
   }
   return Word;
}

void AssignNextWord(gchar **Data,gchar **Dest) {
   if (!Dest) return;
   g_free(*Dest);
   *Dest=g_strdup(GetNextWord(Data,""));
}

int GetNextInt(gchar **Data,int Default) {
   gchar *Word=GetNextWord(Data,NULL);
   if (Word) return atoi(Word); else return Default;
}

price_t GetNextPrice(gchar **Data,price_t Default) {
   gchar *Word=GetNextWord(Data,NULL);
   if (Word) return strtoprice(Word); else return Default;
}

#if NETWORKING
char *SetupNetwork(gboolean NonBlocking) {
/* Sets up the connection from the client to the server. If the connection */
/* is successful, Network and Client are set to TRUE, and ClientSock is a  */
/* file descriptor for the newly-opened socket. NULL is returned. If the   */
/* connection fails, a pointer to an error message is returned.            */
/* If "NonBlocking" is TRUE, a non-blocking connect() is carried out. In   */
/* this case, the routine returns successfully after initiating the        */
/* connect call; the caller should then select() the socket for writing,   */
/* before calling FinishSetupNetwork()                                     */
   struct sockaddr_in ClientAddr;
   struct hostent *he;
   static char NoHost[]= N_("Could not find host");
   static char NoSocket[]= N_("Could not create network socket");
   static char NoConnect[]= N_("Connection refused or no server present");

   Network=Client=Server=FALSE;

   if ((he=gethostbyname(ServerName))==NULL) {
      return NoHost;
   }
   ClientSock=socket(AF_INET,SOCK_STREAM,0);
   if (ClientSock==SOCKET_ERROR) {
      return NoSocket;
   }

   ClientAddr.sin_family=AF_INET;
   ClientAddr.sin_port=htons(Port);
   ClientAddr.sin_addr=*((struct in_addr *)he->h_addr);
   memset(ClientAddr.sin_zero,0,sizeof(ClientAddr.sin_zero));

   if (NonBlocking) fcntl(ClientSock,F_SETFL,O_NONBLOCK);
   if (connect(ClientSock,(struct sockaddr *)&ClientAddr,
       sizeof(struct sockaddr))==-1) {
      if (errno==EINPROGRESS) return NULL;
      CloseSocket(ClientSock);
      return NoConnect;
   } else {
      fcntl(ClientSock,F_SETFL,O_NONBLOCK);
   }
   Client=TRUE; Network=TRUE;
   return NULL;
}

char *FinishSetupNetwork() {
   socklen_t optlen;
   int optval;
   static char NoConnect[]= N_("Connection refused or no server present");

   optlen=sizeof(optval);
   if (getsockopt(ClientSock,SOL_SOCKET,SO_ERROR,&optval,&optlen)==-1) {
      return NoConnect;
   }
   if (optval==0) {
      Client=TRUE; Network=TRUE;
      return NULL;
   } else {
      return NoConnect;
   }
}

#endif /* NETWORKING */

void SwitchToSinglePlayer(Player *Play) {
/* Called when the client is pushed off the server, or the server  */
/* terminates. Using the client information, starts a local server */
/* to reproduce the current game situation as best as possible so  */
/* that the game can be continued in single player mode            */
   Player *NewPlayer;
   if (!Network || !Client || !FirstClient) return;
   if (Play!=FirstClient->data) {
      g_error("Oops! FirstClient should be player!");
   }
   while (g_slist_next(FirstClient)) {
      FirstClient=RemovePlayer((Player *)g_slist_next(FirstClient)->data,
                               FirstClient);
   }
   CloseSocket(ClientSock);
   CleanUpServer();
   Network=Server=Client=FALSE;
   NewPlayer=g_new(Player,1);
   FirstServer=AddPlayer(0,NewPlayer,FirstServer);
   CopyPlayer(NewPlayer,Play);
   NewPlayer->Flags=0;
   NewPlayer->EventNum=E_ARRIVE;
   SendEvent(NewPlayer);
}

void ShutdownNetwork() {
/* Closes down the client side of the network connection. Clears the list */
/* of client players, and closes the network socket.                      */
   while (FirstClient) {
      FirstClient=RemovePlayer((Player *)FirstClient->data,FirstClient);
   }
#if NETWORKING
   if (Client) {
      CloseSocket(ClientSock);
   }
#endif /* NETWORKING */
   Client=Network=Server=FALSE;
}

int ProcessMessage(char *Msg,Player *Play,Player **Other,char *AICode,
                   char *Code,char **Data,GSList *First) {
/* Given a "raw" message in "Msg" and a pointer to the start of the linked   */
/* list of known players in "First", sets the other arguments to the message */
/* fields. Data is returned as a pointer into the message "Msg", and should  */
/* therefore NOT be g_free'd. "Play" is a pointer to the player which is     */
/* receiving the message. "Other" is the player that is identified by the    */
/* message; for messages to clients, this will be the player "From" which    */
/* the message claims to be, while for messages to servers, this will be     */
/* the player "To" which to send messages. Returns 0 on success, -1 on       */
/* failure.                                                                  */
   gchar *pt,*buf;
   guint ID;

   *AICode=*Code=C_NONE;
   *Other=&Noone;
   pt=Msg;
   if (HaveAbility(Play,A_PLAYERID)) {
      buf=GetNextWord(&pt,NULL);
      if (buf[0]) {
         ID=atoi(buf);
         *Other=GetPlayerByID(ID,First);
      }
   } else {
      buf=GetNextWord(&pt,NULL);
      if (Client) *Other=GetPlayerByName(buf,First);
      buf=GetNextWord(&pt,NULL);
      if (Server) *Other=GetPlayerByName(buf,First);
   }
   if (!(*Other)) return -1;

   if (strlen(pt)>=2) {
      *AICode=pt[0];
      *Code=pt[1];
      *Data=&pt[2];
      return 0;
   }
   return -1;
}

void ReceiveDrugsHere(char *text,Player *To) {
/* Decodes the message data "text" into a list of drug prices for */
/* player "To"                                                    */
   char *cp;
   int i;

   To->EventNum=E_ARRIVE;
   cp=text;
   for (i=0;i<NumDrug;i++) {
      To->Drugs[i].Price=GetNextPrice(&cp,0);
   }
}

gboolean HandleGenericClientMessage(Player *From,char AICode,char Code,
                                    Player *To,char *Data,char *DisplayMode) {
/* Handles messages that both human clients and AI players deal with in the */
/* same way.                                                                */
   Player *tmp;
   gchar *pt;
   switch(Code) {
      case C_LIST: case C_JOIN:
         tmp=g_new(Player,1);
         FirstClient=AddPlayer(0,tmp,FirstClient);
         pt=Data;
         SetPlayerName(tmp,GetNextWord(&pt,NULL));
         if (HaveAbility(To,A_PLAYERID)) tmp->ID=GetNextInt(&pt,0);
         break;
      case C_DATA:
         ReceiveMiscData(Data); break;
      case C_INIT:
         ReceiveInitialData(To,Data); break;
      case C_ABILITIES:
         ReceiveAbilities(To,Data); CombineAbilities(To);
         break;
      case C_LEAVE:
         if (From!=&Noone) FirstClient=RemovePlayer(From,FirstClient);
         break;
      case C_TRADE:
         if (DisplayMode) *DisplayMode=DM_DEAL;
         break;
      case C_DRUGHERE:
         ReceiveDrugsHere(Data,To);
         if (DisplayMode) *DisplayMode=DM_STREET;
         break;
      case C_FIGHTPRINT:
         if (From!=&Noone) {
            From->Flags |= FIGHTING;
            To->Flags |= CANSHOOT;
         }
         if (DisplayMode) *DisplayMode=DM_FIGHT;
         break;
      case C_CHANGEDISP:
         if (DisplayMode) {
            if (Data[0]=='N' && *DisplayMode==DM_STREET) *DisplayMode=DM_NONE;
            if (Data[0]=='Y' && *DisplayMode==DM_NONE) *DisplayMode=DM_STREET;
         }
         break;
      default:
         return FALSE; break;
   }
   return TRUE;
}

char *OpenMetaServerConnection(int *HttpSock) {
   static char NoHost[] = N_("Cannot locate metaserver");
   static char NoSocket[] = N_("Cannot create socket");
   static char NoService[] =
                    N_("Metaserver not running HTTP or connection denied");
   struct sockaddr_in HttpAddr;
   struct hostent *he;

   if ((he=gethostbyname(MetaServer.Name))==NULL) return NoHost;
   if ((*HttpSock=socket(AF_INET,SOCK_STREAM,0))==-1) return NoSocket;
   HttpAddr.sin_family=AF_INET;
   HttpAddr.sin_port=htons(MetaServer.HttpPort);
   HttpAddr.sin_addr=*((struct in_addr *)he->h_addr);
   memset(HttpAddr.sin_zero,0,sizeof(HttpAddr.sin_zero));
   if (connect(*HttpSock,(struct sockaddr *)&HttpAddr,
       sizeof(struct sockaddr))==SOCKET_ERROR) {
      CloseSocket(*HttpSock);
      return NoService;
   }
   return NULL;
}

void CloseMetaServerConnection(int HttpSock) {
   CloseSocket(HttpSock);
}

void ClearServerList() {
   ServerData *ThisServer;
   while (ServerList) {
      ThisServer=(ServerData *)(ServerList->data);
      g_free(ThisServer->Name); g_free(ThisServer->Comment);
      g_free(ThisServer->Version); g_free(ThisServer->Update);
      g_free(ThisServer->UpSince); g_free(ThisServer);
      ServerList=g_slist_remove(ServerList,ThisServer);
   }
}

void ReadMetaServerData(int HttpSock) {
   gchar *buf;
   ServerData *NewServer;
   gboolean HeaderDone;

   ClearServerList();
   buf=g_strdup_printf("GET %s?output=text&getlist=%d HTTP/1.0\n\n",
                       MetaServer.Path,METAVERSION);
   send(HttpSock,buf,strlen(buf),0);
   g_free(buf);
   HeaderDone=FALSE;

   while ((buf=bgets(HttpSock))) {
      if (HeaderDone) {
         NewServer=g_new0(ServerData,1);
         NewServer->Name=buf;
         buf=bgets(HttpSock);
         NewServer->Port=atoi(buf); g_free(buf);
         NewServer->Version=bgets(HttpSock);
         buf=bgets(HttpSock);
         if (buf[0]) NewServer->CurPlayers=atoi(buf);
         else NewServer->CurPlayers=-1;
         g_free(buf);
         buf=bgets(HttpSock);
         NewServer->MaxPlayers=atoi(buf); g_free(buf);
         NewServer->Update=bgets(HttpSock);
         NewServer->Comment=bgets(HttpSock);
         NewServer->UpSince=bgets(HttpSock);
         ServerList=g_slist_append(ServerList,NewServer);
      } else {
         if (strncmp(buf,"MetaServer:",11)==0) HeaderDone=TRUE;
         g_free(buf);
      }
   }
}
