#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/app.h>
#include <wx/bitmap.h>
#include <wx/timer.h>
#include <wx/dynlib.h>
#include <wx/fileconf.h>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include "wx/busyinfo.h"
#include "wx/socket.h"

using namespace std;

#include "wx/wxprec.h"

// for all others, include the necessary headers
#ifndef WX_PRECOMP
#  include "wx/wx.h"
#endif

// the application icon
#ifndef wxHAS_IMAGES_IN_RESOURCES
#include "../sample.xpm"
#endif

// --------------------------------------------------------------------------
// classes
// --------------------------------------------------------------------------

// Gestion des constantes pour les langues
// language data
static const wxLanguage langIds[] =
{
	wxLANGUAGE_DEFAULT,
	wxLANGUAGE_FRENCH,
	wxLANGUAGE_ENGLISH
};

const wxString langNames[] =
{
	"System default",
	"French",
	"English"
};


// Define a new application type
class FilmEcran : public wxApp
{
public:
	virtual bool OnInit();
};

// Define a new frame type: this is going to be our main frame
class Controle : public wxFrame
{
public:
	Controle();
	~Controle();

	// event handlers (these functions should _not_ be virtual)
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnParametre(wxCommandEvent& event);
	void DebutFilm(wxCommandEvent& event);
	void FinFilm(wxCommandEvent& event);

	void OnServerEvent(wxSocketEvent& event);
	void OnSocketEvent(wxSocketEvent& event);

	void EnvoyerEcran(wxTimerEvent &);

	// convenience functions
	void UpdateStatusBar();

private:
	wxSocketBase	*sock;
	wxSocketServer	*m_server;
	wxTextCtrl		*m_text;
	wxMenu			*m_menuFile;
	wxMenuBar		*m_menuBar;
	wxTimer			*photo;
	wxString		cmdFFMPEG;
	long			indPort;
	wxString		videoDst;
	bool            m_busy;
	int             numVideo;
	vector<wxSize>	taille;
	wxSize			tailleVideo;
	int				indTailleVideo;
	wxStatusBar		*st;

	wxLanguage		langue;							/*!< language choisi */
	wxLocale		locale;							/*!< locale we'll be using */
	wxFileConfig	*configApp;
};

class Journal
{
public:
Journal(const wxString& name) : m_name(name)
{wxLogMessage("=== %s begins ===", m_name);}

~Journal(){wxLogMessage("=== %s ends ===", m_name);}

private:
	const wxString m_name;
};

// --------------------------------------------------------------------------
// constants
// --------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
	// menu items
	SERVER_UDPTEST = 10,
	SERVER_WAITFORACCEPT,
	SERVER_QUIT = wxID_EXIT,
	SERVER_ABOUT = wxID_ABOUT,

	// id for sockets
	SERVER_ID = 100,
	SOCKET_ID
};


IMPLEMENT_APP(FilmEcran)


bool FilmEcran::OnInit()
{

if (!wxApp::OnInit())
	return false;
wxInitAllImageHandlers();
Controle *ctrl = new Controle();
ctrl->Show(true);
return true;
}

enum {
	MENU_DEBUT_FILM, MENU_FIN_FILM, MENU_PARAMETRE, MENU_FFMPEG, MENU_APROPOS, MENU_REPDST, MENU_QUIT
};

