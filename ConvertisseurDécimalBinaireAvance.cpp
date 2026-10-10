// ConvertisseurDécimalBinaireAvance.cpp
// Convertisseur Decimal <-> Binaire (Unsigned, Sign Magnitude, Cpl1, Cpl2)
// Avec limitation automatique et indication du nombre de bits requis
// Patrice Waechter-Ebling 2026

#include <windows.h>
#include <string>
#include <sstream>

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma warning (disable:4267)
// Largeur en bits
int g_nBits = 8;

// IDs des contrôles
#define IDC_EDIT_INPUT   1001
#define IDC_EDIT_OUTPUT  1002
#define IDC_BUTTON_CONV  1003
#define IDC_COMBO_MODE   1004
#define IDC_COMBO_DIR    1005
#define IDC_EDIT_BITS    1006

enum ConvMode
{
    MODE_UNSIGNED = 0,
    MODE_SIGN_MAG = 1,
    MODE_ONES_COMP = 2,
    MODE_TWOS_COMP = 3
};

enum ConvDir
{
    DIR_DEC_TO_BIN = 0,
    DIR_BIN_TO_DEC = 1
};

HINSTANCE g_hInst;
HWND g_hWndMain;
HWND g_hEditInput;
HWND g_hEditOutput;
HWND g_hButtonConv;
HWND g_hComboMode;
HWND g_hComboDir;
HWND g_hEditBits;

static std::string Trimmer(const std::string& s)
{
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t')) a++;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
    return s.substr(a, b - a);
}
static bool FormatBinaire(const std::string& s)
{
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] != '0' && s[i] != '1') return false;
    return true;
}
static int CalculerNombreBitsRequis(long value, ConvMode mode)
{
    if (mode == MODE_UNSIGNED)
    {
        if (value < 0) value = 0;
        int bits = 1;
        while ((1L << bits) <= value) bits++;
        return bits;
    }
    long mag = (value < 0) ? -value : value;
    int bitsMag = 1;
    while ((1L << bitsMag) <= mag) bitsMag++;
    return bitsMag + 1; // + bit de signe
}
static bool VerifierSignePlage(long value, ConvMode mode, int bits)
{
    if (mode == MODE_UNSIGNED){long maxVal = (1L << bits) - 1;return (value >= 0 && value <= maxVal);}
    long maxMag = (1L << (bits - 1)) - 1;
    switch (mode)
    {
        case MODE_SIGN_MAG:
        case MODE_ONES_COMP:return (value >= -maxMag && value <= maxMag);
        case MODE_TWOS_COMP:
        {
            long maxPos = maxMag;
            long minNeg = -(1L << (bits - 1));
            return (value >= minNeg && value <= maxPos);
        }
    }
    return false;
}
static std::string DecimalEnBinaireNonSigne(long value, int bits)
{
    if (value < 0) value = 0;
    long maxVal = (1L << bits) - 1;
    if (value > maxVal) value = maxVal;
    std::string r(bits, '0');
    for (int i = bits - 1; i >= 0; i--){ r[i] = (value & 1) ? '1' : '0'; value >>= 1;    }
    return r;
}
static long BinToDecUnsigned(const std::string& bin, int bits)
{
    long v = 0;
    int len = bin.size();
    int start = (len > bits) ? (len - bits) : 0;
    for (int i = start; i < len; i++){v <<= 1; if (bin[i] == '1') v |= 1;}
    return v;
}
static std::string DecToBinSignMagnitude(long value, int bits)
{
    bool neg = (value < 0);
    long mag = neg ? -value : value;
    long maxMag = (1L << (bits - 1)) - 1;
    if (mag > maxMag) mag = maxMag;
    std::string r(bits, '0');
    r[0] = neg ? '1' : '0';
    for (int i = bits - 1; i >= 1; i--){ r[i] = (mag & 1) ? '1' : '0'; mag >>= 1; }
    return r;
}
static long BinToDecSignMagnitude(const std::string& bin, int bits)
{
    int len = bin.size();
    int start = (len > bits) ? (len - bits) : 0;
    bool neg = (bin[start] == '1');
    long mag = 0;
    for (int i = start + 1; i < len; i++){ mag <<= 1; if (bin[i] == '1') mag |= 1; }
    return neg ? -mag : mag;
}
static std::string DecToBinOnesComplement(long value, int bits)
{
    bool neg = (value < 0);
    long mag = neg ? -value : value;
    long maxMag = (1L << (bits - 1)) - 1;
    if (mag > maxMag) mag = maxMag;
    std::string r(bits, '0');
    if (!neg)
    {
        for (int i = bits - 1; i >= 0; i--){ r[i] = (mag & 1) ? '1' : '0'; mag >>= 1; }
    }
    else
    {
        std::string pos(bits, '0');
		for (int i = bits - 1; i >= 0; i--){ pos[i] = (mag & 1) ? '1' : '0'; mag >>= 1; }
        for (int i = 0; i < bits; i++) r[i] = (pos[i] == '1') ? '0' : '1';
    }
    return r;
}

