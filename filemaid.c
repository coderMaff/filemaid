/*
	filemaid

	file moving program	

	Maf v0.0.1	09.12.2002	Created from aaVCR
	Maf	v0.0.2	13.12.2002	Improved layout
							Put time up
							Falls out after 1 file copied so not to thrash cpu
							Timedelay now ini file setting								
		v0.0.3				Stopped it deleting files if target dir buggered
	Maf	v0.0.4	14.12.2002	Added G for Go Now
							Display time top right
							added 1-0 keys to run explorer for file extension dirs
							added F1-F12 keys to run explorer for monitored dirs
	Maf v0.0.5	15.12.2002	Used MoveFile rather than copy and delete ( way faster duh! )
							KillTimer and restart after move incase it takes ages
							Added full error messages
							Added validation of inifile directories
	Maf	v0.0.6	15.12.2002	Made tray program
							Tray tooltip is most recent bit of text
							Left Click Minimizes/Maxmizes
							Buttons + mouse interface
							Custom interface
							Start Hidden ini
	Maf	v0.0.7	16.12.2002	Proper windows style drag bar as mouse move code was chit
	Maf	v0.0.8	16.12.2002	Tidy up prepare for initial release
							Reduce crap vars, global vars and general numptyness
	Maf	v1.0.0	27.12.2002	Release 1
							Dosnt winge when it dosnt do anything
	Maf	v1.0.1	28.02.2002	Restarts timer when file moved sucessfully - duh
	Maf	v1.0.2	29.02.2002	If dest file already exists, it renames it with proceeding num till it works
	Maf	v1.0.3	30.02.2002	E - Edits ini file
							L - List Ini file settings
							R - Restart ( use new ini file settings )
	Maf	v1.0.4	04.01.2003	Put above controls in the help text on 'H'
							Fixed quiting on error to quit nicely and remove tray icon
	Maf v1.0.5	23.07.2003  Fixed bug with F Keys to open monitored directpry
							Enlarged Interface
							New Icon
							Improved interface, dumped windows bar, 
							Fully skinned thanks to article Win32Skinning at flipcode.com by Vander Nunes
	Maf v1.0.6	24.07.2003	Fixed bug in text output
							Added stoponerror
							Increased size of tooltip
	Maf v1.0.7	25.07.2003	Added a log file that gets updated
	Maf	v1.0.8	31.07.2003	v = view log file
							Fixed date into UK format
							On "move failed" error now gives file name
							Added LogFileName configuration option
							l = shows above logfilename and UpdateLogFile
							Added version number to "filemaid started" string
							Added DateFormat=UK option to ini file								
	Maf v1.0.9	01.08.2003	Fixed bug in error for move failed
	Maf v1.0.10 05.08.2003		On error writing to logfile, logging is now disabled
							Dosnt log errors to logfile, just sucessfull moves ( because there can be hudreds of them )
							Killed on depreicated function LMB Drag
	Maf v1.1.0	02.10.2003	Recompiled under Visual Studio .NET 2003
							Added buttons at bottom for extra functionality
							Corrected size of activate button
	Maf V1.2.1	03.01.2004	Fixed opening settings file
							Fixed file already exists bug
							Added ini setting for knightrider bar
	Maf V1.2.2	18.02.2004	Made window appear at 60 down for startbar at top mongs
							Made looking for TRUE/FALSE/UK case insensitive
							Fixed bring window to top on un-tray
	Maf v1.3	19.02.2004	MoveFileWithProgress Forces > win2k O/S
		        22.02.2004  Added Version to help
							Add restart button - does same as pressing R (weddups request)
							Renamed  Read_Registry to ReadRegistry so its tidy
							Add logging level 0=None, 1=File moves only, 2=(default)file moves, stop and start 3=All
							Remove fUpdateLog
							Create ini file if not there on first run
							Add new settings to ini if not there, set to defaults
				24.02.2004	Finish %complete thing in movefilewithprogress

	to do:								
		Play sample on file copy					
		
		Recreate tray icon on end task of explorer.exe
		Options dialogue
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

#define VERSION			"v1.3.0 (dereked)"

#define	WINDOW_WIDTH	600	
#define WINDOW_HEIGHT	500

#define _WIN32_WINNT	0x0500

#define TRAY_ICON (WM_APP+100)

#include <windows.h>
#include <winbase.h>
#include <windowsx.h>
#include <time.h>
#include <shellapi.h>
#include <stdio.h>
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
void				ReadRegistry( HWND );
void 				MoveTick( HWND );
DWORD CALLBACK		MoveCallback(  LARGE_INTEGER,  LARGE_INTEGER,  LARGE_INTEGER,  LARGE_INTEGER,  DWORD,  DWORD,  HANDLE,  HANDLE,  LPVOID );
void				CheckDirectories( HWND );
void				OutputText( LPSTR, HWND, int );
void				TrayIcon( HWND, DWORD, LPSTR  );
void				Activate( BOOL, HWND );
void				Hide( BOOL, HWND );
void				ViewHelp( HWND );
void				ViewLogfile( HWND );
void				EditIni( HWND );
void				ListSettings( HWND );

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
		
		int			iFileType		= 0;
		int			iDirectory		= 0;
		int			iTextLine		= 0;
		int			DelayTime		= 1000;
static	int			LoggingLevel	= 2;
static	BOOL		fHidden			= FALSE;	
static	BOOL		fActive			= FALSE;
static	BOOL		fRestart		= FALSE;
static	BOOL		fStartHidden	= FALSE;
static	BOOL		fStopOnError	= FALSE;
static	BOOL		fUKDateFormat	= FALSE;
static	BOOL		fActiveBar		= TRUE;

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

	hwnd = CreateWindowEx(	0,				// DWORD dwExStyle
							szAppName,		// LPCTSTR lpClassName
							lpCaption,		// LPCTSTR lpWindowName
							WS_POPUP,		// DWORD dwStyle
							0,				// int x
							60,				// int y
							WINDOW_WIDTH,	// int nWidth
							WINDOW_HEIGHT,	// int nHeight
							NULL,			// HWND hWndParent
							NULL,			// HMENU hMenu
							hInstance,		// HINSTANCE hInstance
							NULL );			// LPVOID lpParam

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

	SendMessage(hwnd, WM_PAINT, NULL ,NULL);
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

Name : ReadRegistry

Does : loads and reads filemaid.ini for the settings

------------------------------------------------------------*/

