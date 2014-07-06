/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// console.c

#include "client.h"
#include "killLog.h"
#include "hitLog.h"


int g_console_field_width = 78;


#define	NUM_CON_TIMES 4

#define		CON_TEXTSIZE	32768
typedef struct {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens
	float 	yadjust;

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;
} console_t;

extern	console_t	con;

console_t	con;

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;
cvar_t 		*cl_chatcolor;
cvar_t		*con_coloredKills;
cvar_t 		*con_coloredHits;
cvar_t 		*con_prompt;
cvar_t 		*con_promptColor;
cvar_t 		*con_timePrompt;
cvar_t 		*con_timePrompt12;
cvar_t 		*con_drawscrollbar;
cvar_t 		*con_consoleHeight;
cvar_t 		*con_fadeIn;
cvar_t 		*con_margin;

cvar_t 	*con_nochat;
qboolean suppressNext = qfalse;

#define	DEFAULT_CONSOLE_WIDTH	78

vec4_t	console_color = {1.0, 1.0, 1.0, 1.0};

float opacityMult = 1;
float targetOpacityMult = 1;
void Con_RE_SetColor(vec4_t colour) {
	vec4_t c;
	if (colour) {
		c[0] = colour[0];
		c[1] = colour[1];
		c[2] = colour[2];
		c[3] = colour[3] * opacityMult;
		re.SetColor(c);
	} else {
		re.SetColor(NULL);
	}
}

void SCR_AdjustedFillRect(float x, float y, float width, float height, const float *color) {
	vec4_t c;
	if (color) {
		c[0] = color[0];
		c[1] = color[1];
		c[2] = color[2];
		c[3] = color[3] * opacityMult;
	} else {
		c[0] = 1;
		c[1] = 1;
		c[2] = 1;
		c[3] = opacityMult;
	}

	SCR_FillRect(x, y, width, height, c);
}

#define BOX_MARGIN 30