static long BinToDecOnesComplement(const std::string& bin, int bits)
{
    int len = bin.size();
    int start = (len > bits) ? (len - bits) : 0;

    bool neg = (bin[start] == '1');
    long v = 0;

    if (!neg)
    {
        for (int i = start; i < len; i++){ v <<= 1; if (bin[i] == '1') v |= 1; }
    }
    else
    {
        std::string pos(bits, '0');
        int idx = 0;
        for (int i = start; i < len; i++, idx++)  pos[idx] = (bin[i] == '1') ? '0' : '1';
        for (int i = 0; i < bits; i++){ v <<= 1; if (pos[i] == '1') v |= 1;}
        v = -v;
    }
    return v;
}

// Complement a 2
static std::string DecToBinTwosComplement(long value, int bits)
{
    long maxPos = (1L << (bits - 1)) - 1;
    long minNeg = -(1L << (bits - 1));

    if (value > maxPos) value = maxPos;
    if (value < minNeg) value = minNeg;

    unsigned long u;
    if (value >= 0) u = (unsigned long)value;
    else u = (unsigned long)((1L << bits) + value);

    std::string r(bits, '0');
    for (int i = bits - 1; i >= 0; i--)
    {
        r[i] = (u & 1) ? '1' : '0';
        u >>= 1;
    }
    return r;
}