void ReadRegistry( HWND hwnd )
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
		CloseHandle(fh);

		fh = CreateFile(	lpIniFile,
							GENERIC_WRITE,
							0,
							NULL,
							CREATE_NEW,
							FILE_ATTRIBUTE_NORMAL,
							NULL );

		if ( fh == INVALID_HANDLE_VALUE )			// Cant find inifile
		{
		
			CloseHandle(fh);

			wsprintf(m,"Error - Unable to create ini file at %s\nfilemaid will now quit",lpIniFile);
			MessageBox( NULL,m, "Error", MB_OK );
			OnDestroy(hwnd);
			return;

		}
		else
		{
			CloseHandle(fh);
		}

// No ini file so create a default one

		WritePrivateProfileString("information","0","Filemaid configuration file, please read filemaid.txt for help",lpIniFile);
		WritePrivateProfileString("information","1","[indirs] is a list of directories to monitor for new files",lpIniFile);
		WritePrivateProfileString("information","2","This would for example be your eDonkey or Shareaza downloads directory",lpIniFile);
		WritePrivateProfileString("information","3","[types] is a list of file extensions to look for and where to move them",lpIniFile);
		WritePrivateProfileString("information","4","note the | seperates the extension from the path to move them to",lpIniFile);

		WritePrivateProfileString("indirs","0","C:\\Directory_i_wish_to_monitor_for_new_files",lpIniFile);
		WritePrivateProfileString("indirs","1","C:\\Second\\Directory\\i_wish_to_monitor_for_new_files",lpIniFile);

		WritePrivateProfileString("types","0","avi|C:\\Where_i_move_my_avis_to",lpIniFile);
		WritePrivateProfileString("types","1","jpeg|C:\\Where_i_move_my_jpegs_to",lpIniFile);

		WritePrivateProfileString("settings","delaytime","60",lpIniFile);
		WritePrivateProfileString("settings","startactive","FALSE",lpIniFile);
		WritePrivateProfileString("settings","starthidden","FALSE",lpIniFile);
		WritePrivateProfileString("settings","stoponerror","TRUE",lpIniFile);
		WritePrivateProfileString("settings","logginglevel","2",lpIniFile);
		WritePrivateProfileString("settings","logfilename","filemaid_log.html",lpIniFile);
		WritePrivateProfileString("settings","dateformat","UK",lpIniFile);
		WritePrivateProfileString("settings","knightridermode","TRUE",lpIniFile);

		MessageBox( NULL,"A default filemaid.ini has been created\nPlease review this and then start filemaid again", "Filemaid - First run", MB_OK );
		LoggingLevel = 0;
		EditIni( hwnd );
		OnDestroy(hwnd);
		return;

	} 
	else
	{
		CloseHandle(fh);
	}	
	
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
	
