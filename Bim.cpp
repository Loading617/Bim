#include <wx/wx.h>
#include <wx/mediactrl.h>
#include <wx/filedlg.h>

class MediaPlayerApp : public wxApp {
public:
    virtual bool OnInit();
};

class MediaPlayerFrame : public wxFrame {
public:
    MediaPlayerFrame(const wxString& title);

private:
    void OnOpenFile(wxCommandEvent& event);
    void OnPlay(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);

    wxMediaCtrl* mediaCtrl;
    wxButton* playButton;
    wxButton* pauseButton;
    wxButton* stopButton;
};

enum {
    ID_Open = wxID_HIGHEST + 1,
    ID_Play,
    ID_Pause,
    ID_Stop
};

wxIMPLEMENT_APP(MediaPlayerApp);

bool MediaPlayerApp::OnInit() {
    MediaPlayerFrame* frame = new MediaPlayerFrame("wxWidgets Media Player");
    frame->Show(true);
    return true;
}

MediaPlayerFrame::MediaPlayerFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)) {
    // Create media control
    mediaCtrl = new wxMediaCtrl(this, wxID_ANY);

    // Create buttons
    wxButton* openButton = new wxButton(this, ID_Open, "Open File");
    playButton = new wxButton(this, ID_Play, "Play");
    pauseButton = new wxButton(this, ID_Pause, "Pause");
    stopButton = new wxButton(this, ID_Stop, "Stop");

    // Arrange buttons in a sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(openButton, 0, wxALL, 5);
    buttonSizer->Add(playButton, 0, wxALL, 5);
    buttonSizer->Add(pauseButton, 0, wxALL, 5);
    buttonSizer->Add(stopButton, 0, wxALL, 5);

    // Arrange media control and buttons in a vertical sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(mediaCtrl, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER);

    SetSizer(mainSizer);

    // Bind events
    Bind(wxEVT_BUTTON, &MediaPlayerFrame::OnOpenFile, this, ID_Open);
    Bind(wxEVT_BUTTON, &MediaPlayerFrame::OnPlay, this, ID_Play);
    Bind(wxEVT_BUTTON, &MediaPlayerFrame::OnPause, this, ID_Pause);
    Bind(wxEVT_BUTTON, &MediaPlayerFrame::OnStop, this, ID_Stop);
}

void MediaPlayerFrame::OnOpenFile(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, "Open Media File", "", "",
        "Media Files|*.mp3;*.mp4;*.avi;*.mkv;*.wav;*.flac|All Files|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return; // User cancelled
    }

    wxString filePath = openFileDialog.GetPath();
    if (!mediaCtrl->Load(filePath)) {
        wxMessageBox("Unable to load media file.", "Error", wxICON_ERROR);
    }
}

void MediaPlayerFrame::OnPlay(wxCommandEvent& event) {
    if (mediaCtrl->Play()) {
        playButton->Disable();
        pauseButton->Enable();
    } else {
        wxMessageBox("Unable to play media file.", "Error", wxICON_ERROR);
    }
}

void MediaPlayerFrame::OnPause(wxCommandEvent& event) {
    if (mediaCtrl->Pause()) {
        playButton->Enable();
        pauseButton->Disable();
    } else {
        wxMessageBox("Unable to pause media file.", "Error", wxICON_ERROR);
    }
}

void MediaPlayerFrame::OnStop(wxCommandEvent& event) {
    mediaCtrl->Stop();
    playButton->Enable();
    pauseButton->Disable();
}
