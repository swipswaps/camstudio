#include "stdafx.h"
#include "Recorder.h"
#include "AudioFormatDlg.h"

#include "RecorderView.h"
#include "vfw/ACM.h"

#include <CamLib/AudioVolume.h>
#include <CamLib/CStudioLib.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if !defined(AVE_FORMAT_MPEGLAYER3)
#define WAVE_FORMAT_MPEGLAYER3 0x0055
#endif

CAudioFormatDlg::CAudioFormatDlg(CWnd *pParent /*=nullptr*/)
    : CDialog(CAudioFormatDlg::IDD, pParent)
    , m_pwfx(0)
    , m_cbwfx(0)
    , m_iAudioBitsPerSample(0)
    , m_iAudioNumChannels(0)
    , m_iAudioSamplesPerSeconds(0)
    , m_bAudioCompression(TRUE)
    , m_iNumFormat(0)
    , m_iNumDevice(0)
    , m_iInterleavePeriod(0)
    , m_devicemap()
{
    //{{AFX_DATA_INIT(CAudioFormatDlg)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    AllocCompressFormat();
    UpdateCompressFormatInterface();
}

CAudioFormatDlg::CAudioFormatDlg(const sAudioFormat &cFmt, CWnd *pParent)
    : CDialog(CAudioFormatDlg::IDD, pParent)
    , m_cFmt(cFmt)
    , m_pwfx(0)
    , m_cbwfx(0)
    , m_iAudioBitsPerSample(cFmt.m_iBitsPerSample)
    , m_iAudioNumChannels(cFmt.m_iNumChannels)
    , m_iAudioSamplesPerSeconds(cFmt.m_iSamplesPerSeconds)
    , m_bAudioCompression(cFmt.m_bCompression)
    , m_iNumFormat(0)
    , m_iNumDevice(0)
    , m_iInterleavePeriod(cFmt.m_iInterleavePeriod)
    , m_devicemap()
{
}

void CAudioFormatDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAudioFormatDlg)
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_IFACTOR, m_ctrlEditFactor);
    DDX_Control(pDX, IDC_INTERLEAVE, m_ctrlButtonInterleave);
    DDX_Control(pDX, IDC_INTERLEAVEFRAMES, m_ctrlButtonInterleaveFrames);
    DDX_Control(pDX, IDC_INTERLEAVESECONDS, m_ctrlButtonInterleaveSeconds);
    DDX_Control(pDX, IDC_RECORDFORMAT, m_ctrlCBRecordFormat);
    DDX_Control(pDX, IDC_INPUTDEVICE, m_ctrlCBInputDevice);
    DDX_Control(pDX, IDC_SYSTEMRECORD, m_ctrlButtonSystemRecord);
    DDX_Control(pDX, IDC_COMPRESSEDFORMATTAG, m_ctrlEditCompressedFormatTag);
    DDX_Control(pDX, IDC_COMPRESSEDFORMAT, m_ctrlEditCompressedFormat);
    DDX_Control(pDX, IDC_CHOOSE_COMPRESSED_FORMAT, m_ctrlButtonChooseCompressedFormat);
    DDX_Text(pDX, IDC_IFACTOR, m_iInterleavePeriod);
    DDV_MinMaxInt(pDX, m_iInterleavePeriod, 0, 1000);
}