int adjustedScreenWidth = SCREEN_WIDTH;
int margin = 0;


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// closing a full screen console restarts the demo loop
	if ( cls.state == CA_DISCONNECTED && cls.keyCatchers == KEYCATCH_CONSOLE ) {
		CL_StartDemoLoop();
		return;
	}

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	cls.keyCatchers ^= KEYCATCH_CONSOLE;
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	char	buffer[1024];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", Cmd_Argv(1) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		strcat( buffer, "\n" );
		FS_Write(buffer, strlen(buffer), f);
	}

	FS_FCloseFile( f );
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

						

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CON_TEXTSIZE];

	width = (adjustedScreenWidth / SMALLCHAR_WIDTH) - 2;

	if (width == con.linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		Com_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get ("scr_conspeed", "3", CVAR_ARCHIVE);
	cl_chatcolor = Cvar_Get("cl_chatcolor", "7", CVAR_ARCHIVE);

	con_nochat = Cvar_Get("con_nochat", "0", CVAR_ARCHIVE);
	con_coloredKills = Cvar_Get("con_coloredKills", "0", CVAR_ARCHIVE);
	con_coloredHits = Cvar_Get("con_coloredHits", "0", CVAR_ARCHIVE);
	con_prompt = Cvar_Get("con_prompt", "]", CVAR_ARCHIVE);
	con_promptColor = Cvar_Get("con_promptColor", "7", CVAR_ARCHIVE);
	con_timePrompt = Cvar_Get("con_timePrompt", "0", CVAR_ARCHIVE);
	con_timePrompt12 = Cvar_Get("con_timePrompt12", "1", CVAR_ARCHIVE);
	con_drawscrollbar = Cvar_Get("con_drawscrollbar", "0", CVAR_ARCHIVE);
	con_consoleHeight = Cvar_Get("con_consoleHeight", "50", CVAR_ARCHIVE);
	con_fadeIn = Cvar_Get("con_fadeIn", "0", CVAR_ARCHIVE);
	con_margin = Cvar_Get("con_margin", "0", CVAR_ARCHIVE);

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}
	CL_LoadConsoleHistory( );

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand ("messagemode4", Con_MessageMode4_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (qboolean skipnotify)
{
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0)
	{
    if (skipnotify)
		  con.times[con.current % NUM_CON_TIMES] = 0;
    else
		  con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%con.totallines)*con.linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

int nameToTeamColour(char *name) {
	int i, team = 2;
	char *cs;
	for (i = 0; i < MAX_CLIENTS; i++) {
		cs = cl.gameState.stringData + cl.gameState.stringOffsets[544 + i];
		if (!Q_stricmp(Info_ValueForKey(cs, "n"), name)) {
			team = atoi(Info_ValueForKey(cs, "t"));
			if (team == TEAM_RED) {
				team = 1;
			} else if (team == TEAM_BLUE) {
				team = 4;
			} else {
				team = 2;
			}
			break;
		}
	}
	return team;
}

int damageToColor(int damage) {
  int color;

  	if (damage > 51) { 
  		color = 1;
  	} else if (damage <= 51 && damage >= 44) {
  		color = 8;
 	} else if (damage < 44 && damage >= 17) {
  		color = 3;
	} else {
		color = 2;
	}

	return color;
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( char *txt ) {
	int		y;
	int		c, l;
	int		color;
	int 	i;
	int 	team;
	qboolean skipnotify = qfalse;		// NERVE - SMF
	int prev;							// NERVE - SMF
	char player1[MAX_NAME_LENGTH + 1], player2[MAX_NAME_LENGTH + 1];
	char newtxt[MAX_STRING_CHARS + 1];
	char nplayer1[MAX_NAME_LENGTH + 5], nplayer2[MAX_NAME_LENGTH + 5];



	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}
	
	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}
	
	if (!con.initialized) {
		con.color[0] = 
		con.color[1] = 
		con.color[2] =
		con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize ();
		con.initialized = qtrue;
	}

	if (con_nochat && con_nochat->integer == 1) {
		if (strstr(txt, "^3: ^3") || 
			strstr(txt, "): ^3") || 
			strstr(txt, "^7: ^3") || 
			strstr(txt, "^7]: ^3")) {
			suppressNext = qtrue;
			return;
		} else if (suppressNext) {
			suppressNext = qfalse;
			return;
		}
	} else if (con_nochat && con_nochat->integer == 2) {
		if (strstr(txt, "^3: ^3") || 
			strstr(txt, "): ^3") || 
			strstr(txt, "^7: ^3") || 
			strstr(txt, "^7]: ^3") || 
			strstr(txt, "server:") ||
			strstr(txt, "^0(^2b3^0)")) {
			suppressNext = qtrue;
			return;
		} else if (suppressNext) {
			suppressNext = qfalse;
			return;
		}

	}

	if (cls.state == CA_ACTIVE && con_coloredKills && con_coloredKills->integer) {
		char **search;
		int found = 0;
		int killLogNum = Cvar_VariableIntegerValue("cg_drawKillLog");
		if (killLogNum == 1) {
			search = killLog1;
		} else if (killLogNum == 2) {
			search = killLog2;
		} else if (killLogNum == 3) {
			search = killLog3;
		}

		if (killLogNum > 0 && killLogNum < 4) {
			int j;
			char *cs;
			int temp;
			for (i = 0; ; i++) {
				if (!search[i])
					break;

				if (sscanf(txt, search[i], player1, player2) == 2) {
					found = 1;
					if (killLogNum == 1) {
						temp = strlen(player2);
						if (player2[temp - 2] == '\'' && player2[temp - 1] == 's') {
							player2[temp - 2] = 0;
						}
					} else if (killLogNum > 1) {
						temp = strlen(player2);
						player2[temp - 1] = 0;
					}

					team = nameToTeamColour(player1);
					sprintf(nplayer1, "^%i%s^7", team, player1);

					team = nameToTeamColour(player2);
					sprintf(nplayer2, "^%i%s^7", team, player2);
					sprintf(newtxt, search[i], nplayer1, nplayer2);
					txt = newtxt;
					break;
				}
			}

			if (!found) {
				for (i = 0; ; i++) {
					if (!killLogSingle[i]) {
						break;
					}

					if (sscanf(txt, killLogSingle[i], player1, player2) == 2) {
						team = nameToTeamColour(player1);
						sprintf(nplayer1, "^%i%s^7", team, player1);
						sprintf(newtxt, killLogSingle[i], nplayer1, player2);
						txt = newtxt;
						break;
					}
				}
			}

		}
	}

	if (cls.state == CA_ACTIVE && con_coloredHits && con_coloredHits->integer && Cvar_VariableIntegerValue("cg_showbullethits") == 2) {
		char damageString[12];
		int damage, damageCol;
 
		for (i = 0; ; i++) {
			if (!hitLog1[i])
					break;
 
			if (sscanf(txt, hitLog1[i], player1, player2, damageString) == 3) {
				damage = atoi(damageString);
				damageCol = damageToColor(damage);

				if (con_coloredHits->integer == 2) {
					team = nameToTeamColour(player1);
					sprintf(nplayer1, "^%i%s^7", team, player1);

					team = nameToTeamColour(player2);
					sprintf(nplayer2, "^%i%s^7", team, player2);
				} else {
					sprintf(nplayer1, "%s", player1);
					sprintf(nplayer2, "%s", player2);
				}

				sprintf(damageString, "^%i%i%%^7", damageCol, damage);
				sprintf(newtxt, hitLog1[i], nplayer1, nplayer2, damageString);
				txt = newtxt;
				break;
			}
		}

		for (i = 0; ; i++) {
			if (!hitLog2[i])
					break;
 
			if (sscanf(txt, hitLog2[i], player1, player2, damageString) == 3) {
				damage = atoi(damageString);
				damageCol = damageToColor(damage);

				if (con_coloredHits->integer == 2) {
					team = nameToTeamColour(player1);
					sprintf(nplayer1, "^%i%s^7", team, player1);

					team = nameToTeamColour(player2);
					sprintf(nplayer2, "^%i%s^7", team, player2);
				} else {
					sprintf(nplayer1, "%s", player1);
					sprintf(nplayer2, "%s", player2);
				}

				sprintf(damageString, "^%i%i%%^7", damageCol, damage);
				sprintf(newtxt, hitLog2[i], nplayer1, nplayer2, damageString);
				txt = newtxt;
				break;
			}
		}
 
		for (i = 0; ; i++) {
			if (!hitLog3[i])
					break;
 
			if (sscanf(txt, hitLog3[i], player2, damageString) == 2) {
				damage = atoi(damageString);
				damageCol = damageToColor(damage);

				if (con_coloredHits->integer == 2) {
					team = nameToTeamColour(player2);
					sprintf(nplayer2, "^%i%s^7", team, player2);
				} else {
					sprintf(nplayer2, "%s", player2);
				}

				sprintf(damageString, "^%i%i%%^7", damageCol, damage);
				sprintf(newtxt, hitLog3[i], nplayer2, damageString);
				txt = newtxt;
				break;
			}
		}
 
		for (i = 0; ; i++) {
			if (!hitLog4[i])
					break;
 
			if (sscanf(txt, hitLog4[i], player2, damageString) == 2) {
				damage = atoi(damageString);
				damageCol = damageToColor(damage);

				if (con_coloredHits->integer == 2) {
					team = nameToTeamColour(player2);
					sprintf(nplayer2, "^%i%s^7", team, player2);
				} else {
					sprintf(nplayer2, "%s", player2);
				}

				sprintf(damageString, "^%i%i%%^7", damageCol, damage);
				sprintf(newtxt, hitLog4[i], nplayer2, damageString);
				txt = newtxt;
				break;
			}
		}
	}

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *txt) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< con.linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth) ) {
			Con_Linefeed(skipnotify);

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = (color << 8) | c;
			con.x++;
			if (con.x >= con.linewidth) {
				Con_Linefeed(skipnotify);
				con.x = 0;
			}
			break;
		}
	}


	// mark time for transparent overlay
	if (con.current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = con.current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		}
		else
		// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
	int		y;

	if ( cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( SMALLCHAR_HEIGHT * 2 );

	Con_RE_SetColor( con.color );

	if (con_promptColor->integer >= 0 && con_promptColor->integer < 10) {
 		Con_RE_SetColor(g_color_table[con_promptColor->integer]);
 	}

	int promptLen = strlen(con_prompt->string) + 1;
 	int i;
 	char *prompt;
 	qtime_t curTime;
	Com_RealTime(&curTime);
 	
 	if (con_timePrompt->integer && con_timePrompt12->integer == 0) {
		prompt = Z_Malloc(promptLen + 11);
		Com_sprintf(prompt, promptLen + 11, "[%02i:%02i:%02i] %s", curTime.tm_hour, curTime.tm_min, curTime.tm_sec, con_prompt->string);
	} else if (con_timePrompt->integer && con_timePrompt12->integer == 1) {
		prompt = Z_Malloc(promptLen + 15);
		if (curTime.tm_hour > 12) {
		Com_sprintf(prompt, promptLen + 11, "[%02i:%02i:%02i] %s", curTime.tm_hour - 12, curTime.tm_min, curTime.tm_sec, con_prompt->string);
		} else if (curTime.tm_hour == 12) {
		Com_sprintf(prompt, promptLen + 11, "[%02i:%02i:%02i] %s", 12, curTime.tm_min, curTime.tm_sec, con_prompt->string);
		} else if (curTime.tm_hour == 0) {
		Com_sprintf(prompt, promptLen + 11, "[%02i:%02i:%02i] %s", 12, curTime.tm_min, curTime.tm_sec, con_prompt->string);
		} else {
		Com_sprintf(prompt, promptLen + 11, "[%02i:%02i:%02i] %s", curTime.tm_hour, curTime.tm_min, curTime.tm_sec, con_prompt->string);	
		}
	} else {
		prompt = Z_Malloc(promptLen);
		Com_sprintf(prompt, promptLen, "%s", con_prompt->string);
	}

	promptLen = strlen(prompt);
 	for (i = 0; i < promptLen; i++) {
 		SCR_DrawSmallChar( con.xadjust + (i + 1) * SMALLCHAR_WIDTH + ((float)margin * 1.5), y + margin *2, prompt[i]);
 	}

 	Con_RE_SetColor(con.color);

 	if (opacityMult)
	Field_Draw( &g_consoleField, con.xadjust + (promptLen + 1) * SMALLCHAR_WIDTH + ((float)margin * 1.5), y + margin * 2,
		adjustedScreenWidth - 3 * SMALLCHAR_WIDTH, qtrue);
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	short	*text;
	int		i;
	int		time;
	int		skip;
	int		currentColor;
	int 	chatcolor;

	chatcolor = cl_chatcolor->integer;

	currentColor = 7;
	Con_RE_SetColor( g_color_table[currentColor] );

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			continue;
		}

		for (x = 0 ; x < con.linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x]>>8)&7;
				Con_RE_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH, v, text[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	Con_RE_SetColor( NULL );

	if (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
		return;
	}

	// draw the chat line
	if ( cls.keyCatchers & KEYCATCH_MESSAGE )
	{
		if (chat_team)
		{
			SCR_DrawBigStringColor (8, v, "teamchat", g_color_table[chatcolor] );
			SCR_DrawBigStringColor (8, v, "        :", g_color_table[7] );
			skip = 10;
		}
		else
		{
			SCR_DrawBigStringColor (8, v, "chat", g_color_table[chatcolor] );
			SCR_DrawBigStringColor (8, v, "    :", g_color_table[7] );
			skip = 6;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, v,
			adjustedScreenWidth - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue );

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	short			*text;
	int				row;
	int				lines;
//	qhandle_t		conShader;
	int				currentColor;
	vec4_t			colorBG, colorBlack;

	lines = cls.glconfig.vidHeight * frac;
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	if (con_margin && con_margin->integer > 0 && con_margin->integer <= 50)
		lines -= margin - SMALLCHAR_HEIGHT;

	// on wide screens, we will center the text
	con.xadjust = 0;
	SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1 ) {
		y = 0;
	}
	else {

   		colorBlack[0] = 0.0/255.0;
   		colorBlack[1] = 0.0/255.0;
   		colorBlack[2] = 0.0/255.0;
   		colorBlack[3] = 0.9;
 		SCR_AdjustedFillRect(margin, margin, adjustedScreenWidth, y, colorBlack);
 	}
  		colorBG[0] = 0.0/255.0;
 		colorBG[1] = 100.0/255.0;
 		colorBG[2] = 100.0/255.0;
 		colorBG[3] = 1;
 		
 		if (margin) {
			int conPixHeight = 480;
			if (con_consoleHeight->integer >= 0 && con_consoleHeight->integer <= 100) {
				conPixHeight = con_consoleHeight->integer/100.0 * SCREEN_HEIGHT;
		}
		SCR_AdjustedFillRect(margin, margin, adjustedScreenWidth, 1, colorBG);
		SCR_AdjustedFillRect(margin, margin, 1, conPixHeight - 1, colorBG);
 		SCR_AdjustedFillRect(margin + adjustedScreenWidth - 1, margin, 1, conPixHeight - 1, colorBG);
		SCR_AdjustedFillRect(margin, y + margin, adjustedScreenWidth, 1, colorBG);
	} else {
		SCR_AdjustedFillRect(margin, y + margin, adjustedScreenWidth, 2, colorBG);
	}


	// draw the version number

	Con_RE_SetColor(colorBG);

	i = strlen( SVN_VERSION );

	for (x=0 ; x<i ; x++) {

		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH - margin * 2,  

			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)) + margin, SVN_VERSION[x] );

	}


	// draw the text
	con.vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (con.display != con.current)
	{
	// draw arrows to show the buffer is backscrolled
		Con_RE_SetColor(colorBG);
		for (x=0 ; x<con.linewidth ; x+=4)
			SCR_DrawSmallChar( con.xadjust + (x+1)*SMALLCHAR_WIDTH + margin, y + margin * 2, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	if (con_drawscrollbar->integer && con.displayFrac == con.finalFrac) {
		vec4_t scrollbarBG;
		scrollbarBG[0] = scrollbarBG[1] = scrollbarBG[2] = 1;
		scrollbarBG[3] = 0.2;

		int scrollbarBGHeight;
		int visibleHeight;
		int scrollbarPos;
		int totalLines = con.current;
		int visible = rows;

		if (con_consoleHeight->integer >= 0 && con_consoleHeight->integer <= 100) {
			scrollbarBGHeight = ((con_consoleHeight->integer/100.0) * SCREEN_HEIGHT) - 60;
		} else {
			scrollbarBGHeight = 180;
		}

		if (scrollbarBGHeight >= 10) {
			visibleHeight = visible / (float)totalLines * scrollbarBGHeight;
			scrollbarPos = (con.display - rows) / (float)(totalLines - rows) * (scrollbarBGHeight - visibleHeight);
			
			SCR_AdjustedFillRect(618 - margin, margin + 30, 2, scrollbarBGHeight, scrollbarBG);
			scrollbarBG[3] = 0.8;
			SCR_AdjustedFillRect(618 - margin, margin + 30 + scrollbarPos, 2, visibleHeight, scrollbarBG);
		}
	}
	
	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	currentColor = 7;
	Con_RE_SetColor( g_color_table[currentColor] );

	for (i=0 ; i<rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (margin && y < (margin / 8)) {
			break;
		}

		if (row < 0)
			break;
		if (con.current - row >= con.totallines) {
			// past scrollback wrap point
			continue;	
		}

		text = con.text + (row % con.totallines)*con.linewidth;

		for (x=0 ; x<con.linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x] >> 8) % 10;
				Con_RE_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(  con.xadjust + (x+1)*SMALLCHAR_WIDTH + ((float)margin * 1.5), y + margin * 2, text[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();

	Con_RE_SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE ) {
			Con_DrawNotify ();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	adjustedScreenWidth = SCREEN_WIDTH;
	margin = 0;
	if (con_margin && con_margin->integer > 0 && con_margin->integer <= 50) {
		Cvar_Set("con_fadeIn", "1");
		adjustedScreenWidth = SCREEN_WIDTH - con_margin->integer * 2;
		margin = con_margin->integer;
		con.yadjust = margin;
	}

	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		if (con_consoleHeight->integer >= 0 && con_consoleHeight->integer <= 100)
			con.finalFrac = con_consoleHeight->integer / 100.0;
		else 
			con.finalFrac = 0.5;		// half screen

		targetOpacityMult = 1;
	} else {
		con.finalFrac = 0;				// none visible

		targetOpacityMult = 0;
	}

	if (con_fadeIn && con_fadeIn->integer) {
		float moveDist = con_conspeed->value*cls.realFrametime*0.001;
		if (targetOpacityMult < opacityMult) {
			opacityMult -= moveDist;
			if (opacityMult < targetOpacityMult)
				opacityMult = targetOpacityMult;
		} else if (targetOpacityMult > opacityMult) {
			opacityMult += moveDist;
			if (opacityMult > targetOpacityMult)
				opacityMult = targetOpacityMult;
		}

		if (con_consoleHeight->integer >= 0 && con_consoleHeight->integer <= 100)
			con.finalFrac = con_consoleHeight->integer / 100.0;
		else 
			con.finalFrac = 0.5;		// half screen
		con.displayFrac = con.finalFrac;
	}

	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value*cls.realFrametime*0.001;
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value*cls.realFrametime*0.001;
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}
