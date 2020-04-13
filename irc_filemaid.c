/*
	filemaid

	file moving program	

	Maf 09.12.2002	v0.0.1	Created from aaVCR
	Maf	13.12.2002	v0.0.2	Improved layout
							Put time up
							Falls out after 1 file copied so not to thrash cpu
							Timedelay now ini file setting								
					v0.0.3	Stopped it deleting files if target dir buggered
	Maf	14.12.2002	v0.0.4	Added G for Go Now
							Display time top right
							added 1-0 keys to run explorer for file extension dirs
							added F1-F12 keys to run explorer for monitored dirs
	Maf 15.12.2002	v0.0.5	Used MoveFile rather than copy and delete ( way faster duh! )
							KillTimer and restart after move incase it takes ages
							Added full error messages
							Added validation of inifile directories
	Maf	15.12.2002	v0.0.6	Made tray program
							Tray tooltip is most recent bit of text
							Left Click Minimizes/Maxmizes
							Buttons + mouse interface
							Custom interface
							Start Hidden ini
	Maf	16.12.2002	v0.0.7	Proper windows style drag bar as mouse move code was chit
	Maf	16.12.2002	v0.0.8	Tidy up prepare for initial release
							Reduce crap vars, global vars and general numptyness
	Maf	27.12.2002	v1.0.0	Release 1
							Dosnt winge when it dosnt do anything
	Maf	28.02.2002	v1.0.1	Restarts timer when file moved sucessfully - duh
	Maf	29.02.2002	v1.0.2	If dest file already exists, it renames it with proceeding num till it works
	Maf	30.02.2002	v1.0.3	E - Edits ini file
							L - List Ini file settings
							R - Restart ( use new ini file settings )
	Maf	04.01.2003	v1.0.4	Put above controls in the help text on 'H'
							Fixed quiting on error to quit nicely and remove tray icon
	Maf 23.07.2003  v1.0.5	Fixed bug with F Keys to open monitored directpry
							Enlarged Interface
							New Icon
							Improved interface, dumped windows bar, 
							Fully skinned thanks to article Win32Skinning at flipcode.com by Vander Nunes
	Maf 24.07.2003	v1.0.6	Fixed bug in text output
							Added stoponerror
							Increased size of tooltip
	Maf 25.07.2003	v1.0.7	Added a log file that gets updated
	Maf	31.07.2003	v1.0.8	v = view log file
							Fixed date into UK format
							On "move failed" error now gives file name
							Added LogFileName configuration option
							l = shows above logfilename and UpdateLogFile
							Added version number to "filemaid started" string
							Added DateFormat=UK option to ini file								
	Maf 01.08.2003	v1.0.9	Fixed bug in error for move failed
	Maf 05.08.2003	v1.0.10	On error writing to logfile, logging is now disabled
							Dosnt log errors to logfile, just sucessfull moves ( because there can be hudreds of them )
							Killed on depreicated function LMB Drag
	Maf 02.10.2003	v1.1.0	Recompiled under Visual Studio .NET 2003
							Added buttons at bottom for extra functionality
							Corrected size of activate button

	Maf 28.10.2003	v1.2.0	Added irc logging


	to do:
		BringWindowToTop() dosnt work	
*/

#define	STRICT
#define WIN32_LEAN_AND_MEAN

#define TIMER_TICK		1
#define TIMER_LAUNCH	2
#define TIMER_MONITOR	3
#define	TIMER_SECOND	4

#define COL_BLACK		0x00000000
#define COL_GREEN		0x0000ff00

#define	TICK_Y			5

#define TEXT_LINES		32

#define VERSION			"v1.2.0 (.netified)"

#define	WINDOW_WIDTH	600	
#define WINDOW_HEIGHT	500

#define TRAY_ICON (WM_APP+100)

#include <windows.h>
#include <windowsx.h>
#include <time.h>
#include <shellapi.h>
#include <stdio.h>
#include <winsock2.h>

#include "resource.h"


// ------------
//  Prototypes
// ------------

BOOL				Register( HINSTANCE );
HWND				Create(	HINSTANCE, int );
LRESULT CALLBACK	WndProc( HWND, UINT, WPARAM, LPARAM );
UINT				TextToInt( char *, int );
void 				RedrawScreen( HWND );
void 				RedrawPart( HWND, int, int, int, int);
void				Read_Registry( HWND );
void 				MoveTick( HWND );
void				CheckDirectories( HWND );
void				OutputText( LPSTR, HWND, int );
void				TrayIcon( HWND, DWORD, LPSTR  );
void				Activate( BOOL, HWND );
void				Hide( BOOL, HWND );
void				ViewHelp( HWND );
void				ViewLogfile( HWND );
void				EditIni( HWND );
void				ListSettings( HWND );

int					irc_HandleIn( HWND );
int					irc_HandleOut( HWND, char *);
int					irc_Join( HWND );

BOOL				OnCreate( HWND,CREATESTRUCT FAR *);
void				OnDestroy( HWND );
void				OnPaint( HWND );
void				OnKeyDown( HWND, UINT, BOOL, int, UINT );
void				OnTimer( HWND, UINT );
void				OnTrayIcon(	HWND, WPARAM, LPARAM );
void				OnLmbClick(	HWND, WPARAM, LPARAM );

// ------------------
//  Global Variables
// ------------------

		char		szAppName[]	= "filemaid";
static	HWND		MainWindow;													// Windowhandle
		char		lpIniFile[80];												// Name and path of ini file		
		char		lpCaption[]	= "filemaid - www.kardiak.co.uk - h for help";	// Window Caption
		HANDLE		hLogo;														// Logo handle
		HANDLE		hTick[3];													// Moving spots handle
		HFONT		hFont;														// Font handle