Controle::Controle() : wxFrame((wxFrame *)NULL, wxID_ANY,
	_("Record your screen"),
	wxDefaultPosition, wxSize(300, 200))
{
configApp = new wxFileConfig("wxFilmEcran", "LB", "wxFilmEcran.ini", wxEmptyString);

;
//gestion du langage
long l;
langue = wxLANGUAGE_UNKNOWN;
langue = (wxLanguage)configApp->Read("langue", langue);
if (langue == wxLANGUAGE_UNKNOWN)
{
	int lng = wxGetSingleChoiceIndex
		(
		_("Please choose language:"),
		_("Language"),
		WXSIZEOF(langNames),
		langNames
		);
	langue = lng == -1 ? wxLANGUAGE_DEFAULT : langIds[lng];
}

if (!locale.Init(langue, wxLOCALE_DONT_LOAD_DEFAULT))
{
	wxLogWarning(_("This language is not supported by the system."));
}
wxLocale::AddCatalogLookupPathPrefix("../lang");
const wxLanguageInfo* pInfo = wxLocale::GetLanguageInfo(langue);
if (!locale.AddCatalog("messages"))
{
	wxLogError(_("Couldn't find/load the 'main' catalog for locale '%s'."),
		pInfo ? pInfo->GetLocaleName() : _("unknown"));
}
locale.AddCatalog("wxstd");
#ifdef __LINUX__
{
	wxLogNull noLog;
	locale.AddCatalog("fileutils");
}
#endif
int largeur,hauteur;
configApp->Write("langue", (long)langue);
configApp->Flush();

if (!configApp->Read("taille0x",&largeur))
	{
	configApp->Write("taille0x", 160);
	configApp->Write("taille0y", 120);
	configApp->Write("taille1x", 320);
	configApp->Write("taille1y", 240);
	configApp->Write("taille2x", 640);
	configApp->Write("taille2y", 480);
	configApp->Write("taille3x", 800);
	configApp->Write("taille3y", 600);
	configApp->Write("taille4x", 800);
	configApp->Write("taille4y", 600);
	configApp->Write("taille5x", 960);
	configApp->Write("taille5y", 720);
	configApp->Write("taille6x", 1024);
	configApp->Write("taille6y", 768);
	configApp->Write("taille7x", 1280);
	configApp->Write("taille7y", 720);
	configApp->Write("taille8x", 1280);
	configApp->Write("taille8y", 960);
	}
int i = 0;
wxString cle;
cle.Printf("taille%dx",i);
while (configApp->Read(cle,&largeur))	
	{
	cle.Printf("taille%dy", i);
	configApp->Read(cle, &hauteur);
	taille.push_back(wxSize(largeur,hauteur));
	i++;
	cle.Printf("taille%dx", i);
	}
if (!configApp->Read("cmdFFMPEG", &cmdFFMPEG))
	{
	wxCommandEvent w;
	w.SetId(MENU_FFMPEG);
	OnParametre(w);
	}
if (!configApp->Read("indPort", &indPort))
	{
	wxCommandEvent w;
	w.SetId(MENU_PARAMETRE);
	OnParametre(w);
	}
if (!configApp->Read("RepertoireVideo",&videoDst))
	{
	wxCommandEvent w;
	w.SetId(MENU_REPDST);
	OnParametre(w);
	}


configApp->Read("largeurVideo", &largeur);
configApp->Read("hauteurVideo", &hauteur);
tailleVideo = wxSize(largeur, hauteur);
SetSize(largeur,hauteur);
wxPoint p(0, 0);
SetPosition(p);
vector<wxSize>::iterator it;
i=0;
for (it=taille.begin();it!=taille.end();it++,i++)
	if (it->GetX() == tailleVideo.GetX() && it->GetY() == tailleVideo.GetY())
		indTailleVideo = i;
// Give the frame an icon
SetIcon(wxICON(sample));
sock=NULL;
// Make menus
m_menuFile = new wxMenu();
m_menuFile->Append(MENU_DEBUT_FILM, _("&Start recording\tCtrl-G"));
m_menuFile->Append(MENU_FIN_FILM, _("&Stop recording\tCtrl-S"));
m_menuFile->AppendSeparator();
m_menuFile->Append(MENU_PARAMETRE, _("&Settings"));
m_menuFile->Append(MENU_FFMPEG, _("&FFMPEG path")); 
m_menuFile->Append(MENU_REPDST, _("&Destination folder"));
m_menuFile->AppendSeparator();
m_menuFile->Append(MENU_APROPOS, _("&About"), _("Show about dialog"));
m_menuFile->AppendSeparator();
m_menuFile->Append(MENU_QUIT, _("E&xit\tAlt-X"), _("Quit server"));

// Append menus to the menubar
m_menuBar = new wxMenuBar();
m_menuBar->Append(m_menuFile, _("&Video"));
SetMenuBar(m_menuBar);
Bind(wxEVT_COMMAND_MENU_SELECTED, &Controle::OnQuit, this, MENU_QUIT);
Bind(wxEVT_COMMAND_MENU_SELECTED, &Controle::OnAbout, this, MENU_APROPOS);
Bind(wxEVT_COMMAND_MENU_SELECTED, &Controle::FinFilm, this, MENU_FIN_FILM);
Bind(wxEVT_COMMAND_MENU_SELECTED, &Controle::DebutFilm, this, MENU_DEBUT_FILM);
Bind(wxEVT_COMMAND_MENU_SELECTED, &Controle::OnParametre, this, MENU_PARAMETRE);
Bind(wxEVT_COMMAND_MENU_SELECTED, &Controle::OnParametre, this, MENU_FFMPEG);
Bind(wxEVT_COMMAND_MENU_SELECTED, &Controle::OnParametre, this, MENU_REPDST);
Bind(wxEVT_SOCKET, &Controle::OnServerEvent, this, SERVER_ID);
Bind(wxEVT_SOCKET, &Controle::OnSocketEvent, this, SOCKET_ID);

photo = NULL;
#if wxUSE_STATUSBAR
// Status bar
st=CreateStatusBar(2);
#endif // wxUSE_STATUSBAR

// Make a textctrl for logging
m_text = new wxTextCtrl(this, wxID_ANY,
	_("Welcome to wxFilmEcran\n"),
	wxDefaultPosition, wxDefaultSize,
	wxTE_MULTILINE | wxTE_READONLY);
delete wxLog::SetActiveTarget(new wxLogTextCtrl(m_text));

// Create the address - defaults to localhost:0 initially
wxIPV4address addr;
addr.Service(indPort);

wxLogMessage("Creating server at %s:%u", addr.IPAddress(), addr.Service());

// Create the socket
m_server = new wxSocketServer(addr);

// We use IsOk() here to see if the server is really listening
if (!m_server->IsOk())
{
	wxLogMessage("Could not listen at the specified port !");
	return;
}

wxIPV4address addrReal;
if (!m_server->GetLocal(addrReal))
{
	wxLogMessage("ERROR: couldn't get the address we bound to");
}
else
{
	wxLogMessage("Server listening at %s:%u",
		addrReal.IPAddress(), addrReal.Service());
}

// Setup the event handler and subscribe to connection events
m_server->SetEventHandler(*this, SERVER_ID);
m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
m_server->Notify(true);

m_busy = false;
numVideo = 0;
UpdateStatusBar();
}