// Delay time
	GetPrivateProfileString( "settings","delaytime","NOT FOUND!",lpResult,sizeof(lpResult), lpIniFile );
	
	if( lstrcmp( lpResult, "NOT FOUND!" ) == 0 )
	{
		DelayTime = 60000;
		WritePrivateProfileString("settings","delaytime","60",lpIniFile);
	} else
	{
		DelayTime = ( TextToInt(lpResult,lstrlen(lpResult)) * 1000 );
	}

	wsprintf(m,"delay time %d second(s)",( DelayTime / 1000 ));
	OutputText( m , hwnd,FALSE);	

// Start Active
	GetPrivateProfileString( "settings","startactive","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	

	if( lstrcmp( lpResult, "NOT FOUND!" ) == 0 )
	{
		WritePrivateProfileString("settings","startactive","FALSE",lpIniFile);
	}
	else
	if ( lstrcmpi( lpResult, "TRUE" ) == 0 )
	{
		Activate( TRUE, hwnd );
	}

// Start Hidden
	GetPrivateProfileString( "settings","starthidden","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	

	if( lstrcmp( lpResult, "NOT FOUND!" ) == 0 )
	{
		WritePrivateProfileString("settings","starthidden","FALSE",lpIniFile);
		fStartHidden = FALSE;
	}
	else
	if ( lstrcmpi( lpResult, "TRUE" ) == 0 )
	{
		fHidden = TRUE;
		fStartHidden = TRUE;
		ShowWindow( hwnd, SW_HIDE );
	} 

// Stop on Error
	GetPrivateProfileString( "settings","stoponerror","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	

	if( lstrcmp( lpResult, "NOT FOUND!" ) == 0 )
	{
		WritePrivateProfileString("settings","stoponerror","TRUE",lpIniFile);
		fStopOnError = TRUE;
	}
	else
	if ( lstrcmpi( lpResult, "TRUE" ) == 0 )
	{
		fStopOnError = TRUE;
	} 

// Logging Level
	GetPrivateProfileString( "settings","logginglevel","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	
	if ( lstrcmpi( lpResult, "NOT FOUND!" ) == 0 )
	{
		WritePrivateProfileString("settings","logginglevel","2",lpIniFile);
		LoggingLevel = 2;	// Default
	} 
	else
	{
		LoggingLevel = TextToInt(lpResult,lstrlen(lpResult) );
	}

	// Validate
	if (( LoggingLevel < 0 ) || ( LoggingLevel > 3 ))
		LoggingLevel = 2;

// Log file name
	GetPrivateProfileString( "settings","logfilename","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	
	if ( lstrcmpi( lpResult, "NOT FOUND!" ) == 0 )
	{
		WritePrivateProfileString("settings","logfilename","filemaid.html",lpIniFile);
		lstrcpy(lpLogFileName,"filemaid_log.html");
	} 
	else 
		lstrcpy(lpLogFileName,lpResult);

//Date Format
	GetPrivateProfileString( "settings","dateformat","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	
	if ( lstrcmpi( lpResult, "NOT FOUND!" ) == 0 )
	{
		WritePrivateProfileString("settings","dateformat","UK",lpIniFile);
		fUKDateFormat = TRUE;
	}
	else
	if ( lstrcmpi( lpResult, "UK" ) == 0 )
	{
		fUKDateFormat = TRUE;
	} 

// Knightrider Mode
	GetPrivateProfileString( "settings","knightridermode","NOT FOUND!",lpResult,sizeof(lpResult),lpIniFile );	
	if ( lstrcmpi( lpResult, "NOT FOUND!" ) == 0 )
	{
		WritePrivateProfileString("settings","knightridermode","TRUE",lpIniFile);
		fActiveBar = TRUE;
	}
	if ( lstrcmpi( lpResult, "FALSE" ) == 0 )
	{
		fActiveBar = FALSE;
	}

	SetTimer( hwnd,TIMER_TICK,250,NULL);
	SetTimer( hwnd,TIMER_SECOND,1000,NULL);

	RedrawScreen( hwnd );

	wsprintf(m,"Filemaid %s Started",VERSION);
	OutputText(m,hwnd,(LoggingLevel > 1));

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

Name : MoveCallback

Does : Displays progress while moving a file

-----------------------------------------------------------*/

DWORD CALLBACK MoveCallback(
  LARGE_INTEGER TotalFileSize,
  LARGE_INTEGER TotalBytesTransferred,
  LARGE_INTEGER StreamSize,
  LARGE_INTEGER StreamBytesTransferred,
  DWORD dwStreamNumber,
  DWORD dwCallbackReason,
  HANDLE hSourceFile,
  HANDLE hDestinationFile,
  LPVOID lpData
)

{
			long	k;
			char	m[1024];
			long	tbt,tfs;
	static	long	perc;

	tbt	=	( TotalBytesTransferred.LowPart / 1024 );
	tfs =	( TotalFileSize.LowPart / 1024 );

	k = (( tbt * 100 ) / tfs );

	if((( k % 10 ) == 0 ) && ( k != perc ))
	{
		perc = k;
		wsprintf(m,"complete %ld%%",k );
		OutputText(m,lpData,(LoggingLevel > 2));			
	}
	
	return 0;
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
//		OutputText(m,hwnd,(LoggingLevel > 2));

		for( k = 0; k < iFileType; k++ )
		{

//			wsprintf(m,"..for %s",lpFileType[k] );
//			OutputText(m,hwnd,(LoggingLevel > 2));

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
					wsprintf(m,"...attempting to move %s to %s",FileData.cFileName,lpFileTypeDir[k] );
					OutputText(m ,hwnd,(LoggingLevel > 2));

					if( MoveFileWithProgress(FromName,ToName,MoveCallback,hwnd,MOVEFILE_COPY_ALLOWED ) )
					{
						wsprintf(m,"...moved %s to %s",FileData.cFileName,lpFileTypeDir[k] );
						OutputText(m ,hwnd,(LoggingLevel > 0));
						FindClose(hSearch);
						Activate( TRUE, hwnd );
						return;
					} else
					{						
						LPVOID	lpMsgBuf;
						DWORD	error;

						error = GetLastError();						
												
						if(( error == 183 )	|| // File already exists so add 10 _'s to the front 
						   ( error == 80 ))
						{						// 1 at a time till it manages, or fail

							int idone = 0;							

							for( idone = 0; idone < 1000; idone++ )
							{
								SetLastError(0);
								wsprintf( ToName, "%s\\%03d%s",lpFileTypeDir[k],idone,FileData.cFileName );
								
								if( MoveFileWithProgress(FromName,ToName,MoveCallback,hwnd,MOVEFILE_COPY_ALLOWED ) )
								{
									wsprintf(m,"...moved %s to %s",FileData.cFileName,lpFileTypeDir[k] );
									OutputText(m ,hwnd,(LoggingLevel > 0));
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
							OutputText(m,hwnd,(LoggingLevel > 2) );

							wsprintf(m,"...error(%d) : %s",error,(LPCTSTR)lpMsgBuf);
							OutputText(m,hwnd,(LoggingLevel > 2) );

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

	if(fLog)
	{

		fh = fopen(lpLogFileName,"r");
		if(fh == NULL)						// Dosnt exist yet, do header
		{
			fh = fopen(lpLogFileName,"a+");
			if(fh == NULL)
			{
				wsprintf(m,"Error - unable to open log file for writing, logging disabled",lpIniFile);
				MessageBox( NULL,m, "Error", MB_OK );
				LoggingLevel = 0;
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
				LoggingLevel = 0;
				return;
		}

		fprintf(fh,"<TR><TD>%s %s</TD><TD>%s</TD></TR>\n",tmpbuf2, tmpbuf, lpText);

		fclose(fh);
	}

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
		// BringWindowToTop( hwnd );

		SetWindowPos(	hwnd,
						HWND_TOP,
						0,				// X ignored becuase were using SWP_NOMOVE
						0,				// Y ditto
						WINDOW_WIDTH,
						WINDOW_HEIGHT,
						SWP_SHOWWINDOW | SWP_NOMOVE );
		fHidden = FALSE;
	}			
}


/*-----------------------------------------------------------
Name : Help
Does : Displays Help on screen
------------------------------------------------------------- */
void ViewHelp( HWND hwnd )
{

	char	m[1024];

	wsprintf(m,"Help for Filemaid %s",VERSION);
	OutputText(m, hwnd,FALSE);							
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

	wsprintf(m,"notepad filemaid.ini");
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

	if( (LoggingLevel > 0) )
	{			
		wsprintf(m,"Logging Level : %d to %s",LoggingLevel,lpLogFileName);
		OutputText(m,hwnd,FALSE);
	}
	else
		OutputText("No logfile output",hwnd,FALSE);
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
	OutputText(m,hwnd,(LoggingLevel > 1));

	Activate( FALSE, hwnd );

	KillTimer( hwnd, TIMER_TICK );
	KillTimer( hwnd, TIMER_SECOND );
	
	TrayIcon( hwnd, NIM_DELETE, "" );

	DeleteObject( hLogo );
	DeleteObject( hFont );
	DeleteObject( hTick[0] );
	DeleteObject( hTick[1] );
	DeleteObject( hTick[2] );

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

	if ( fActiveBar )
	{
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

	TextOut(	PaintDC,
				327,
				480,
				"restart",
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
									ReadRegistry( hwnd );
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
// Restart
	else if(( xPos > 322) && ( yPos > 478 ) && ( xPos < 373 ) && (yPos < 497))
	{
		fRestart = TRUE;
		OnDestroy( hwnd );
	}	
	else
		SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION,NULL);
}

// ---------
//  The End
// ---------