static	HINSTANCE	hInst;														// App Instance
static 	int			posX = 21;													// position of the tick counter
		BOOL		fDirection = 0;												// direction of travel for tick
		char		lpWindowText[TEXT_LINES + 1][1024];							// Text to display in window
		char		lpTime[128];												// Current time, filed every second
		char		lpFileType[256][MAX_PATH];									// File Types
		char		lpFileTypeDir[256][MAX_PATH];
		char		lpDirectory[256][MAX_PATH];
		char		lpLogFileName[1024];
		
		char		lpircserver[256];
		char		lpircport[256];
		char		lpirchandle[256];		
		char		lpircchannel[256];

		int			iFileType		= 0;
		int			iDirectory		= 0;
		int			iTextLine		= 0;
		int			DelayTime		= 1000;
		BOOL		fHidden			= FALSE;	
static	BOOL		fActive			= FALSE;
static	BOOL		fRestart		= FALSE;
static	BOOL		fStartHidden	= FALSE;
static	BOOL		fStopOnError	= FALSE;
static  BOOL		fUpdateLog		= FALSE;
static	BOOL		fUKDateFormat	= FALSE;
static	BOOL		firclogging		= FALSE;


/* irc globals */

long			lWSResult;
SOCKET			s;
struct			sockaddr_in A;
WSADATA			ws;	

/*-----------------------------------------------------------

Name : WinMain

Does : Program Entry point

------------------------------------------------------------*/

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR  lpCmdLine,
                     int    nCmdShow)
{
	MSG			Msg;
	char		m[50];

	if( lstrlen(lpCmdLine) == 0 )
		lstrcpy(lpIniFile,".\\filemaid.ini");
	else
		wsprintf(lpIniFile,".\\%s",lpCmdLine);

	if( !Register(hInstance))
	{
		wsprintf(m,"Failed to Register Window - Error %ld",GetLastError());
		MessageBox( NULL, m,"Error", MB_OK );

		return FALSE;
	}

	if(!Create(hInstance, nCmdShow))
	{
		MessageBox( NULL, "Failed to Create Window","Error", MB_OK );
		return FALSE;
	}

	while( GetMessage (&Msg, NULL, 0, 0 ))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}



/*-----------------------------------------------------------

Name : Register

Does : Registers Window

------------------------------------------------------------*/
BOOL Register(HINSTANCE hInstance)
{

	WNDCLASS W;

	memset( &W, 0, sizeof(W) );

	W.style			= CS_HREDRAW | CS_VREDRAW |	CS_BYTEALIGNCLIENT;;
	W.lpfnWndProc	= WndProc;
	W.hInstance		= hInstance;
	W.hIcon			= LoadIcon(	hInst, "IDI_ICON");
	W.lpszClassName = szAppName;
	W.hbrBackground	= GetStockBrush( WHITE_BRUSH );
	W.hCursor		= LoadCursor( NULL, IDC_ARROW );

	return ( RegisterClass( &W ) != (ATOM)0 );
}



/*-----------------------------------------------------------

Name : Create

Does : Creates Window

------------------------------------------------------------*/

HWND Create(HINSTANCE hInstance, int nCmdShow)
{
	
	HWND	hwnd;

	hInst = hInstance;

	hwnd = CreateWindowEx(	0,
							szAppName,
							lpCaption,
							WS_POPUP,
							CW_USEDEFAULT,
							0,
							WINDOW_WIDTH,
							WINDOW_HEIGHT,
							NULL,
							NULL,
							hInstance,
							NULL );

	if( hwnd == NULL )
		return hwnd;

	ShowWindow( hwnd, nCmdShow );
	UpdateWindow( hwnd );

	TrayIcon( hwnd, NIM_ADD, "filemaid" );

	return hwnd;	
}


/*-----------------------------------------------------------

Name : WndProc

Does : Main Loop

------------------------------------------------------------*/

LRESULT CALLBACK	WndProc(	HWND hwnd,
								UINT Message,
								WPARAM wParam,
								LPARAM lParam )
{

	switch( Message )
	{
		HANDLE_MSG( hwnd, WM_CREATE, OnCreate );		// Message cracker from <windowsx.h>
		HANDLE_MSG( hwnd, WM_DESTROY, OnDestroy );
		HANDLE_MSG( hwnd, WM_PAINT, OnPaint );
		HANDLE_MSG( hwnd, WM_KEYDOWN, OnKeyDown );
		HANDLE_MSG( hwnd, WM_TIMER, OnTimer );		
		
		case	TRAY_ICON	:	OnTrayIcon(hwnd, wParam, lParam );							
								return DefWindowProc( hwnd, Message, wParam, lParam );
								break;

		case	WM_LBUTTONDOWN	:	OnLmbClick(hwnd, wParam, lParam );							
									return DefWindowProc( hwnd, Message, wParam, lParam );
									break;
			
		default:
			return DefWindowProc( hwnd, Message, wParam, lParam );
	}
}


/*--------------------------------------------------------

Name : TextToInt

Does : Helper Funcntion, converts text to int

------------------------------------------------------------*/

UINT TextToInt( char *Txt, int len )

{
	UINT	mul, total;
	int		i;

	mul		= 1;
	total	= 0;


	for	(i = len - 1; i	>= 0; i--)
	{
		total =	total +	((Txt[i] - '0')	* mul);
		mul	= mul *	10;
	}

	return total;
}



/*-----------------------------------------------------------

Name : RedrawScreen

Does : Forces a screen redraw

------------------------------------------------------------*/

void RedrawScreen( HWND hwnd )
{

	RECT	r;

	r.left = 0;
	r.top = 0;
	r.right = WINDOW_WIDTH;
	r.bottom = WINDOW_HEIGHT;

	InvalidateRect(hwnd,&r,FALSE); // Invalidate whole screen
}

/*-----------------------------------------------------------

Name : RedrawPart

Does : Forces a part screen redraw

------------------------------------------------------------*/

void RedrawPart( HWND hwnd, int x, int y, int r, int b )
{

	RECT	rct;

	rct.left = x;
	rct.top = y;
	rct.right = r;
	rct.bottom = b;

	InvalidateRect(hwnd,&rct,FALSE); // Invalidate whole screen
}