Controle::~Controle()
{
	// No delayed deletion here, as the frame is dying anyway
	delete m_server;
}

// event handlers

void Controle::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	// true is to force the frame to close
	Close(true);
}

void Controle::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(_("FilmEcran using wxWidgets and ffmpeg-\nL. Berger"),
		_("About FilmEcran"),
		wxOK | wxICON_INFORMATION, this);
}

void Controle::OnParametre(wxCommandEvent& event)
{
wxDialog d(this,-1,_("Settings"));

switch (event.GetId()){
case MENU_PARAMETRE:
	{
	new wxStaticText((wxWindow*)&d, 100, cmdFFMPEG, wxPoint(60, 10), wxSize(400, 20));
	wxArrayString mode;
	vector<wxSize>::iterator it;
	int i = 0;
	for (it = taille.begin(); it != taille.end(); it++, i++)
		{
		wxString s;
		s.Printf("%dx%d",it->GetX(),it->GetY());
		mode.Add(s);
		}

	new wxStaticText((wxWindow*)&d, 101, _("Video size"), wxPoint(10, 40), wxSize(150, 20));
	wxChoice *c=new wxChoice((wxWindow*)&d, 12, wxPoint(160, 40), wxSize(150, 20), mode);
	c->SetSelection(indTailleVideo);

	new wxStaticText((wxWindow*)&d, 102, _("Port number"), wxPoint(10, 70), wxSize(100, 20));
	wxString s;
	s.Printf("%d",indPort);
	wxTextCtrl *te=new wxTextCtrl((wxWindow*)&d, 13, s, wxPoint(110, 70), wxSize(110, 20));

	new wxButton((wxWindow*)&d, wxID_OK, _("OK"), wxPoint(10, 100), wxSize(50, 20));
	new wxButton((wxWindow*)&d, wxID_CANCEL, _("Cancel"), wxPoint(60, 100), wxSize(50, 20));

	if (d.ShowModal())
		{
		s=te->GetValue();
		s.ToLong(&indPort);
		indTailleVideo=c->GetSelection();
		tailleVideo=taille[indTailleVideo];
		configApp->Write("indPort", indPort);
		configApp->Write("largeurVideo", tailleVideo.x);
		configApp->Write("hauteurVideo",tailleVideo.y);
		configApp->Flush();
		SetSize(tailleVideo.x, tailleVideo.y);
		wxPoint p(0,0);
		SetPosition(p);
		}
	else
		return;
	break;
	}
case MENU_FFMPEG:
	{
	wxFileDialog f(this, _("Path to FFMPEG"),  wxEmptyString, wxEmptyString, "ffmpeg.*", wxFD_FILE_MUST_EXIST );
	if (f.ShowModal() != wxID_OK)
		return;
	cmdFFMPEG=f.GetPath();
	configApp->Write("cmdFFMPEG", cmdFFMPEG);
	configApp->Flush();
	break;
	}
case MENU_REPDST:
	{
	wxDirDialog f(this, _("Path to video folder"));
	if (f.ShowModal() != wxID_OK)
		return;
	videoDst = f.GetPath() + wxFILE_SEP_PATH;

	configApp->Write("RepertoireVideo", videoDst);
	configApp->Flush();
	break;
	}
	}

}