BEGIN_MESSAGE_MAP(CAudioFormatDlg, CDialog)
//{{AFX_MSG_MAP(CAudioFormatDlg)
ON_BN_CLICKED(IDC_CHOOSE_COMPRESSED_FORMAT, OnSelectCompression)
ON_CBN_SELCHANGE(IDC_RECORDFORMAT, OnSelchangeRecordformat)
ON_BN_CLICKED(IDC_INTERLEAVE, OnInterleave)
ON_BN_CLICKED(IDVOLUME, OnVolume)
ON_CBN_SELCHANGE(IDC_INPUTDEVICE, OnSelchangeInputdevice)
ON_BN_CLICKED(IDC_INTERLEAVEFRAMES, OnInterleaveframes)
ON_BN_CLICKED(IDC_INTERLEAVESECONDS, OnInterleaveseconds)
ON_BN_CLICKED(IDC_SYSTEMRECORD, OnSystemrecord)
ON_BN_CLICKED(IDC_BUTTON1, OnHelp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()



void CAudioFormatDlg::AllocCompressFormat()
{
    CACM acm;
    if (m_pwfx)
    {
        // Do nothing....already allocated
        // verify size
        DWORD cbwfx = 0;
        ;
        VERIFY(0 == acm.MaxFormatSize(cbwfx));
        ASSERT(cbwfx == m_cbwfx);
        ASSERT(m_pwfx->cbSize <= (m_cbwfx - sizeof(WAVEFORMATEX)));
        return;
    }
    MMRESULT mmresult = acm.MaxFormatSize(m_cbwfx);
    if (MMSYSERR_NOERROR != mmresult)
    {
        CString msgstr;
        CString tstr;
        tstr.LoadString(IDS_STRING_NOTE);
        msgstr.Format(_T("Metrics failed mmresult=%u!"), mmresult);
        ::MessageBox(nullptr, msgstr, tstr, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    if (m_cbwfx < m_cFmt.m_dwCbwFX)
        m_cbwfx = m_cFmt.m_dwCbwFX;
    ASSERT(sizeof(WAVEFORMATEX) <= m_cbwfx);
    m_pwfx = reinterpret_cast<LPWAVEFORMATEX>(new char[m_cbwfx]);
    if (nullptr == m_pwfx)
    {
        CString msgstr;
        CString tstr;
        tstr.LoadString(IDS_STRING_NOTE);
        msgstr.Format(_T("GlobalAllocPtr(%lu) failed!"), m_cbwfx);
        ::MessageBox(nullptr, msgstr, tstr, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    // Prevent C4244 warning and take some predictions for unwanted truncations
    // m_pwfx->cbSize = (m_cbwfx - sizeof(WAVEFORMATEX));        -> C4244
    DWORD tmpDWord = (m_cbwfx - sizeof(WAVEFORMATEX));
    if (tmpDWord > static_cast<WORD>(tmpDWord))
    {
        TRACE("Casted value [%i] is not the same as original [%l] ..!\n", static_cast<WORD>(tmpDWord), tmpDWord);
    }
    m_pwfx->cbSize = static_cast<WORD>(tmpDWord);

    ASSERT(m_pwfx->cbSize <= (m_cbwfx - sizeof(WAVEFORMATEX)));
}

void CAudioFormatDlg::BuildLocalRecordingFormat(WAVEFORMATEX &rsWaveFormat)
{
    rsWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    rsWaveFormat.wBitsPerSample = static_cast<WORD>(m_iAudioBitsPerSample);
    rsWaveFormat.nSamplesPerSec = m_iAudioSamplesPerSeconds;
    rsWaveFormat.nChannels = static_cast<WORD>(m_iAudioNumChannels);
    rsWaveFormat.nBlockAlign = rsWaveFormat.nChannels * (rsWaveFormat.wBitsPerSample / 8);
    rsWaveFormat.nAvgBytesPerSec = rsWaveFormat.nSamplesPerSec * rsWaveFormat.nBlockAlign;
    rsWaveFormat.cbSize = 0;
}

void CAudioFormatDlg::SuggestLocalCompressFormat()
{
    m_bAudioCompression = TRUE;
    AllocCompressFormat();

    // 1st try MPEGLAYER3
    // TODO: Why????
    WAVEFORMATEX sWaveFormat;
    BuildLocalRecordingFormat(sWaveFormat);
    MMRESULT mmr = MMSYSERR_ERROR;
    if ((sWaveFormat.nSamplesPerSec == 22050) && (sWaveFormat.nChannels == 2) && (sWaveFormat.wBitsPerSample <= 16))
    {
        m_pwfx->wFormatTag = WAVE_FORMAT_MPEGLAYER3;
        mmr = ::acmFormatSuggest(nullptr, &sWaveFormat, m_pwfx, m_cbwfx, ACM_FORMATSUGGESTF_WFORMATTAG);
    }

    if (0 != mmr)
    {
        // ver 1.6, use PCM if MP3 not available

        // Then try ADPCM
        // BuildLocalRecordingFormat();
        // m_pwfx->wFormatTag = WAVE_FORMAT_ADPCM;
        // mmr = acmFormatSuggest(nullptr, &m_sWaveFormat, m_pwfx, m_cbwfx, ACM_FORMATSUGGESTF_WFORMATTAG);
        // if (mmr != 0) {
        // Use the PCM as default
        BuildLocalRecordingFormat(sWaveFormat);
        m_pwfx->wFormatTag = WAVE_FORMAT_PCM;
        mmr = acmFormatSuggest(nullptr, &sWaveFormat, m_pwfx, m_cbwfx, ACM_FORMATSUGGESTF_WFORMATTAG);
        if (mmr != 0)
        {
            m_bAudioCompression = FALSE;
        }
        //}
    }
}

BOOL CAudioFormatDlg::GetFormatDescription(CString &rstrFormatTag, CString &rstrFormat)
{
    // Retrieve the descriptive name for the FormatTag in pwformat.
    ACMFORMATTAGDETAILS aftd;
    ::ZeroMemory(&aftd, sizeof(ACMFORMATTAGDETAILS));
    // Fill in the required members FormatTAG query.
    aftd.cbStruct = sizeof(ACMFORMATTAGDETAILS);
    aftd.dwFormatTag = m_pwfx->wFormatTag;

    // Ask ACM to find first available driver that supports the specified Format tag.
    MMRESULT mmr = ::acmFormatTagDetails(nullptr, &aftd, ACM_FORMATTAGDETAILSF_FORMATTAG);
    if (MMSYSERR_NOERROR != mmr)
    {
        return FALSE;
    }

    // Copy the Format tag name into the callier's buffer.
    rstrFormatTag = aftd.szFormatTag;

    CString strSamplesPerSecond;
    CString strBitsPerSample;
    CString strAvgBytesPerSecond;
    CString strChannels;

    strSamplesPerSecond.Format(_T("%d Hz"), m_pwfx->nSamplesPerSec);
    strBitsPerSample.Format(_T("%d Bit"), m_pwfx->wBitsPerSample);
    strAvgBytesPerSecond.Format(_T("%d Bytes/sec"), m_pwfx->nAvgBytesPerSec);
    strChannels.Format(_T("%s"), (1 == m_pwfx->nChannels) ? _T("Mono") : _T("Stereo"));

    CString formatstr;
    formatstr = strSamplesPerSecond + ", ";
    if (0 < m_pwfx->wBitsPerSample)
        formatstr = formatstr + strBitsPerSample + ", ";
    formatstr = formatstr + strChannels + " " + strAvgBytesPerSecond;
    rstrFormat = formatstr;

    return TRUE;
}

void CAudioFormatDlg::UpdateCompressFormatInterface()
{
    if (!m_bAudioCompression)
    {
        m_ctrlEditCompressedFormatTag.SetWindowText(_T("None Available"));
        m_ctrlEditCompressedFormat.SetWindowText(_T(" "));
        return;
    }

    CString strFormat("");
    CString strFormatTag("");
    BOOL res = GetFormatDescription(strFormatTag, strFormat);
    if (res)
    {
        m_ctrlEditCompressedFormatTag.SetWindowText(strFormatTag);
        m_ctrlEditCompressedFormat.SetWindowText(strFormat);
    }
}

// UpdateDeviceData
// Update the user - interface based on the device data
//
// If the third parameter (compressed format) is not null,
// we assume it is compatible with the 2nd parameter (recording format)
void CAudioFormatDlg::UpdateDeviceData(UINT /*deviceID*/, DWORD dwFormat, const WAVEFORMATEX &rwfx)
{
    // WAVEINCAPS wic;
    // MMRESULT mmr = ::waveInGetDevCaps(deviceID , &wic, sizeof(WAVEINCAPS));

    // Reset Recording Format Combo Box and format map
    //    LoadFormatList();

    if (m_iNumFormat <= 0)
    {
        m_ctrlCBRecordFormat.AddString(_T("None Available"));
        m_ctrlCBRecordFormat.SetCurSel(0);
        m_ctrlEditCompressedFormatTag.SetWindowText(_T("None Available"));
        m_ctrlEditCompressedFormat.SetWindowText(_T(" "));
        m_ctrlButtonChooseCompressedFormat.EnableWindow(FALSE);
        // For this case, where no recording format, compressed format is available
        // is handled by OnOk (no external formats is updated) when the user press
        // the OK button.
        return;
    }
    m_ctrlButtonChooseCompressedFormat.EnableWindow(TRUE);

    int selected_cindex = -1; // selected index of combo box
    for (int i = 0; i < m_iNumFormat; ++i)
    {
        if (dwFormat == m_vFormat[i].first)
        {
            selected_cindex = i;
            break;
        }
    }

    // If can reach here ==> m_iNumFormat > 0
    if ((-1 == selected_cindex) && (0 < m_iNumFormat))
    { // selected recording format not found
        // force selection to one that is compatible
        m_ctrlCBRecordFormat.SetCurSel(0);
        // force selection of compress format
        OnSelchangeRecordformat();
    }
    else
    {
        // Compressed or Save format
        AllocCompressFormat();
        // if (!pwfx) {
        //    SuggestLocalCompressFormat();
        //} else {
        ASSERT(m_pwfx);
        // ASSERT(pwfx);
        // if (pwfx != m_pwfx) {
        // copy format; copies the largest format block
        ::CopyMemory(m_pwfx, &rwfx, m_cFmt.m_dwCbwFX);
        //}
        //}
        OnSelchangeRecordformat();
        UpdateCompressFormatInterface();

        // will this following line call OnSelchangeRecordformat() ?
        if (0 <= selected_cindex)
        {
            m_ctrlCBRecordFormat.SetCurSel(selected_cindex);
        }
    }
}

// fill the combo box with <device name, uDeviceID> values
// the uDeviceID is the value used by waveInGetDevCaps
bool CAudioFormatDlg::LoadDeviceList()
{
    m_iNumDevice = 0;
    m_ctrlCBInputDevice.ResetContent();

    int iIdx = m_ctrlCBInputDevice.AddString(_T("Default input device"));
    m_ctrlCBInputDevice.SetItemData(iIdx, WAVE_MAPPER);
    m_devicemap[m_iNumDevice++] = WAVE_MAPPER;

    int numdevs = ::waveInGetNumDevs();
    for (int i = 0; i < numdevs; i++)
    {
        WAVEINCAPS wicaps;
        MMRESULT mmr = ::waveInGetDevCaps(i, &wicaps, sizeof(WAVEINCAPS));
        if (MMSYSERR_NOERROR == mmr)
        {
            iIdx = m_ctrlCBInputDevice.AddString(wicaps.szPname);
            m_ctrlCBInputDevice.SetItemData(iIdx, i);
            // As i is always a positive value we can cast to UINT
            if (m_cFmt.m_uDeviceID == (UINT)i)
            {
                m_ctrlCBInputDevice.SetCurSel(iIdx);
            }
            // TODO: should be obsolete
            m_devicemap[m_iNumDevice] = i;
            m_iNumDevice++;
        }
    }

    // if (CB_ERR == m_ctrlCBInputDevice.GetCurSel()) {
    m_ctrlCBInputDevice.SetCurSel(0);
    //}

    return true;
}

bool CAudioFormatDlg::FillFormatList()
{
    m_vFormat.clear();
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_1M08, _T("11.025 kHz, mono, 8-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_1M16, _T("11.025 kHz, mono, 16-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_1S08, _T("11.025 kHz, stereo, 8-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_1S16, _T("11.025 kHz, stereo, 16-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_2M08, _T("22.05 kHz, mono, 8-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_2M16, _T("22.05 kHz, mono, 16-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_2S08, _T("22.05 kHz, stereo, 8-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_2S16, _T("22.05 kHz, stereo, 16-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_4M08, _T("44.1 kHz, mono, 8-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_4M16, _T("44.1 kHz, mono, 16-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_4S08, _T("44.1 kHz, stereo, 8-bit")));
    m_vFormat.push_back(pairIDFormat(WAVE_FORMAT_4S16, _T("44.1 kHz, stereo, 16-bit")));

    return true;
}

bool CAudioFormatDlg::LoadFormatList()
{
    FillFormatList();
    m_ctrlCBRecordFormat.ResetContent();
    m_iNumFormat = 0;
    for (std::vector<pairIDFormat>::iterator iter = m_vFormat.begin(); iter != m_vFormat.end(); ++iter)
    {
        int iIdx = m_ctrlCBRecordFormat.AddString(iter->second);
        m_ctrlCBRecordFormat.SetItemData(iIdx, iter->first);
        if (iter->first == m_cFmt.m_dwWaveinSelected)
        {
            m_ctrlCBRecordFormat.SetCurSel(iIdx);
        }
    }
    m_iNumFormat = static_cast<int>(m_vFormat.size());

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// CAudioFormatDlg message handlers

BOOL CAudioFormatDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Interleave
    m_ctrlButtonInterleave.SetCheck(m_cFmt.m_bInterleaveFrames);
    m_iInterleavePeriod = m_cFmt.m_iInterleaveFactor;

    m_ctrlEditFactor.EnableWindow(m_cFmt.m_bInterleaveFrames);
    m_ctrlButtonInterleaveFrames.EnableWindow(m_cFmt.m_bInterleaveFrames);
    m_ctrlButtonInterleaveSeconds.EnableWindow(m_cFmt.m_bInterleaveFrames);

    m_ctrlButtonInterleaveFrames.SetCheck(m_cFmt.m_iInterleavePeriod == FRAMES);
    m_ctrlButtonInterleaveSeconds.SetCheck(m_cFmt.m_iInterleavePeriod == MILLISECONDS);

    m_iAudioBitsPerSample = m_cFmt.m_iBitsPerSample;
    m_iAudioNumChannels = m_cFmt.m_iNumChannels;
    m_iAudioSamplesPerSeconds = m_cFmt.m_iSamplesPerSeconds;

    m_bAudioCompression = m_cFmt.m_bCompression;

    // Ver 1.2

    // Generate device list
    LoadDeviceList();

    ////Select the device combo box
    // int deviceIsSelected = 0;
    // int selectedDevice = WAVE_MAPPER;
    // for (int i = 0; i < m_iNumDevice; ++i) {
    //    if (m_cFmt.m_uDeviceID == m_devicemap[i]) {
    //        m_ctrlCBInputDevice.SetCurSel(i);
    //        selectedDevice = m_devicemap[i];
    //        deviceIsSelected = 1;
    //    }
    //}
    // if (!deviceIsSelected) {
    //    if (m_iNumDevice) {
    //        m_ctrlCBInputDevice.SetCurSel(0);
    //    }
    //}

    LoadFormatList();

    // Ver 1.2
    WAVEINCAPS pwic;
    MMRESULT mmr = ::waveInGetDevCaps(m_cFmt.m_uDeviceID, &pwic, sizeof(pwic));
    if (MMSYSERR_NOERROR != mmr)
    {
        // TODO: handle error
    }

    // This code works on the assumption (when filling in values for the user - interfaces)
    // that the m_Format and pwfx formats (external variables) are already chosen correctly
    // and compatible with each other
    int devID = m_ctrlCBInputDevice.GetCurSel();
    if (CB_ERR != devID)
    {
        const auto device_id = static_cast<UINT>(m_ctrlCBInputDevice.GetItemData(devID));
        UpdateDeviceData(device_id, m_cFmt.m_dwWaveinSelected, m_cFmt.AudioFormat());
    }

    // ver 1.8
    m_ctrlButtonSystemRecord.SetCheck(m_cFmt.m_bUseMCI);
    // UpdateCompressFormatInterface();
    UpdateData(FALSE);

    return TRUE; // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CAudioFormatDlg::OnOK()
{
    UpdateData();
    m_cFmt.m_iInterleaveFactor = m_iInterleavePeriod;
    m_cFmt.m_bInterleaveFrames = m_ctrlButtonInterleave.GetCheck() ? true : false;
    m_cFmt.m_iInterleavePeriod = (m_ctrlButtonInterleaveFrames.GetCheck()) ? FRAMES : MILLISECONDS;

    // The Recording format, Compressed format and device must be valid before
    // data from the Audio Options Dialog can be updated to the external variables
    if (0 < m_iNumFormat)
    {
        int sel = m_ctrlCBRecordFormat.GetCurSel();
        if (0 <= sel)
        {
            if (m_pwfx)
            {
                // Ver 1.2
                int getdevice = m_ctrlCBInputDevice.GetCurSel();
                if (getdevice < m_iNumDevice)
                {
                    if (m_cbwfx <= m_cFmt.m_dwCbwFX)
                    { // All checks cleared, update external values
                        // Updating to external variables
                        m_cFmt.m_uDeviceID = m_devicemap[getdevice];
                        m_cFmt.m_bCompression = m_bAudioCompression ? true : false;
                        m_cFmt.m_iBitsPerSample = m_iAudioBitsPerSample;
                        m_cFmt.m_iNumChannels = m_iAudioNumChannels;
                        m_cFmt.m_iSamplesPerSeconds = m_iAudioSamplesPerSeconds;

                        // Update the external pwfx (compressed format);
                        m_cFmt.m_dwWaveinSelected = m_vFormat[sel].first;
                        m_cFmt.m_dwCbwFX = m_cbwfx;
                        m_cFmt.WriteAudio(m_pwfx);
                    }
                }
            }
        }
    }

    if (m_pwfx)
    {

        // GlobalFreePtr(m_pwfx);
        delete[] m_pwfx;
        m_pwfx = nullptr;
    }

    m_cFmt.m_bUseMCI = m_ctrlButtonSystemRecord.GetCheck() ? true : false;

    CDialog::OnOK();
}

void CAudioFormatDlg::OnSelectCompression()
{
    if (m_pwfx == nullptr)
    {
        SuggestLocalCompressFormat();
    }
    // TEST
    // WAVEFORMATEX sWaveFormatEx;
    //::ZeroMemory(&sWaveFormatEx, sizeof(WAVEFORMATEX));
    // BuildLocalRecordingFormat(sWaveFormatEx);
    // sWaveFormatEx.wFormatTag = m_pwfx->wFormatTag;
    // TEST

    TRACE(_T("\nWAVEFORMAT:"));
    TRACE(_T("\nwFormatTag = %d"), m_pwfx->wFormatTag);
    TRACE(_T("\nnChannels = %d"), m_pwfx->nChannels);
    TRACE(_T("\nnSamplesPerSec = %d"), m_pwfx->nSamplesPerSec);
    TRACE(_T("\nnAvgBytesPerSec = %d"), m_pwfx->nAvgBytesPerSec);
    TRACE(_T("\nnBlockAlign = %d"), m_pwfx->nBlockAlign);
    TRACE(_T("\ncbSize = %d\n"), m_pwfx->cbSize);

    // initialize the ACMFORMATCHOOSE members
    ACMFORMATCHOOSE acmfc;
    ::ZeroMemory(&acmfc, sizeof(ACMFORMATCHOOSE));
    acmfc.cbStruct = sizeof(ACMFORMATCHOOSE);
    acmfc.hwndOwner = m_hWnd;
    acmfc.pwfx = m_pwfx;
    acmfc.fdwStyle = ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;
    acmfc.cbwfx = m_cbwfx;
    acmfc.pszTitle = _T("Audio Compression Format");
    acmfc.szFormatTag[0] = '\0';
    acmfc.szFormat[0] = '\0';
    acmfc.pszName = nullptr;
    acmfc.cchName = 0;
    acmfc.fdwEnum = 0;
    acmfc.pwfxEnum = m_pwfx;
    // acmfc.pwfxEnum = &sWaveFormatEx;
    acmfc.hInstance = nullptr;
    acmfc.pszTemplateName = nullptr;
    acmfc.lCustData = 0L;
    acmfc.pfnHook = nullptr;

    // Valid formats for saving
    // BUG: loses current settings
    // BuildLocalRecordingFormat();
    // acmfc.pwfxEnum = &m_sWaveFormat;
    acmfc.fdwEnum = ACM_FORMATENUMF_SUGGEST;

    // acmfc.fdwEnum |= ACM_FORMATENUMF_WFORMATTAG;

    MMRESULT mmresult = ::acmFormatChoose(&acmfc);
    if (MMSYSERR_NOERROR != mmresult)
    {
        if (ACMERR_CANCELED != mmresult)
        {
            switch (mmresult)
            {
                case ACMERR_NOTPOSSIBLE:
                    TRACE(_T("ACMERR_NOTPOSSIBLE\n"));
                    break;
                case MMSYSERR_INVALFLAG:
                    TRACE(_T("MMSYSERR_INVALFLAG\n"));
                    break;
                case MMSYSERR_INVALHANDLE:
                    TRACE(_T("MMSYSERR_INVALHANDLE\n"));
                    break;
                case MMSYSERR_INVALPARAM:
                    TRACE(_T("MMSYSERR_INVALPARAM\n"));
                    break;
                case MMSYSERR_NODRIVER:
                    TRACE(_T("MMSYSERR_NODRIVER\n"));
                    break;
                default:
                    TRACE(_T("Unknown\n"));
                    break;
            }
            CString msgstr;
            CString tstr;
            tstr.LoadString(IDS_STRING_NOTE);
            msgstr.Format(_T("FormatChoose() failed with error = %u!"), mmresult);
            MessageBox(msgstr, tstr, MB_OK | MB_ICONEXCLAMATION);
        }
        return;
    }

    TRACE(_T("\nWAVEFORMAT:"));
    TRACE(_T("\nwFormatTag = %d"), m_pwfx->wFormatTag);
    TRACE(_T("\nnChannels = %d"), m_pwfx->nChannels);
    TRACE(_T("\nnSamplesPerSec = %d"), m_pwfx->nSamplesPerSec);
    TRACE(_T("\nnAvgBytesPerSec = %d"), m_pwfx->nAvgBytesPerSec);
    TRACE(_T("\nnBlockAlign = %d"), m_pwfx->nBlockAlign);
    TRACE(_T("\ncbSize = %d\n"), m_pwfx->cbSize);
    SuggestLocalCompressFormat();
    UpdateCompressFormatInterface();
}

void CAudioFormatDlg::OnSelchangeRecordformat()
{
    if (m_iNumFormat <= 0)
    {
        return; // no format to choose from
    }

    int sel = m_ctrlCBRecordFormat.GetCurSel();
    if (sel < 0)
    {
        return;
    }
    switch (m_vFormat[sel].first)
    {
        case WAVE_FORMAT_1M08:
            m_iAudioBitsPerSample = 8;
            m_iAudioNumChannels = 1;
            m_iAudioSamplesPerSeconds = 11025;
            break;
        case WAVE_FORMAT_1M16:
            m_iAudioBitsPerSample = 16;
            m_iAudioNumChannels = 1;
            m_iAudioSamplesPerSeconds = 11025;
            break;
        case WAVE_FORMAT_1S08:
            m_iAudioBitsPerSample = 8;
            m_iAudioNumChannels = 2;
            m_iAudioSamplesPerSeconds = 11025;
            break;
        case WAVE_FORMAT_1S16:
            m_iAudioBitsPerSample = 16;
            m_iAudioNumChannels = 2;
            m_iAudioSamplesPerSeconds = 11025;
            break;
        case WAVE_FORMAT_2M08:
            m_iAudioBitsPerSample = 8;
            m_iAudioNumChannels = 1;
            m_iAudioSamplesPerSeconds = 22050;
            break;
        case WAVE_FORMAT_2M16:
            m_iAudioBitsPerSample = 16;
            m_iAudioNumChannels = 1;
            m_iAudioSamplesPerSeconds = 22050;
            break;
        case WAVE_FORMAT_2S08:
            m_iAudioBitsPerSample = 8;
            m_iAudioNumChannels = 2;
            m_iAudioSamplesPerSeconds = 22050;
            break;
        case WAVE_FORMAT_2S16:
            m_iAudioBitsPerSample = 16;
            m_iAudioNumChannels = 2;
            m_iAudioSamplesPerSeconds = 22050;
            break;
        case WAVE_FORMAT_4M08:
            m_iAudioBitsPerSample = 8;
            m_iAudioNumChannels = 1;
            m_iAudioSamplesPerSeconds = 44100;
            break;
        case WAVE_FORMAT_4M16:
            m_iAudioBitsPerSample = 16;
            m_iAudioNumChannels = 1;
            m_iAudioSamplesPerSeconds = 44100;
            break;
        case WAVE_FORMAT_4S08:
            m_iAudioBitsPerSample = 8;
            m_iAudioNumChannels = 2;
            m_iAudioSamplesPerSeconds = 44100;
            break;
        case WAVE_FORMAT_4S16:
            m_iAudioBitsPerSample = 16;
            m_iAudioNumChannels = 2;
            m_iAudioSamplesPerSeconds = 44100;
            break;
    }

    // BuildLocalRecordingFormat();
    SuggestLocalCompressFormat();
    UpdateCompressFormatInterface();
}

void CAudioFormatDlg::OnInterleave()
{
    BOOL binteleave = m_ctrlButtonInterleave.GetCheck();
    if (binteleave)
    {
        m_ctrlEditFactor.EnableWindow(TRUE);
        m_ctrlButtonInterleaveFrames.EnableWindow(TRUE);
        m_ctrlButtonInterleaveSeconds.EnableWindow(TRUE);
    }
    else
    {
        m_ctrlEditFactor.EnableWindow(FALSE);
        m_ctrlButtonInterleaveFrames.EnableWindow(FALSE);
        m_ctrlButtonInterleaveSeconds.EnableWindow(FALSE);
    }
}

void CAudioFormatDlg::OnCancel()
{
    if (m_pwfx)
    {
        // GlobalFreePtr(m_pwfx);
        delete[] m_pwfx;
        m_pwfx = nullptr;
    }

    CDialog::OnCancel();
}

void CAudioFormatDlg::OnVolume()
{
    OnAudioVolume(m_hWnd);
}

void CAudioFormatDlg::OnSelchangeInputdevice()
{
    int devID = m_ctrlCBInputDevice.GetCurSel();
    if (devID < m_iNumDevice)
    {
        // UpdateDeviceData(m_devicemap[devID], m_cFmt.m_dwWaveinSelected, nullptr);
        UpdateDeviceData(m_devicemap[devID], m_cFmt.m_dwWaveinSelected, m_cFmt.AudioFormat());
    }
}

void CAudioFormatDlg::OnInterleaveframes()
{
    m_ctrlButtonInterleaveFrames.SetCheck(TRUE);
    m_ctrlButtonInterleaveSeconds.SetCheck(FALSE);
}

void CAudioFormatDlg::OnInterleaveseconds()
{
    m_ctrlButtonInterleaveFrames.SetCheck(FALSE);
    m_ctrlButtonInterleaveSeconds.SetCheck(TRUE);
}

void CAudioFormatDlg::OnSystemrecord()
{
    // TODO: Add your control notification handler code here
}

void CAudioFormatDlg::OnHelp()
{
    //CString progdir = GetProgPath();
    //CString helppath = progdir + _T("\\help.htm#Helpmci");
    //Openlink(helppath);
}