static long BinToDecTwosComplement(const std::string& bin, int bits)
{
    int len = bin.size();
    int start = (len > bits) ? (len - bits) : 0;
    unsigned long u = 0;
    for (int i = start; i < len; i++){u <<= 1;if (bin[i] == '1') u |= 1;}
    unsigned long sign = 1UL << (bits - 1);
    if (u & sign) return (long)(u - (1UL << bits));
    return (long)u;
}
static bool ConvertValue(ConvMode mode, ConvDir dir, const std::string& input, std::string& output)
{
    std::string in = Trimmer(input);
    if (in.empty()){output = "Entree vide";return false;}
    if (dir == DIR_DEC_TO_BIN)
    {
        long val = 0;
        std::istringstream iss(in);
        iss >> val;
        if (iss.fail()){output = "Entree decimale invalide";return false;}
        if (!VerifierSignePlage(val, mode, g_nBits))
        {
            int needed = CalculerNombreBitsRequis(val, mode);
            std::ostringstream oss;
            oss << "Impossible avec " << g_nBits << " bits. " << "Bits requis: " << needed;
            output = oss.str();
            return false;
        }
        switch (mode)
        {
			case MODE_UNSIGNED: output = DecimalEnBinaireNonSigne(val, g_nBits); break;
			case MODE_SIGN_MAG: output = DecToBinSignMagnitude(val, g_nBits); break;
			case MODE_ONES_COMP: output = DecToBinOnesComplement(val, g_nBits); break;
			case MODE_TWOS_COMP: output = DecToBinTwosComplement(val, g_nBits); break;
        }
        return true;
    }
    else
    {
        if (!FormatBinaire(in)){output = "Entree binaire invalide";return false;}
        if ((int)in.size() > g_nBits)
        {
            std::ostringstream oss;
            oss << "Trop de bits (" << in.size() << "). Limite: " << g_nBits;
            output = oss.str();
            return false;
        }
        long val = 0;
        switch (mode)
        {
			case MODE_UNSIGNED: val = BinToDecUnsigned(in, g_nBits); break;
			case MODE_SIGN_MAG: val = BinToDecSignMagnitude(in, g_nBits); break;
			case MODE_ONES_COMP: val = BinToDecOnesComplement(in, g_nBits); break;
			case MODE_TWOS_COMP: val = BinToDecTwosComplement(in, g_nBits); break;
        }
        std::ostringstream oss;
        oss << val;
        output = oss.str();
        return true;
    }
}
static void CreationControls(HWND hWnd)
{
    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CreateWindow("STATIC", "Mode:", WS_CHILD | WS_VISIBLE,10, 10, 60, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow("STATIC", "Direction:", WS_CHILD | WS_VISIBLE,10, 40, 60, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow("STATIC", "Bits:", WS_CHILD | WS_VISIBLE,10, 70, 60, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow("STATIC", "Entree:", WS_CHILD | WS_VISIBLE,10, 100, 60, 20, hWnd, NULL, g_hInst, NULL);
    CreateWindow("STATIC", "Sortie:", WS_CHILD | WS_VISIBLE,10, 130, 60, 20, hWnd, NULL, g_hInst, NULL);
    g_hComboMode = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", "",WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,80, 10, 250, 200,hWnd, (HMENU)IDC_COMBO_MODE, g_hInst, NULL);
    SendMessage(g_hComboMode, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessage(g_hComboMode, CB_ADDSTRING, 0, (LPARAM)"Non signe");
    SendMessage(g_hComboMode, CB_ADDSTRING, 0, (LPARAM)"Signe Magnitude");
    SendMessage(g_hComboMode, CB_ADDSTRING, 0, (LPARAM)"Complement a 1");
    SendMessage(g_hComboMode, CB_ADDSTRING, 0, (LPARAM)"Complement a 2");
    SendMessage(g_hComboMode, CB_SETCURSEL, 0, 0);
    g_hComboDir = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 80, 40, 250, 200,hWnd, (HMENU)IDC_COMBO_DIR, g_hInst, NULL);
    SendMessage(g_hComboDir, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessage(g_hComboDir, CB_ADDSTRING, 0, (LPARAM)"Decimal -> Binaire");
    SendMessage(g_hComboDir, CB_ADDSTRING, 0, (LPARAM)"Binaire -> Decimal");
    SendMessage(g_hComboDir, CB_SETCURSEL, 0, 0);
    g_hEditBits = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "8",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,80, 70, 50, 20,hWnd, (HMENU)IDC_EDIT_BITS, g_hInst, NULL);
    SendMessage(g_hEditBits, WM_SETFONT, (WPARAM)font, TRUE);
    g_hEditInput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,80, 100, 250, 20,hWnd, (HMENU)IDC_EDIT_INPUT, g_hInst, NULL);
    SendMessage(g_hEditInput, WM_SETFONT, (WPARAM)font, TRUE);
    g_hEditOutput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,80, 130, 250, 20,hWnd, (HMENU)IDC_EDIT_OUTPUT, g_hInst, NULL);
    SendMessage(g_hEditOutput, WM_SETFONT, (WPARAM)font, TRUE);
    g_hButtonConv = CreateWindow("BUTTON", "Convertir",WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,80, 160, 100, 24,hWnd, (HMENU)IDC_BUTTON_CONV, g_hInst, NULL);
    SendMessage(g_hButtonConv, WM_SETFONT, (WPARAM)font, TRUE);
}
static void Convertion()
{
    char bufBits[32] = { 0 };
    GetWindowText(g_hEditBits, bufBits, sizeof(bufBits));
    int bits = atoi(bufBits);
    if (bits < 2 || bits > 32) bits = 8;
    g_nBits = bits;
    int modeIndex = SendMessage(g_hComboMode, CB_GETCURSEL, 0, 0);
    int dirIndex = SendMessage(g_hComboDir, CB_GETCURSEL, 0, 0);
    ConvMode mode = (ConvMode)modeIndex;
    ConvDir dir = (ConvDir)dirIndex;
    char buf[256] = { 0 };
    GetWindowText(g_hEditInput, buf, sizeof(buf));
    std::string input(buf);
    std::string output;
    ConvertValue(mode, dir, input, output);
    SetWindowText(g_hEditOutput, output.c_str());
}
static LRESULT CALLBACK ProcedurePrincipale(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreationControls(hWnd);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_CONV)
        {
            Convertion();
            return 0;
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    g_hInst = hInst;
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ProcedurePrincipale;
    wc.hInstance = hInst;
    wc.hIcon = LoadIconA(NULL,(LPCSTR) 101);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "ConvertisseurDécimalBinaireAvance";
    RegisterClass(&wc);
    g_hWndMain = CreateWindow(wc.lpszClassName,"Convertisseur Decimal <> Binaire",WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX & ~WS_MINIMIZEBOX,CW_USEDEFAULT, CW_USEDEFAULT,380, 240,NULL, NULL, hInst, NULL);
    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