void Controle::DebutFilm(wxCommandEvent& WXUNUSED(event))
{
	wxString	cmd;
	wxString	nom;
	wxDateTime	m;
	m=m.Now();
	nom.Printf("%d",m.GetYear());
	if (m.GetMonth()+1<10)
		nom.Printf("%s0%d", nom, m.GetMonth()+1);
	else
		nom.Printf("%s%d", nom, m.GetMonth()+1);
	if (m.GetDay()<10)
		nom.Printf("%s0%d", nom, m.GetDay());
	else
		nom.Printf("%s%d", nom, m.GetDay());
	if (m.GetHour()<10)
		nom.Printf("%s0%d", nom, m.GetHour());
	else
		nom.Printf("%s%d", nom, m.GetHour());
	if (m.GetMinute()<10)
		nom.Printf("%s0%d", nom, m.GetMinute());
	else
		nom.Printf("%s%d", nom, m.GetMinute());
	if (m.GetSecond()<10)
		nom.Printf("%s0%d", nom, m.GetSecond());
	else
		nom.Printf("%s%d", nom, m.GetSecond());
		
	cmd.Printf("\"%s\" -f rawvideo -pixel_format rgb24  -video_size %dx%d -i \"tcp://127.0.0.1:%d\" -codec:v libx264 -pix_fmt yuv420p %sVideo%s.mp4", cmdFFMPEG, tailleVideo.GetWidth(), tailleVideo.GetHeight(), indPort,videoDst,nom);
	wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE);
	m_text->SetBackgroundColour(wxColour(255, 0, 0));
	m_text->Refresh();
	UpdateStatusBar();
}