/*-----------------------------------------------------------

Name : Read_Registry

Does : loads and reads filemaid.ini for the settings

------------------------------------------------------------*/

void Read_Registry( HWND hwnd )
{
	char			lpResult[256];
	char			m[1024];
	HANDLE			fh;
	int				w,x,y,z;
	char			c[8],g;
	BOOL			f;

	wsprintf(m,"welcome to filemaid %s", VERSION); // put it on screen
	OutputText(m,hwnd,FALSE);
	OutputText("------------------------------------------------------", hwnd,FALSE);


	KillTimer( hwnd, TIMER_LAUNCH );	// Only wants to happen once

	fh = CreateFile(	lpIniFile,
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL );

	if ( fh == INVALID_HANDLE_VALUE )			// Cant find inifile
	{
		wsprintf(m,"Error - INI file does not exist ( %s ), or INI file not found in expected location\nfilemaid will now exit",lpIniFile);
		MessageBox( NULL,m, "Error", MB_OK );
		OnDestroy(hwnd);
		return;
	}

	CloseHandle(fh);
	
	for( x = 0; x<256; x++ )
	{
		wsprintf(c,"%d",x);		
		GetPrivateProfileString( "indirs",c,"NOT FOUND!",lpDirectory[x],sizeof(lpDirectory[x]),lpIniFile );			
		if( lstrcmp( lpDirectory[x], "NOT FOUND!" ) == 0 ) // blank it jump loop
		{
			lpDirectory[x][0] = 0x00;
			x = 666;
		}
		else
		{
			iDirectory++;

			if( GetFileAttributes(lpDirectory[x]) == 0xFFFFFFFF )
			{
				wsprintf(m,"Error in INI file, %s in section [indirs] does not exist\nfilemaid will now exit",lpDirectory[x]);
				MessageBox( NULL,m, "Error", MB_OK );
				OnDestroy(hwnd);
				return;
			}
		}
	}

	wsprintf(m,"monitoring %d directories",iDirectory);
	OutputText( m, hwnd,FALSE );
	
	for( x = 0; x<256; x++ )
	{
		wsprintf(c,"%d",x);
		GetPrivateProfileString( "types",c,"NOT FOUND!",lpFileType[x],sizeof(lpFileType[x]),lpIniFile );
		if( lstrcmp( lpFileType[x], "NOT FOUND!" ) == 0 ) // blank it jump loop
		{
			lpFileType[x][0] = 0x00;
			x = 666;
		}
		else
		{
			// Split the dir off after the '|'
			f = FALSE;
			w = 0;
			z = strlen( lpFileType[x] );

			for( y = 0; y < z; y++ )
			{
				g = lpFileType[x][y];

				if( g == '|' )
				{
					f = TRUE;
					lpFileType[x][y] = 0x00;
				}
				else
				if( f )
				{
					lpFileTypeDir[x][w] = g;
					lpFileTypeDir[x][w + 1] = 0x00;
					w++;
				}
			}
			iFileType++;

			if( GetFileAttributes(lpFileTypeDir[x]) == 0xFFFFFFFF )
			{
				wsprintf(m,"Error in INI file, %s in section [types] does not exist\nfilemaid will now exit",lpFileTypeDir[x]);
				MessageBox( NULL,m, "Error", MB_OK );
				OnDestroy(hwnd);
				return;
			}

		}
	}
	
	GetPrivateProfileString( "settings","delaytime","NOT FOUND!",lpResult,sizeof(lpResult), lpIniFile );
	
	if( lstrcmp( lpResult, "NOT FOUND!" ) == 0 )
	{
		DelayTime = 1000;	
	} else
	{
		DelayTime = ( TextToInt(lpResult,lstrlen(lpResult)) * 1000 );
	}

	wsprintf(m,"delay time %d second(s)",( DelayTime / 1000 ));
	OutputText( m , hwnd,FALSE);	

	GetPrivateProfileString( "settings","startactive","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	

	if ( lstrcmp( lpResult, "TRUE" ) == 0 )
	{
		Activate( TRUE, hwnd );
	}

	GetPrivateProfileString( "settings","starthidden","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	

	if ( lstrcmp( lpResult, "TRUE" ) == 0 )
	{
		fHidden = TRUE;
		fStartHidden = TRUE;
		ShowWindow( hwnd, SW_HIDE );
	} 

	GetPrivateProfileString( "settings","stoponerror","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	

	if ( lstrcmp( lpResult, "TRUE" ) == 0 )
	{
		fStopOnError = TRUE;
	} 


	GetPrivateProfileString( "settings","updatelog","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	
	if ( lstrcmp( lpResult, "TRUE" ) == 0 )
	{

		fUpdateLog = TRUE;
	} 

	GetPrivateProfileString( "settings","logfilename","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	

	if ( lstrcmp( lpResult, "NOT FOUND!" ) == 0 )
	{
		if( fUpdateLog )
		{
			wsprintf(m,"Error in INI file\n updatelog set to True but no logfilename specified, output to logfile disabled");
			MessageBox( NULL,m, "Error", MB_OK );
			fUpdateLog = FALSE;			
		}
	} 
	else 
		lstrcpy(lpLogFileName,lpResult);

	GetPrivateProfileString( "settings","dateformat","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	
	if ( lstrcmp( lpResult, "UK" ) == 0 )
	{
		fUKDateFormat = TRUE;
	} 

	GetPrivateProfileString( "settings","irclogging","NOT FOUND!",lpResult,sizeof(lpResult), lpIniFile );
	if( lstrcmp( lpResult,"TRUE") == 0 ) // Yup they want irc logging
	{
		firclogging = TRUE;

		GetPrivateProfileString( "settings","ircchannel","NOT FOUND!",lpircchannel,sizeof(lpResult), lpIniFile );
		if (lstrcmp( lpircchannel,"NOT FOUND!") == 0 ) firclogging = FALSE;

		GetPrivateProfileString( "settings","ircserver","NOT FOUND!",lpircserver,sizeof(lpResult), lpIniFile );
		if (lstrcmp( lpircserver,"NOT FOUND!") == 0 ) firclogging = FALSE;

		GetPrivateProfileString( "settings","ircport","NOT FOUND!",lpircport,sizeof(lpResult), lpIniFile );
		if (lstrcmp( lpircport,"NOT FOUND!") == 0 ) firclogging = FALSE;

		GetPrivateProfileString( "settings","irchandle","NOT FOUND!",lpirchandle,sizeof(lpResult), lpIniFile );
		if (lstrcmp( lpirchandle,"NOT FOUND!") == 0 ) firclogging = FALSE;
	}


	SetTimer( hwnd,TIMER_TICK,250,NULL);
	SetTimer( hwnd,TIMER_SECOND,1000,NULL);

	RedrawScreen( hwnd );

	wsprintf(m,"Filemaid %s Started",VERSION);
	OutputText(m,hwnd,TRUE);

}

/*-----------------------------------------------------------

Name : MoveTick

Does : moves little update bar 

------------------------------------------------------------*/

void MoveTick( HWND hwnd )
{
	if ( fDirection == 0 )
	{
		posX = posX + 12;
		if ( posX > 404 )
			fDirection = 1;
	} else
	{
		posX = posX - 12;
		if ( posX < 22 )
			fDirection = 0;
	}

	RedrawPart( hwnd, 0, 0, 450, 20 );
}

/*-----------------------------------------------------------

Name : CheckDirectories

Does : Heart of the program finds checks moves files

------------------------------------------------------------*/

void CheckDirectories( HWND hwnd )
{
	int				j,k;
	WIN32_FIND_DATA FileData;
    HANDLE          hSearch;
    BOOL            fFinished = FALSE;
    char            Pattern[256], FromName[MAX_PATH], ToName[MAX_PATH];
    DWORD           dwAttrs;
    BOOL			fIsDir;
	char			m[1024];

	Activate(FALSE,hwnd);

	for( j = 0; j < iDirectory; j++ )
	{

//		wsprintf(m,"analizing %s",lpDirectory[j] );
//		OutputText(m,hwnd);

		for( k = 0; k < iFileType; k++ )
		{

//			wsprintf(m,"..for %s",lpFileType[k] );
//			OutputText(m,hwnd);

			lstrcpy( Pattern, lpDirectory[j] );
			lstrcat( Pattern, "\\*." );
			lstrcat( Pattern, lpFileType[k] );

			hSearch = FindFirstFile( Pattern, &FileData);

			fFinished = (hSearch == INVALID_HANDLE_VALUE);

			while (!fFinished)
			{
				lstrcpy( FromName, lpDirectory[j] );
				lstrcat( FromName, "\\" );
				lstrcat( FromName, FileData.cFileName );

				lstrcpy( ToName, lpFileTypeDir[k] ); 
				lstrcat( ToName, "\\");
				lstrcat( ToName, FileData.cFileName );

				dwAttrs = GetFileAttributes(FromName);

				fIsDir = ((dwAttrs & FILE_ATTRIBUTE_DIRECTORY) > 0);

				if(fIsDir == FALSE )				   
				{
					if( MoveFile(FromName,ToName ) )
					{
						wsprintf(m,"...moved %s to %s",FileData.cFileName,lpFileTypeDir[k] );
						OutputText(m ,hwnd,TRUE);
						FindClose(hSearch);
						Activate( TRUE, hwnd );
						return;
					} else
					{						
						LPVOID	lpMsgBuf;
						DWORD	error;

						error = GetLastError();						
												
						if( error == 183 )	// File already exists so add 10 _'s to the front 
						{					// 1 at a time till it manages, or fail
							int idone = 0;							

							for( idone = 0; idone < 1000; idone++ )
							{
								SetLastError(0);
								wsprintf( ToName, "%s\\%03d%s",lpFileTypeDir[k],idone,FileData.cFileName );
								
								if( MoveFile( FromName, ToName ) )
								{
									wsprintf(m,"...moved %s to %s",FileData.cFileName,lpFileTypeDir[k] );
									OutputText(m ,hwnd,TRUE);
									FindClose(hSearch);
									Activate( TRUE, hwnd );
									return;
								}
								else 							
									error = GetLastError();	
														
							}
						}
						
						if( error != 0 ) // Nope after 1000 tries its still fecked
						{
							FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
											FORMAT_MESSAGE_FROM_SYSTEM | 
											FORMAT_MESSAGE_IGNORE_INSERTS,
											NULL,
											error,
											MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
											(LPTSTR) &lpMsgBuf,
											0,
											NULL );

							wsprintf(m,"...move %s failed",FileData.cFileName);
							OutputText(m,hwnd,FALSE );

							wsprintf(m,"...error(%d) : %s",error,(LPCTSTR)lpMsgBuf);
							OutputText(m,hwnd,FALSE );

							LocalFree( lpMsgBuf );
							
							FindClose(hSearch);
							if( !fStopOnError )
								Activate( TRUE, hwnd );
							return;
						}
					}
				}

				if (!FindNextFile(hSearch, &FileData)) fFinished = TRUE;
			}

			FindClose(hSearch);
		}
	}

	Activate( TRUE, hwnd );
}

/*-----------------------------------------------------------

Name : Output Text

Does : Puts text to window, scrolls when it hits the bottom

------------------------------------------------------------*/

void OutputText( LPSTR lpText, HWND hwnd, int fLog)
{
	int		j;
	char	m[1024];
	char	o[512];
	char	n[256];
	char	tmpbuf[128];
	char	tmpbuf2[128];
	char	tmp[128];
	FILE	*fh;

	_strtime( tmpbuf );
	_strdate( tmpbuf2 );

// Fix date from crappy US format
	if( fUKDateFormat )
	{
		lstrcpy(tmp,tmpbuf2);
		tmpbuf2[0] = tmp[3];
		tmpbuf2[1] = tmp[4];
		tmpbuf2[3] = tmp[0];
		tmpbuf2[4] = tmp[1];
	};

	lstrcpyn(m,lpText,255);
	wsprintf(n,"fm\n%s",m);
	TrayIcon( hwnd, NIM_MODIFY, n );

	lstrcpyn(o,lpText,512);
	wsprintf( m,"%s %s", tmpbuf, o );

	if( fUpdateLog && fLog)
	{

		fh = fopen(lpLogFileName,"r");
		if(fh == NULL)						// Dosnt exist yet, do header
		{
			fh = fopen(lpLogFileName,"a+");
			if(fh == NULL)
			{
				wsprintf(m,"Error - unable to open log file for writing, logging disabled",lpIniFile);
				MessageBox( NULL,m, "Error", MB_OK );
				fUpdateLog = FALSE;
				return;
			}
			
			fprintf(fh,"<HTML>\n<BODY bgcolor=#9CA2AD link=#EEFFFF vlink=#EEFFFF alink=#EEFFFF>\n");
			fprintf(fh,"<font face=\"Arial, Helvetica, sans-serif\" size=-1>");
			fprintf(fh,"<TABLE border=1>\n");
			fprintf(fh,"<TR><TH><A href=\"http://www.kardiak.co.uk\">filemaid</A> logfile</TH></TR>\n");
		}
		else
			fh = fopen(lpLogFileName,"a+");

		if(fh == NULL)
		{
				wsprintf(m,"Error - unable to open log file for writing, logging disabled",lpIniFile);
				MessageBox( NULL,m, "Error", MB_OK );
				fUpdateLog = FALSE;
				return;
		}

		fprintf(fh,"<TR><TD>%s %s</TD><TD>%s</TD></TR>\n",tmpbuf2, tmpbuf, lpText);

		fclose(fh);
	}

	irc_HandleOut(hwnd,m);

	if( iTextLine < TEXT_LINES )
	{
		lstrcpy( lpWindowText[iTextLine],m );
		iTextLine++;
	}
	else
	{
		lstrcpy( lpWindowText[TEXT_LINES],m );

		for( j = 0; j < TEXT_LINES; j++ )
		{
			lstrcpy( lpWindowText[j], lpWindowText[j + 1] );
		}
		
	}

	RedrawScreen(hwnd);
}


/*-----------------------------------------------------------

Name : TrayIcon

Does : handle the tray icon

------------------------------------------------------------*/

void TrayIcon( HWND hwnd,  DWORD dothis, LPSTR lpText )
{
	NOTIFYICONDATA tnd; 

	tnd.cbSize				= sizeof(NOTIFYICONDATA);     
	tnd.hWnd				= hwnd;     
	tnd.uID					= 1579385693;     
	tnd.uFlags				= NIF_MESSAGE|NIF_ICON|NIF_TIP;     
	tnd.hIcon				= LoadIcon(	hInst, "IDI_ICON");
	tnd.uCallbackMessage	= TRAY_ICON;
	lstrcpyn(tnd.szTip, lpText, sizeof(tnd.szTip)); 	

	Shell_NotifyIcon( dothis, &tnd );
}

/*-----------------------------------------------------------

Name : Activate

Does : Activate/Deactivates program

------------------------------------------------------------*/
void Activate( BOOL fActivate, HWND hwnd )
{
	if ( fActivate == FALSE )
	{
		KillTimer( hwnd, TIMER_MONITOR );		
		fActive = FALSE;
		RedrawPart( hwnd, 462, 3, 513, 20 );
	}
	else
	{
		SetTimer( hwnd,TIMER_MONITOR,DelayTime,NULL);			// Timer Tick 1/2 sec interval
		fActive = TRUE;
		RedrawPart( hwnd, 475, 3, 513, 20 );
	}		
}

void Hide( BOOL fHide, HWND hwnd )
{
	if( fHide == TRUE )
	{
		ShowWindow( hwnd, SW_HIDE );
		fHidden = TRUE;
	}
	else
	{
		ShowWindow( hwnd, SW_SHOWNORMAL );				
		SetActiveWindow( hwnd );
		BringWindowToTop( hwnd );
		fHidden = FALSE;
	}			
}


/*-----------------------------------------------------------
Name : Help
Does : Displays Help on screen
------------------------------------------------------------- */
void ViewHelp( HWND hwnd )
{

	OutputText("Help", hwnd,FALSE);							
	OutputText("Keyboard controls:", hwnd,FALSE);				
	OutputText(" a - activate/de-activate", hwnd,FALSE);		
	OutputText(" b - reset window position ( if you loose it )",hwnd,FALSE);
	OutputText(" e - edit ini file", hwnd,FALSE);			
	OutputText(" g - go now", hwnd,FALSE);					
	OutputText(" h - help", hwnd,FALSE);						
	OutputText(" k - kardiak website ;o)", hwnd,FALSE);				
	OutputText(" l - list ini file settings", hwnd,FALSE);							
	OutputText(" q - Quit", hwnd,FALSE);							
	OutputText(" r - restart", hwnd,FALSE);							
	OutputText(" v - view HTML logfile", hwnd, FALSE );
	OutputText(" 0..9 - open file extension dir", hwnd,FALSE);			
	OutputText(" F1..F12 - open monitored", hwnd,FALSE);					
}


/*-----------------------------------------------------------
Name : View the logfile
Does : Views the logfile
------------------------------------------------------------- */
void ViewLogfile( HWND hwnd )
{	
	char	m[1204];

	wsprintf(m,"explorer %s",lpLogFileName);
	WinExec(m,SW_SHOW);	
}				

/*-----------------------------------------------------------
Name : EditIni
Does : Edits The Ini file
------------------------------------------------------------- */
void EditIni( HWND hwnd )
{	
	char	m[1204];

	wsprintf(m,"explorer filemaid.ini");
	OutputText(m, hwnd,FALSE );
	WinExec( m, SW_SHOW );
	OutputText("NB - Restart to pick up changes (r)", hwnd,FALSE);
}

/*-----------------------------------------------------------
Name : List 
Does : Displays list on screen
------------------------------------------------------------- */
void ListSettings( HWND hwnd )
{
	int		j;
	char	m[1204];

	OutputText("Current Settings:",hwnd,FALSE);
	for(j = 0; j < iFileType; j++ )
	{
		wsprintf(m," %d) %s files move to %s",j,lpFileType[j],lpFileTypeDir[j]);
		OutputText(m,hwnd,FALSE);
	}

	OutputText("Monitored directories :",hwnd,FALSE);
	for(j = 0; j < iDirectory; j++ )
	{
		wsprintf(m,"F%d) %s ",j + 1,lpDirectory[j]);
		OutputText(m,hwnd,FALSE);
	}

	wsprintf(m,"Delay Time : %d Second(s)",( DelayTime / 1000 ));
	OutputText(m,hwnd,FALSE);

	if( fStartHidden )
		OutputText("Start Hidden : TRUE",hwnd,FALSE);
	else
		OutputText("Start Hidden : FALSE",hwnd,FALSE);

	if( fActive )
		OutputText("Start Active : TRUE",hwnd,FALSE);
	else
		OutputText("Start Active : FALSE",hwnd,FALSE);

	if( fUpdateLog )
	{			
		wsprintf(m,"Update Log : TRUE to %s",lpLogFileName);
		OutputText(m,hwnd,FALSE);
	}
	else
		OutputText("UpdateLog : FALSE",hwnd,FALSE);
}
/*-----------------------------------------------------------
Name : Join_irc
Does : joins irc and flirts with geeks
-------------------------------------------------------------*/

int irc_Join(HWND hwnd)
{
	// Add some error chicken
	int		d;
	char	m[1024];
	// open a socket and connect to  server

	OutputText("Attempting to connect to irc server",hwnd,FALSE);

	lWSResult = WSAStartup(0x0101,&ws);	
	wsprintf(m,"WSAStartup ..%ld\n",lWSResult);
	OutputText(m,hwnd,FALSE);
	
	s = socket(AF_INET,SOCK_STREAM,0);	

	A.sin_family = AF_INET;
	A.sin_port = htons(6668);
	A.sin_addr.s_addr = inet_addr("213.131.131.155"); 	

	d = connect(s,(struct sockaddr *)&A,sizeof(A));

	wsprintf(m,"in connect d = %ld",d);
	OutputText(m,hwnd,FALSE);

	if ( d != 0 )
	{
		OutputText("IRC connect failed, IRC loggin disabled",hwnd,FALSE);
		firclogging = 0;
		return 1;
	}

	// IRC handshaking bit goes here
	// Stick it in handshaking mode and set a timer
	// read the data back and respond to what it says
	// dump all this do it blind stuff

	irc_HandleIn(hwnd);

	send(s,"\n",strlen("\n"),0);

	irc_HandleIn(hwnd);

	send(s,"\n",strlen("\n"),0);

	irc_HandleIn(hwnd);

	send(s,"\n",strlen("\n"),0);

	irc_HandleIn(hwnd);

	wsprintf(m,"nick %s\n\r",lpirchandle);
	send(s,m,lstrlen(m),0);
	OutputText(m,hwnd,FALSE);

	irc_HandleIn(hwnd);

	wsprintf(m,"USER %s nun nun :%s\n\r",lpirchandle,lpirchandle);
	send(s,m,lstrlen(m),0);
	OutputText(m,hwnd,FALSE);

	irc_HandleIn(hwnd);

	wsprintf(m,"JOIN %s\n\r",lpircchannel);
	send(s,m,lstrlen(m),0);
	OutputText(m,hwnd,FALSE);

	irc_HandleIn(hwnd);

	wsprintf(m,":%s PRIVMSG %s :Filemaid reporting in. (c)2003 Kardiak.co.uk\n\r",lpirchandle,lpircchannel);
	send(s,m,lstrlen(m),0);
	OutputText(m,hwnd,FALSE);

	irc_HandleIn(hwnd);

	// End of IRC connect, all went well
	return 0;
}

/* ======================================================================= *\

	HandleIn()

	this handles incomming data from the IRC stream, gets stuck on
	recv = - if there is no data, fix it!
	returns int

\* ======================================================================= */


int irc_HandleIn(HWND hwnd)
{
	int		iResult = 0;
	char	in[8192];
	char	m[1024];

	do 
	{
		iResult = recv(s,in,sizeof(in),0);
		in[iResult] = 0x00; /// NULL terminate the string

		if ( iResult > 0 )
		{			
			if ( in[0]=='P' && in[1]=='I' &&  in[2]=='N' && in[3]=='G' &&  in[4]==' ' && in[5]==':' )
			{			
				sprintf(m,"pong %s\n",&in[6]);
				send(s,m,strlen(m),0);				
			}
		}

		OutputText(in,hwnd,FALSE);
	} while( iResult == 8192 );
	return 0;
}

/* ======================================================================= *\

	HandleOut()

	sends text to the irc stream
	returns int

\* ======================================================================= */

int irc_HandleOut(HWND hwnd,char *textin)
{
	char	sendtext[255];

	wsprintf(sendtext,":%s PRIVMSG %s : %s",lpirchandle,lpircchannel,textin);
	sendtext[23] = 0x03; // Colour code char ^ in there before the 12
	send(s,sendtext,strlen(sendtext),0);
// HandleIn(); commented out cause it keeps getting stuck
	return 0;
}
/************************************************************************************************/
/*								On's Message response functions									*/
/************************************************************************************************/

/*-----------------------------------------------------------
Name : OnDestroy
Does : quits nicely, like a kitten
------------------------------------------------------------*/

void OnDestroy( HWND hwnd)
{
	char	m[80];

	wsprintf(m,"Filemaid %s Stopped",VERSION);
	OutputText(m,hwnd,TRUE);

	Activate( FALSE, hwnd );

	KillTimer( hwnd, TIMER_TICK );
	KillTimer( hwnd, TIMER_SECOND );
	
	TrayIcon( hwnd, NIM_DELETE, "" );

	DeleteObject( hLogo );
	DeleteObject( hFont );
	DeleteObject( hTick[0] );
	DeleteObject( hTick[1] );
	DeleteObject( hTick[2] );

	WSACleanup();

	PostQuitMessage( 0 );
	
	if( fRestart )
	{
		wsprintf(m,"%s","filemaid.exe");
		WinExec( m, SW_SHOW );
	}
}


/*-----------------------------------------------------------

Name : OnCreate

Does : Loads bitmaps and creates shit

------------------------------------------------------------*/
BOOL OnCreate(	HWND				hwnd,
				CREATESTRUCT FAR*	lpCreateStruct )
{

	char	m[50];

	// GFX Inititalisation

	hLogo = LoadImage(	hInst,
						MAKEINTRESOURCE(102),
						IMAGE_BITMAP,
						WINDOW_WIDTH,
						WINDOW_HEIGHT,
						LR_DEFAULTCOLOR );

	if ( !hLogo )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hTick[0] = LoadImage( 	hInst,
							MAKEINTRESOURCE(IDB_TICK0),
							IMAGE_BITMAP,
							12,
							13,
							LR_DEFAULTCOLOR );
	if ( !hTick[0] )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hTick[1] = LoadImage( 	hInst,
							MAKEINTRESOURCE(IDB_TICK1),
							IMAGE_BITMAP,
							12,
							13,
							LR_DEFAULTCOLOR );
	if ( !hTick[1] )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hTick[2] = LoadImage( 	hInst,
							MAKEINTRESOURCE(IDB_TICK2),
							IMAGE_BITMAP,
							12,
							13,
							LR_DEFAULTCOLOR );
	if ( !hTick[2] )
	{
		wsprintf(m,"Error Loading Bitmap %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	hFont = CreateFont(	12,					// logical height of font
						6,					// logical average character width
						0,					// angle of escapement
						0,					// base-line orientation angle
						FW_REGULAR,			// font weight
						FALSE,				// italic attribute flag
						FALSE,				// underline attribute flag
						FALSE,				// strikeout attribute flag
						ANSI_CHARSET,		// character set identifier
						OUT_DEFAULT_PRECIS,	// output precision
						CLIP_MASK,			// clipping precision
						PROOF_QUALITY,		// output quality
						FF_SWISS,			// pitch and family
						NULL );				// pointer to typeface name string

	if ( !hFont )
	{
		wsprintf(m,"Error Loading Font %d - Instance = %lx",GetLastError(),hInst);
		MessageBox( hwnd, m,"Error",MB_OK);
		return FALSE;
	}

	
	SetTimer( hwnd,TIMER_LAUNCH,250,NULL);			// Timer Tick 1/2 sec interval	
	
	return TRUE;
}


/*-----------------------------------------------------------

Name : OnPaint

Does : Draws screen

------------------------------------------------------------*/

void OnPaint( HWND hwnd )
{
	int				x;
	HDC				PaintDC,BltDC,TickDC;
	PAINTSTRUCT		PaintStruct;

	PaintDC		= BeginPaint( hwnd, &PaintStruct );
	BltDC		= CreateCompatibleDC( PaintDC );
	TickDC		= CreateCompatibleDC( PaintDC );

	SelectObject( BltDC, hLogo );
	BitBlt( PaintDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, BltDC, 0, 0, SRCCOPY );

	SelectObject( TickDC, hTick[0] );
	BitBlt( PaintDC, posX, TICK_Y, 12, 13, TickDC, 0, 0, SRCCOPY );

	if ( fDirection == 0 )	// Traveling right
	{
		if( posX > 21 )
		{
			SelectObject( TickDC, hTick[1] );
			BitBlt( PaintDC, posX - 12, TICK_Y, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
		if( posX > 33 )
		{
			SelectObject( TickDC, hTick[2] );
			BitBlt( PaintDC, posX - 24, TICK_Y, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
	}
	else					// Traveling left
	{
		if( posX < 402 )
		{
			SelectObject( TickDC, hTick[1] );
			BitBlt( PaintDC, posX + 12, TICK_Y, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
		if( posX < 392 )
		{
			SelectObject( TickDC, hTick[2] );
			BitBlt( PaintDC, posX + 24, TICK_Y, 12, 13, TickDC, 0, 0, SRCCOPY );
		}
	}


	SelectObject( PaintDC, hFont );

	SetBkMode(PaintDC, TRANSPARENT); 
	SetTextColor(PaintDC,COL_BLACK);

	for( x = 0; x < TEXT_LINES; x++ )	
	{
		TextOut( PaintDC, 
				 10, 
				 24 + ( x * 14 ), 
				 lpWindowText[x], 
				 lstrlen( lpWindowText[x] ));
	}

	SetTextColor(PaintDC,COL_GREEN);

	for( x = 0; x < TEXT_LINES; x++ )	
	{
		TextOut( PaintDC, 
				 11, 
				 26 + ( x * 14 ), 
				 lpWindowText[x], 
				 lstrlen( lpWindowText[x] ));
	}


	if ( fActive == TRUE )	
		TextOut(	PaintDC,
					475,
					7,
					"active",
					6);
	else
		TextOut(	PaintDC,
					475,
					7,
					"idle",
					4);

	TextOut(	PaintDC,
				520,
				7,
				lpTime,
				lstrlen( lpTime ));

// Bottom Buttons 
	TextOut(	PaintDC,
				11,
				480,
				"go now",
				6 );

	TextOut(	PaintDC,
				64,
				480,
				"help",
				4 );

	TextOut(	PaintDC,
				116,
				480,
				"list",
				4 );

	TextOut(	PaintDC,
				169,
				480,
				"log",
				3 );

	TextOut(	PaintDC,
				223,
				480,
				"settings",
				8 );

	TextOut(	PaintDC,
				275,
				480,
				"kardiak",
				7 );

	DeleteDC( TickDC );
	DeleteDC( BltDC );
	DeleteDC( PaintDC );

	EndPaint( hwnd, &PaintStruct );

}


/*-----------------------------------------------------------

Name : OnKeyDown

Does : Handles keystroke events

------------------------------------------------------------*/

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	char	m[1204];
//	wsprintf(m,"Keystroke %d",vk);
//	OutputText(m, hwnd);

//1-10
	if(( vk >= 48 ) && ( vk <= 57 ))						
	{		
		wsprintf(m,"explorer %s",lpFileTypeDir[vk - 48]);
		OutputText(m, hwnd,FALSE);
		WinExec( m, SW_SHOW );
	}
	else
//F1-F12
	if(( vk >= 112 ) && ( vk <= 123 ))
	{		
		wsprintf(m,"explorer %s",lpDirectory[vk - 112]);
		OutputText(m, hwnd,FALSE);
		WinExec( m, SW_SHOW );
	}
	else
//a - activate
	if ( vk == 65 ) Activate( !fActive, hwnd );
	else
//b - reset window position
	if( vk == 66 ) MoveWindow( hwnd, 5, 5, WINDOW_WIDTH, WINDOW_HEIGHT, TRUE );
	else
//e
	if( vk == 69 ) EditIni( hwnd );
	else
//g - go now
	if ( vk == 71 )	CheckDirectories( hwnd );
	else
//h - help
	if ( vk == 72 ) ViewHelp( hwnd );
	else
//k - kardiak website
	if ( vk == 75 )	WinExec("explorer http://www.kardiak.co.uk",SW_SHOW);	
	else
// l - list
	if( vk == 76 )	ListSettings( hwnd );
//q and esc
	if (( vk == 81 ) || ( vk == 27 )) OnDestroy( hwnd );
	else	
//r restart
	if ( vk == 82 )
	{
		fRestart = TRUE;
		OnDestroy( hwnd );
	}
//v view logfile
	else
	if ( vk == 86 )	ViewLogfile( hwnd );
}

/*-----------------------------------------------------------
Name : OnTimer
Does : Handles timer events
------------------------------------------------------------*/

void OnTimer( HWND hwnd, UINT id )
{
	switch( id )
	{
		case TIMER_LAUNCH		:	KillTimer( hwnd, TIMER_LAUNCH );
									Read_Registry( hwnd );
									if( firclogging )
										irc_Join( hwnd );
									break;

		case TIMER_TICK			:	if( fActive ) MoveTick( hwnd );
									break;

		case TIMER_SECOND		:	_strtime( lpTime );
									RedrawPart( hwnd, 520, 7, 600, 17 ); // for time
									break;

		case TIMER_MONITOR		:	CheckDirectories( hwnd );
									break;								
	}

}

/*-----------------------------------------------------------

Name : OnTrayIcon

Does : Handles tray events

------------------------------------------------------------*/

void OnTrayIcon( HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	
	UINT			uMouseMsg;
	WINDOWPLACEMENT lpwndpl;	
	
	uMouseMsg = (UINT)lParam;

	if( uMouseMsg == WM_LBUTTONDOWN )
	{
		lpwndpl.length = sizeof(WINDOWPLACEMENT);
		if( GetWindowPlacement( hwnd, &lpwndpl ) )
		{
			Hide(!fHidden,hwnd);
		}		
	}
}

/*-----------------------------------------------------------

Name : OnLmbClick

Does : Handles click of left mouse button

------------------------------------------------------------*/

void OnLmbClick( HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	int	xPos,yPos;

	xPos = LOWORD(lParam);  // horizontal position of cursor 
	yPos = HIWORD(lParam);  // vertical position of cursor 

// Top Bar y > 3 && y < 20

// Active/Deactivate
		 if(( xPos > 461) && ( yPos > 3 ) && ( xPos < 513 ) && ( yPos < 20 ))  Activate( !fActive, hwnd );
// Minimize
	else if(( xPos > 569) && ( yPos > 3 ) && ( xPos < 582 ) && ( yPos < 20 ))  Hide( TRUE, hwnd );		
// Quit
	else if(( xPos > 583) && ( yPos > 3 ) && ( xPos < 600 ) && ( yPos < 20 ))  OnDestroy( hwnd );		

// Bottom Bar y > 478 && y < 497

// Go Now
	else if(( xPos >   6) && ( yPos > 478 ) && ( xPos <  56 ) && (yPos < 497))	CheckDirectories( hwnd );	
// Help
	else if(( xPos >  59) && ( yPos > 478 ) && ( xPos < 109 ) && (yPos < 497))	ViewHelp( hwnd ); 
// List
	else if(( xPos > 111) && ( yPos > 478 ) && ( xPos < 161 ) && (yPos < 497))	ListSettings( hwnd ); 
// View Log
	else if(( xPos > 164) && ( yPos > 478 ) && ( xPos < 214 ) && (yPos < 497))	ViewLogfile( hwnd );	
// Edit Ini
	else if(( xPos > 218) && ( yPos > 478 ) && ( xPos < 268 ) && (yPos < 497))	EditIni( hwnd ); 
// Visit Kardiak
	else if(( xPos > 270) && ( yPos > 478 ) && ( xPos < 321 ) && (yPos < 497))  WinExec("explorer http://www.kardiak.co.uk",SW_SHOW);	 
	
	else
		SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION,NULL);
}

// ---------
//  The End
// ---------
