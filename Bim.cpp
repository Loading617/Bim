#include "stdafx.h" 

// include the LEAD Multimedia TOOLKIT header 
#include "ltmm.h" 

#include "resource.h" 
#include <tchar.h> 
#include <stdio.h> 
#include <math.h> 
#include <rpcasync.h>
#include <wtypes.h>

#define SZ_WNDCLASS_PLAY _T("PLAY WNDCLASS") 
#define WM_PLAYNOTIFY (WM_USER + 1000)  

HINSTANCE g_hInstance;    // application instance handle 
HWND g_hwndPlay;      // video frame window 
IltmmPlay* g_pPlay;      // play object interface pointer 
int g_nPositionView;   // current position indicator view mode 
enum
{
    POSITIONVIEW_TIME,
    POSITIONVIEW_FRAME,
    POSITIONVIEW_TRACKING
};

// 
// SnapFrameToVideo 
// resizes the frame window to match the video width and height 
// 
void SnapFrameToVideo(void)
{
    HWND hwnd;
    RECT rcWindow, rcClient;
    long cx, cy;

    // get the frame window 
    g_pPlay->get_VideoWindowFrame((long*)&hwnd);

    // get the video dimensions 
    g_pPlay->get_VideoWidth(&cx);
    g_pPlay->get_VideoHeight(&cy);

    // adjust by the border dimensions 
    GetWindowRect(hwnd, &rcWindow);
    GetClientRect(hwnd, &rcClient);
    cx += ((rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left));
    cy += ((rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top));

    // resize the window 
    SetWindowPos(hwnd, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
}

// 
// FreeSource 
// resets source and frees associated resources 
// 
void FreeSource(void)
{
    long type;
    VARIANT var;
    HGLOBAL hglobal;

    g_pPlay->get_SourceType(&type);
    if (type == ltmmPlay_Source_Array)
    {
        g_pPlay->get_SourceArray(&var);
        g_pPlay->ResetSource();
        VariantClear(&var);
    }
    else if (type == ltmmPlay_Source_HGlobal)
    {
        g_pPlay->get_SourceHGlobal((long*)&hglobal);
        g_pPlay->ResetSource();
        GlobalFree(hglobal);
    }
    else
    {
        g_pPlay->ResetSource();
    }

}

// 
// SetSourceArray 
// preloads a SAFEARRAY with media data and assigns it to the play object 
// 
void SetSourceArray(void)
{
    HRESULT hr;
    HANDLE hfile;
    DWORD size, cb;
    void* buffer;
    VARIANT var;
    SAFEARRAY* psaSource;

    // open the source file 
    hfile = CreateFile(_T("c:\\source.avi"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE)
        return;

    // allocate same-sized SAFEARRAY 

    size = GetFileSize(hfile, NULL);

    psaSource = SafeArrayCreateVector(VT_UI1, 0, size);
    if (!psaSource)
    {
        CloseHandle(hfile);
        return;
    }

    // read entire source into array 
    SafeArrayAccessData(psaSource, (void**)&buffer);
    if (!ReadFile(hfile, buffer, size, &cb, NULL) || cb != size)
    {
        SafeArrayUnaccessData(psaSource);
        CloseHandle(hfile);
        SafeArrayDestroy(psaSource);
        return;
    }
    SafeArrayUnaccessData(psaSource);

    // close file 
    CloseHandle(hfile);

    // assign the source array 
    VariantInit(&var);
    V_VT(&var) = (VT_ARRAY | VT_UI1);
    V_ARRAY(&var) = psaSource;

    hr = g_pPlay->put_SourceArray(var);
    if (FAILED(hr))
    {
        SafeArrayDestroy(psaSource);
        return;
    }
}

// 
// SetSourceArray 
// preloaded global memory with media data and assigns it to the play object 
// 
void SetSourceHGlobal(void)
{
    HRESULT hr;
    HANDLE hfile;
    DWORD size, cb;
    void* buffer;
    HGLOBAL hglobal;

    // open the source file 
    hfile = CreateFile(_T("c:\\source.avi"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE)
        return;

    // allocate same-sized global memory 
    size = GetFileSize(hfile, NULL);

    hglobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hglobal)
    {
        CloseHandle(hfile);
        return;
    }

    // read entire source into memory 
    buffer = GlobalLock(hglobal);
    if (!ReadFile(hfile, buffer, size, &cb, NULL) || cb != size)
    {
        GlobalUnlock(hglobal);
        CloseHandle(hfile);
        GlobalFree(hglobal);
        return;
    }
    GlobalUnlock(hglobal);

    // close file 
    CloseHandle(hfile);


    hr = g_pPlay->put_SourceHGlobal((long)hglobal);
    if (FAILED(hr))
    {
        GlobalFree(hglobal);
        return;
    }
}

// 
// PlayWndProc 
// video frame window procedure 
// 
LRESULT CALLBACK PlayWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BSTR bstr;
    VARIANT_BOOL f;
    long l;
    TCHAR sz[2048];
    LPTSTR p;
    double d;
    HRESULT hr;
    ltmmSizeMode sm;
    long x, y, cx, cy;
    RECT rc;
    POINT pt;

    switch (message)
    {
    case WM_CREATE:
        g_hwndPlay = hwnd;
        // window is the video window frame 
        g_pPlay->put_VideoWindowFrame((long)hwnd);

        // want notification messages as well 
        g_pPlay->SetNotifyWindow((long)hwnd, WM_PLAYNOTIFY);

        // make sure we start playing immediately 

        g_pPlay->put_AutoStart(VARIANT_TRUE);

        // set the source file 
        bstr = SysAllocString(L"c:\\source.avi");
        g_pPlay->put_SourceFile(bstr);
        SysFreeString(bstr);

        SnapFrameToVideo();

        return 0;
        break;
    case WM_INITMENUPOPUP:
        if (GetSubMenu(GetMenu(hwnd), 0) == (HMENU)wParam)
        {
            // enable menu items based on the play object state 
            g_pPlay->get_State(&l);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_PLAY, (l == ltmmPlay_State_Paused || l == ltmmPlay_State_Stopped) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_PAUSE, (l == ltmmPlay_State_Running) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_STOP, (l == ltmmPlay_State_Running || l == ltmmPlay_State_Paused) ? MF_ENABLED : MF_GRAYED);

            // enable menu items based on seeking capabilities 
            g_pPlay->CheckSeekingCapabilities(ltmmPlay_Seeking_Forward | ltmmPlay_Seeking_Backward | ltmmPlay_Seeking_FrameForward | ltmmPlay_Seeking_FrameBackward, &l);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_SEEKSTART, (l & ltmmPlay_Seeking_Backward) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_SEEKEND, (l & ltmmPlay_Seeking_Forward) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_NEXTFRAME, (l & ltmmPlay_Seeking_FrameForward) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_PREVIOUSFRAME, (l & ltmmPlay_Seeking_FrameBackward) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_SEEKSELECTIONSTART, (l & (ltmmPlay_Seeking_Forward | ltmmPlay_Seeking_Backward)) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_SEEKSELECTIONEND, (l & (ltmmPlay_Seeking_Forward | ltmmPlay_Seeking_Backward)) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_LASTFRAME, (l & ltmmPlay_Seeking_FrameForward) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_FIRSTFRAME, (l & ltmmPlay_Seeking_FrameBackward) ? MF_ENABLED : MF_GRAYED);

            EnableMenuItem((HMENU)wParam, ID_CONTROL_STEPFORWARD1SEC, (l & ltmmPlay_Seeking_Forward) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_STEPFORWARD10, (l & ltmmPlay_Seeking_Forward) ? MF_ENABLED : MF_GRAYED);

            // check the current speed 
            g_pPlay->get_Rate(&d);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_HALFSPEED, (fabs(d - 0.5) < 0.1) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_NORMALSPEED, (fabs(d - 1.0) < 0.1) ? MF_CHECKED : MF_UNCHECKED);

            // check the current video size mode 
            g_pPlay->get_VideoWindowSizeMode(&sm);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_FITTOWINDOW, (sm == ltmmFit) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_STRETCHTOWINDOW, (sm == ltmmStretch) ? MF_CHECKED : MF_UNCHECKED);

            // enable volume menu items 
            g_pPlay->get_Volume(&l);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_INCREASEVOLUME, (l < 0) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_DECREASEVOLUME, (l > -10000) ? MF_ENABLED : MF_GRAYED);

            // enable balance menu items 
            g_pPlay->get_Balance(&l);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_PANRIGHT, (l < 10000) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_PANLEFT, (l > -10000) ? MF_ENABLED : MF_GRAYED);


            // check mute 
            g_pPlay->get_Mute(&f);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_MUTE, f ? MF_CHECKED : MF_UNCHECKED);

            // check auto rewind 
            g_pPlay->get_AutoRewind(&f);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_AUTOREWIND, f ? MF_CHECKED : MF_UNCHECKED);

            // check loop 
            g_pPlay->get_PlayCount(&l);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_LOOP, (l == 0) ? MF_CHECKED : MF_UNCHECKED);

            // enable processors 
            g_pPlay->HasDialog(ltmmPlay_Dlg_VideoProcessors, &f);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_PROCESSORS_VIDEO, f ? MF_ENABLED : MF_GRAYED);
            g_pPlay->HasDialog(ltmmPlay_Dlg_AudioProcessors, &f);
            EnableMenuItem((HMENU)wParam, ID_CONTROL_PROCESSORS_AUDIO, f ? MF_ENABLED : MF_GRAYED);

            // check source 
            g_pPlay->get_SourceType(&l);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_SOURCE_FILE, (l == ltmmPlay_Source_File) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_SOURCE_ARRAY, (l == ltmmPlay_Source_Array) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_SOURCE_HGLOBAL, (l == ltmmPlay_Source_HGlobal) ? MF_CHECKED : MF_UNCHECKED);

            g_pPlay->get_AutoStart(&f);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_AUTOSTART, f ? MF_CHECKED : MF_UNCHECKED);

            // check position view 
            CheckMenuItem((HMENU)wParam, ID_CONTROL_POSITIONVIEW_FRAME, (g_nPositionView == POSITIONVIEW_FRAME) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_POSITIONVIEW_TIME, (g_nPositionView == POSITIONVIEW_TIME) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, ID_CONTROL_POSITIONVIEW_TRACKING, (g_nPositionView == POSITIONVIEW_TRACKING) ? MF_CHECKED : MF_UNCHECKED);

        }
        break;
    case WM_PLAYNOTIFY:
        switch (wParam)
        {
        case ltmmPlay_Notify_TrackingSelectionChanged:
            break;
        case ltmmPlay_Notify_TrackingPositionChanged:
            p = sz;

            *p = _T('\0');

            if (g_nPositionView == POSITIONVIEW_TRACKING)   // tracking position based 
            {
                // display current tracking position 
                hr = g_pPlay->get_CurrentTrackingPosition(&l);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Trk. Pos.: %d"), l);
                }

                // display current tracking selection start 
                hr = g_pPlay->get_TrackingSelectionStart(&l);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Trk. Start: %d"), l);
                }

                // display current tracking selection end 
                hr = g_pPlay->get_TrackingSelectionEnd(&l);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Trk. End: %d"), l);
                }
            }
            else if (g_nPositionView == POSITIONVIEW_FRAME)   // frame based 
            {
                // display current frame position 
                hr = g_pPlay->get_CurrentFramePosition(&l);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Frame: %d"), l + 1);
                }

                // display total number of frames 
                hr = g_pPlay->get_FrameDuration(&l);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Total Frames: %d"), l);
                }


            }
            else // time based 
            {
                // display the current position 
                hr = g_pPlay->get_CurrentPosition(&d);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Position: %f"), d);
                }

                // display the total duration 
                hr = g_pPlay->get_Duration(&d);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Duration: %f"), d);
                }

                // display current selection start 
                hr = g_pPlay->get_SelectionStart(&d);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("Start: %f"), d);
                }

                // display current selection end 
                hr = g_pPlay->get_SelectionEnd(&d);
                if (SUCCEEDED(hr))
                {
                    if (p != sz)
                        p += _stprintf(p, _T(", "));
                    p += _stprintf(p, _T("End: %f"), d);
                }

            }
            SetWindowText(hwnd, sz);
            break;
        case ltmmPlay_Notify_StateChanged:
            switch (LOWORD(lParam))
            {
            case ltmmPlay_State_NotReady:
                _stprintf(sz, _T("Not Ready"));
                SetWindowText(hwnd, sz);
                break;
            case ltmmPlay_State_Stopped:
                // uncomment the following line to view the graph in DirectShow GraphEdit 

                // g_pPlay->EditGraph(); 

                g_pPlay->get_SourceType(&l);
                if (l == ltmmPlay_Source_Array)
                {
                    _stprintf(sz, _T("Stopped - [array]"));
                }
                else if (l == ltmmPlay_Source_HGlobal)
                {
                    _stprintf(sz, _T("Stopped - [hglobal]"));
                }
                else
                {
                    g_pPlay->get_SourceFile(&bstr);
                    _stprintf(sz, _T("Stopped - [%ls]"), bstr);
                    SysFreeString(bstr);
                }
                SetWindowText(hwnd, sz);
                break;
            case ltmmPlay_State_Paused:
                g_pPlay->get_SourceType(&l);
                if (l == ltmmPlay_Source_Array)
                {
                    _stprintf(sz, _T("Paused - [array]"));
                }
                else if (l == ltmmPlay_Source_HGlobal)
                {
                    _stprintf(sz, _T("Paused - [hglobal]"));
                }
                else
                {
                    g_pPlay->get_SourceFile(&bstr);
                    _stprintf(sz, _T("Paused - [%ls]"), bstr);
                    SysFreeString(bstr);
                }
                SetWindowText(hwnd, sz);
                break;
            case ltmmPlay_State_Running:
                _stprintf(sz, _T("Playing"));
                SetWindowText(hwnd, sz);
                break;
            }
            break;
        case ltmmPlay_Notify_Error:
            _stprintf(sz, _T("Error 0x%.8X. Playback stopped."), lParam);
            MessageBox(hwnd, sz, _T("Play"), MB_ICONEXCLAMATION | MB_OK);
            break;
        }
        return 0;
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            // if fullscreen mode then exit it 
            g_pPlay->get_FullScreenMode(&f);
            if (f)
            {
                g_pPlay->put_FullScreenMode(VARIANT_FALSE);
                return 0;
            }
        }      break;
    case WM_DESTROY:
        FreeSource();
        // no more notifications 
        g_pPlay->SetNotifyWindow((long)NULL, 0);
        PostQuitMessage(0);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_CONTROL_POSITIONVIEW_TIME:
            g_nPositionView = POSITIONVIEW_TIME;
            break;
        case ID_CONTROL_POSITIONVIEW_FRAME:
            g_nPositionView = POSITIONVIEW_FRAME;
            break;
        case ID_CONTROL_POSITIONVIEW_TRACKING:
            g_nPositionView = POSITIONVIEW_TRACKING;
            break;
        case ID_CONTROL_AUTOSTART:
            // toggle auto start 
            g_pPlay->get_AutoStart(&f);
            g_pPlay->put_AutoStart(f ? VARIANT_FALSE : VARIANT_TRUE);
            return 0;
            break;
        case ID_CONTROL_PLAY:
            // play file 
            g_pPlay->Run();
            return 0;
            break;
        case ID_CONTROL_PAUSE:
            // pause playback 
            g_pPlay->Pause();
            return 0;
            break;
        case ID_CONTROL_STOP:
            // stop playback 
            g_pPlay->Stop();
            return 0;
            break;
        case ID_CONTROL_HALFSPEED:
            // set to half the normal playback speed 
            g_pPlay->put_Rate(0.5);
            return 0;
            break;
        case ID_CONTROL_NORMALSPEED:
            // set to normal speed 
            g_pPlay->put_Rate(1.0);
            return 0;
            break;
        case ID_CONTROL_SEEKSTART:
            // seek to file start 
            g_pPlay->SeekStart();
            return 0;
            break;
        case ID_CONTROL_SEEKEND:
            // seek to file end 
            g_pPlay->SeekEnd();
            return 0;
            break;
        case ID_CONTROL_SEEKSELECTIONSTART:
            // seek to the start of the current selection 
            g_pPlay->SeekSelectionStart();
            return 0;
            break;
        case ID_CONTROL_SEEKSELECTIONEND:
            // seek to the end of the current selection 
            g_pPlay->SeekSelectionEnd();
            return 0;
            break;
        case ID_CONTROL_SETSELECTIONSTART:
            // set the start of the selection to the current position 
            g_pPlay->MarkSelectionStart();
            return 0;
            break;
        case ID_CONTROL_SETSELECTIONEND:
            // set the end of the selection to the current position 
            g_pPlay->MarkSelectionEnd();
            return 0;
            break;
        case ID_CONTROL_CLEARSELECTION:
            // clear the current selection 
            g_pPlay->get_Duration(&d);
            g_pPlay->put_SelectionStart(0.0);
            g_pPlay->put_SelectionEnd(d);
            return 0;
            break;
        case ID_CONTROL_NEXTFRAME:
            // goto the next frame 
            g_pPlay->NextFrame();
            return 0;
            break;
        case ID_CONTROL_PREVIOUSFRAME:
            // goto the previous frame 
            g_pPlay->PreviousFrame();
            return 0;
            break;
        case ID_CONTROL_FIRSTFRAME:
            // go to the first frame 
            g_pPlay->put_CurrentFramePosition(0);
            return 0;
            break;
        case ID_CONTROL_LASTFRAME:
            // go to the last frame 
            g_pPlay->get_FrameDuration(&l);
            g_pPlay->put_CurrentFramePosition(l - 1);
            return 0;
            break;
        case ID_CONTROL_STEPFORWARD1SEC:
            // step forward 1 second 
            g_pPlay->get_CurrentPosition(&d);
            g_pPlay->put_CurrentPosition(d + 1.0);
            return 0;
            break;
        case ID_CONTROL_STEPFORWARD10:
            // step forward 10% 
            g_pPlay->get_CurrentTrackingPosition(&l);
            g_pPlay->put_CurrentTrackingPosition(l + 1000);
            return 0;
            break;
        case ID_CONTROL_FITTOWINDOW:
            // fit the video to the window 
            g_pPlay->put_VideoWindowSizeMode(ltmmFit);
            return 0;
            break;
        case ID_CONTROL_STRETCHTOWINDOW:
            // stretch the video to the window 
            g_pPlay->put_VideoWindowSizeMode(ltmmStretch);
            return 0;
            break;
        case ID_CONTROL_MUTE:
            // toggle mute 
            g_pPlay->get_Mute(&f);
            g_pPlay->put_Mute(f ? VARIANT_FALSE : VARIANT_TRUE);
            return 0;
            break;
        case ID_CONTROL_INCREASEVOLUME:
            // increase the volume 
            g_pPlay->get_Volume(&l);
            g_pPlay->put_Volume(min(0, l + 300));
            return 0;
            break;
        case ID_CONTROL_DECREASEVOLUME:
            // decrease the volume 
            g_pPlay->get_Volume(&l);
            g_pPlay->put_Volume(max(-10000, l - 300));
            return 0;
            break;
        case ID_CONTROL_PANRIGHT:
            // pan balance to the right 
            g_pPlay->get_Balance(&l);
            g_pPlay->put_Balance(min(10000, l + 300));
            return 0;
            break;
        case ID_CONTROL_PANLEFT:
            // pan balance to the left 
            g_pPlay->get_Balance(&l);
            g_pPlay->put_Balance(max(-10000, l - 300));
            return 0;
            break;
        case ID_CONTROL_LOOP:
            // toggle looping 
            g_pPlay->get_PlayCount(&l);
            g_pPlay->put_PlayCount(l ? 0 : 1);
            return 0;
            break;
        case ID_CONTROL_FULLSCREEN:
            // toggle fullscreen mode 
            g_pPlay->ToggleFullScreenMode();
            return 0;
            break;
        case ID_CONTROL_AUTOREWIND:
            // toggle auto rewind 
            g_pPlay->get_AutoRewind(&f);
            g_pPlay->put_AutoRewind(f ? VARIANT_FALSE : VARIANT_TRUE);
            return 0;
            break;
        case ID_CONTROL_PROCESSORS_VIDEO:
            // invoke video processor dialog box 
            g_pPlay->ShowDialog(ltmmPlay_Dlg_VideoProcessors, (long)hwnd);
            return 0;
            break;
        case ID_CONTROL_PROCESSORS_AUDIO:
            // invoke audio processor dialog box 
            g_pPlay->ShowDialog(ltmmPlay_Dlg_AudioProcessors, (long)hwnd);
            return 0;
            break;
        case ID_CONTROL_SOURCE_FILE:
            // set the source file 
            FreeSource();
            bstr = SysAllocString(MAKE_MEDIA_PATH("source.avi"));
            g_pPlay->put_SourceFile(bstr);
            SysFreeString(bstr);
            SnapFrameToVideo();
            return 0;
            break;
        case ID_CONTROL_SOURCE_ARRAY:
            // set the source array 
            FreeSource();
            SetSourceArray();
            SnapFrameToVideo();
            return 0;
            break;
        case ID_CONTROL_SOURCE_HGLOBAL:
            // set the source hglobal 
            FreeSource();
            SetSourceHGlobal();
            SnapFrameToVideo();
            return 0;
            break;
        case ID_CONTROL_MEDIAINFORMATION:
            // display available media information 
            p = sz;
            g_pPlay->get_Title(&bstr);
            p += _stprintf(p, _T("Title = '%ls'"), bstr ? bstr : L"");
            SysFreeString(bstr);
            g_pPlay->get_Author(&bstr);
            p += _stprintf(p, _T(", Author = '%ls'"), bstr ? bstr : L"");
            SysFreeString(bstr);
            g_pPlay->get_Copyright(&bstr);
            p += _stprintf(p, _T(", Copyright = '%ls'"), bstr ? bstr : L"");
            SysFreeString(bstr);
            g_pPlay->get_Description(&bstr);
            p += _stprintf(p, _T(", Description = '%ls'"), bstr ? bstr : L"");
            SysFreeString(bstr);
            g_pPlay->get_Rating(&bstr);
            p += _stprintf(p, _T(", Rating = '%ls'"), bstr ? bstr : L"");
            SysFreeString(bstr);
            MessageBox(hwnd, sz, _T("Media Information"), MB_OK);
            return 0;
            break;
        }
        break;
    case WM_LBUTTONDOWN:
        // perform play/pause if the user clicks in the video window 
        g_pPlay->get_VideoWindowLeft(&x);
        g_pPlay->get_VideoWindowTop(&y);
        g_pPlay->get_VideoWindowWidth(&cx);
        g_pPlay->get_VideoWindowHeight(&cy);
        SetRect(&rc, x, y, x + cx, y + cy);
        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        if (PtInRect(&rc, pt))
        {
            g_pPlay->get_State(&l);
            if (l == ltmmPlay_State_Stopped || l == ltmmPlay_State_Paused)
                g_pPlay->Run();
            else if (l == ltmmPlay_State_Running)
                g_pPlay->Pause();
        }      return 0;
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    MSG msg;
    HRESULT hr;
    WNDCLASSEX wcex;
    g_hInstance = hInstance;

    // initialize COM library 
    hr = CoInitialize(NULL);
    if (FAILED(hr))
        goto error;

    // register the video frame window class 
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PlayWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = g_hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wcex.lpszMenuName = (LPCWSTR)IDR_MENU;
    wcex.lpszClassName = SZ_WNDCLASS_PLAY;
    wcex.hIconSm = NULL;
    if (!RegisterClassEx(&wcex))
        goto error;

    // create the play object 
    hr = CoCreateInstance(CLSID_ltmmPlay, NULL, CLSCTX_INPROC_SERVER, IID_IltmmPlay, (void**)&g_pPlay);
    if (FAILED(hr))
        goto error;

    // create the video frame window 
    if (!CreateWindow(SZ_WNDCLASS_PLAY, _T("Play"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, g_hInstance, NULL))
        goto error;

    ShowWindow(g_hwndPlay, nCmdShow);
    UpdateWindow(g_hwndPlay);

    // process until done 
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

error:
    if (g_pPlay)
        g_pPlay->Release();
    CoUninitialize();
    return 0;
}