void Controle::FinFilm(wxCommandEvent& WXUNUSED(event))
{
if (photo)
	{
	photo->Stop();
	photo->Unbind(wxEVT_TIMER,&Controle::EnvoyerEcran,this,1);
	}
if (sock)
	{	
	sock->Close();
	sock = NULL;
	}
m_text->SetBackgroundColour(wxColour(255, 255, 255));
m_text->Refresh();
UpdateStatusBar();
}



void Controle::OnServerEvent(wxSocketEvent& event)
{
	wxString s = _("OnServerEvent: ");

	switch (event.GetSocketEvent())
	{
	case wxSOCKET_CONNECTION: s.Append(_("wxSOCKET_CONNECTION\n")); break;
	default: s.Append(_("Unexpected event !\n")); break;
	}

	m_text->AppendText(s);

	// Accept new connection if there is one in the pending
	// connections queue, else exit. We use Accept(false) for
	// non-blocking accept (although if we got here, there
	// should ALWAYS be a pending connection).

	if (sock!=NULL)
		{
		wxLogMessage("Error: couldn't accept a new connection");
		return;
		}
	sock = m_server->Accept(false);

	if (sock)
		{
		wxIPV4address addr;
		if (!sock->GetPeer(addr))
			wxLogMessage("New connection from unknown client accepted.");
		else
			wxLogMessage("New client connection from %s:%u accepted",
				addr.IPAddress(), addr.Service());
		}
	else
		{
		wxLogMessage("Error: couldn't accept a new connection");
		return;
		}

	sock->SetEventHandler(*this, SOCKET_ID);
	sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
	sock->Notify(true);
	photo = new wxTimer(this);
	photo->SetOwner(this);
	Bind(wxEVT_TIMER,&Controle::EnvoyerEcran,this);
	photo->Start(50, wxTIMER_CONTINUOUS);

	
	numVideo++;
	UpdateStatusBar();
}


void Controle::EnvoyerEcran(wxTimerEvent &event)
{
		wxScreenDC dcScreen;

		int height = tailleVideo.GetHeight();
		int width = tailleVideo.GetWidth();
        wxCursor c=GetCursor();
        wxPoint p=wxGetMousePosition();
		wxBitmap copieEcran(width, height, -1);
        wxBitmap curseur(*wxSTANDARD_CURSOR);
		wxMemoryDC memDC;

		memDC.SelectObject(copieEcran);
		memDC.Blit(0, 0,width,height,&dcScreen,0, 0);
		memDC.DrawBitmap(curseur,p.x, p.y);
		int nb = width*height*3;
		memDC.SelectObject(wxNullBitmap);

		wxImage b = copieEcran.ConvertToImage();
		if (sock && sock->IsConnected())
			sock->Write(b.GetData(), nb);
		else
			{
			sock->Close();
			sock=NULL;
			photo->Stop();
			}
}

void Controle::OnSocketEvent(wxSocketEvent& event)
{
	wxString s = _("OnSocketEvent: ");
	wxSocketBase *sock = event.GetSocket();

	// First, print a message
	switch (event.GetSocketEvent())
	{
	case wxSOCKET_INPUT: s.Append(_("wxSOCKET_INPUT\n")); break;
	case wxSOCKET_LOST: s.Append(_("wxSOCKET_LOST\n")); break;
	default: s.Append(_("Unexpected event !\n")); break;
	}

	m_text->AppendText(s);

	// Now we process the event
	switch (event.GetSocketEvent()){
	case wxSOCKET_INPUT:
		sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
		break;
	case wxSOCKET_LOST:


		wxLogMessage("Deleting socket.");
		sock->Destroy();
		break;
	default:;
		}

	UpdateStatusBar();
}

// convenience functions

void Controle::UpdateStatusBar()
{
#if wxUSE_STATUSBAR
	wxString s;
	s.Printf(_("%d Video created"), numVideo);
	SetStatusText(s, 1);
#endif // wxUSE_STATUSBAR
}







