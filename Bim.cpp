#include <wx/wx.h>
#include <wx/mediactrl.h>
#include <wx/slider.h>
#include <wx/timer.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

// Forward declaration of the main frame class
class MediaPlayerFrame : public wxFrame {
public:
    MediaPlayerFrame(const wxString& title);

private:
    wxMediaCtrl* mediaCtrl;
    wxSlider* volumeSlider;
    wxSlider* seekSlider;
    wxTimer* timer;

    void OnOpenFile(wxCommandEvent& event);
    void OnPlay(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnVolumeChange(wxCommandEvent& event);
    void OnSeek(wxCommandEvent& event);
    void UpdateSeekBar(wxTimerEvent& event);

    wxDECLARE_EVENT_TABLE();
};

// Event table to map events to handler functions
enum {
    ID_Open = wxID_HIGHEST + 1,
    ID_Play,
    ID_Pause,
    ID_Stop,
    ID_Volume,
    ID_Seek,
    ID_Timer
};

wxBEGIN_EVENT_TABLE(MediaPlayerFrame, wxFrame)
    EVT_BUTTON(ID_Open, MediaPlayerFrame::OnOpenFile)
    EVT_BUTTON(ID_Play, MediaPlayerFrame::OnPlay)
    EVT_BUTTON(ID_Pause, MediaPlayerFrame::OnPause)
    EVT_BUTTON(ID_Stop, MediaPlayerFrame::OnStop)
    EVT_SLIDER(ID_Volume, MediaPlayerFrame::OnVolumeChange)
    EVT_SLIDER(ID_Seek, MediaPlayerFrame::OnSeek)
    EVT_TIMER(ID_Timer, MediaPlayerFrame::UpdateSeekBar)
wxEND_EVENT_TABLE()

// Application class
class MediaPlayerApp : public wxApp {
public:
    virtual bool OnInit();
};

wxIMPLEMENT_APP(MediaPlayerApp);

bool MediaPlayerApp::OnInit() {
    MediaPlayerFrame* frame = new MediaPlayerFrame("Advanced Media Player");
    frame->Show(true);
    return true;
}

// Constructor for the main frame
MediaPlayerFrame::MediaPlayerFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
      timer(new wxTimer(this, ID_Timer)) {
    // Media Control
    mediaCtrl = new wxMediaCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(800, 450));

    // Buttons
    wxButton* openButton = new wxButton(this, ID_Open, "Open");
    wxButton* playButton = new wxButton(this, ID_Play, "Play");
    wxButton* pauseButton = new wxButton(this, ID_Pause, "Pause");
    wxButton* stopButton = new wxButton(this, ID_Stop, "Stop");

    // Volume Control
    wxStaticText* volumeLabel = new wxStaticText(this, wxID_ANY, "Volume:");
    volumeSlider = new wxSlider(this, ID_Volume, 50, 0, 100);

    // Seek Slider
    wxStaticText* seekLabel = new wxStaticText(this, wxID_ANY, "Seek:");
    seekSlider = new wxSlider(this, ID_Seek, 0, 0, 1000);

    // Layout
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(openButton, 0, wxALL, 5);
    buttonSizer->Add(playButton, 0, wxALL, 5);
    buttonSizer->Add(pauseButton, 0, wxALL, 5);
    buttonSizer->Add(stopButton, 0, wxALL, 5);

    wxBoxSizer* controlSizer = new wxBoxSizer(wxHORIZONTAL);
    controlSizer->Add(volumeLabel, 0, wxALL, 5);
    controlSizer->Add(volumeSlider, 1, wxEXPAND | wxALL, 5);
    controlSizer->Add(seekLabel, 0, wxALL, 5);
    controlSizer->Add(seekSlider, 3, wxEXPAND | wxALL, 5);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(mediaCtrl, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER);
    mainSizer->Add(controlSizer, 0, wxEXPAND | wxALL, 5);

    SetSizer(mainSizer);
}

// Event Handlers
void MediaPlayerFrame::OnOpenFile(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, "Open Media File", "", "",
        "Media Files|*.mp3;*.mp4;*.avi;*.mkv;*.wav|All Files|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return; // User canceled
    }

    wxString filePath = openFileDialog.GetPath();
    if (!mediaCtrl->Load(filePath)) {
        wxMessageBox("Unable to load media file.", "Error", wxICON_ERROR);
    }
    seekSlider->SetValue(0);
    timer->Start(100); // Update seek bar every 100ms
}

void MediaPlayerFrame::OnPlay(wxCommandEvent& event) {
    mediaCtrl->Play();
}

void MediaPlayerFrame::OnPause(wxCommandEvent& event) {
    mediaCtrl->Pause();
}

void MediaPlayerFrame::OnStop(wxCommandEvent& event) {
    mediaCtrl->Stop();
    seekSlider->SetValue(0);
    timer->Stop();
}

void MediaPlayerFrame::OnVolumeChange(wxCommandEvent& event) {
    mediaCtrl->SetVolume(volumeSlider->GetValue() / 100.0);
}

void MediaPlayerFrame::OnSeek(wxCommandEvent& event) {
    mediaCtrl->Seek(seekSlider->GetValue() * mediaCtrl->Length() / 1000);
}

void MediaPlayerFrame::UpdateSeekBar(wxTimerEvent& event) {
    if (mediaCtrl->IsPlaying()) {
        seekSlider->SetValue((mediaCtrl->Tell() * 1000) / mediaCtrl->Length());
    }
}

libvlc_instance_t* instance = libvlc_new(0, nullptr);
libvlc_media_player_t* player = libvlc_media_player_new(instance);
libvlc_media_t* media = libvlc_media_new_path(instance, "media_file.mp4");
libvlc_media_player_set_media(player, media);
libvlc_media_player_play(player);
