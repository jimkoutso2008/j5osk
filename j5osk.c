/*
 * j5osk.exe - minimal on-screen keyboard for WinPE on the Galaxy J5 2016.
 *
 * WinPE ships no osk.exe, and TabTip needs TabletInputService, which WinPE
 * lacks. This window docks at the bottom of the screen, never takes focus
 * (WS_EX_NOACTIVATE), and turns taps into real key presses with SendInput,
 * so they go to whatever window (e.g. cmd) has focus.
 *
 * Shift and Ctrl are sticky: tap once, they apply to the next key.
 * No C runtime; built with clang/lld from WSL (see build.sh).
 */

#include <windows.h>

typedef struct {
  const WCHAR *Label;
  const WCHAR *Shifted; /* NULL: same label */
  WORD         Vk;
  BYTE         Width;   /* in quarter keys */
} KEY;

#define VK_SHIFT_STICKY 0xFFF0
#define VK_CTRL_STICKY  0xFFF1
#define VK_HIDE         0xFFF2

static const KEY mRow1[] = {
  {L"1", L"!", '1', 4}, {L"2", L"@", '2', 4}, {L"3", L"#", '3', 4}, {L"4", L"$", '4', 4},
  {L"5", L"%", '5', 4}, {L"6", L"^", '6', 4}, {L"7", L"&", '7', 4}, {L"8", L"*", '8', 4},
  {L"9", L"(", '9', 4}, {L"0", L")", '0', 4}, {L"-", L"_", VK_OEM_MINUS, 4}, {L"=", L"+", VK_OEM_PLUS, 4},
};
static const KEY mRow2[] = {
  {L"q", L"Q", 'Q', 4}, {L"w", L"W", 'W', 4}, {L"e", L"E", 'E', 4}, {L"r", L"R", 'R', 4},
  {L"t", L"T", 'T', 4}, {L"y", L"Y", 'Y', 4}, {L"u", L"U", 'U', 4}, {L"i", L"I", 'I', 4},
  {L"o", L"O", 'O', 4}, {L"p", L"P", 'P', 4}, {L"[", L"{", VK_OEM_4, 4}, {L"]", L"}", VK_OEM_6, 4},
};
static const KEY mRow3[] = {
  {L"a", L"A", 'A', 4}, {L"s", L"S", 'S', 4}, {L"d", L"D", 'D', 4}, {L"f", L"F", 'F', 4},
  {L"g", L"G", 'G', 4}, {L"h", L"H", 'H', 4}, {L"j", L"J", 'J', 4}, {L"k", L"K", 'K', 4},
  {L"l", L"L", 'L', 4}, {L";", L":", VK_OEM_1, 4}, {L"'", L"\"", VK_OEM_7, 4}, {L"\\", L"|", VK_OEM_5, 4},
};
static const KEY mRow4[] = {
  {L"Shift", NULL, VK_SHIFT_STICKY, 6}, {L"z", L"Z", 'Z', 4}, {L"x", L"X", 'X', 4}, {L"c", L"C", 'C', 4},
  {L"v", L"V", 'V', 4}, {L"b", L"B", 'B', 4}, {L"n", L"N", 'N', 4}, {L"m", L"M", 'M', 4},
  {L",", L"<", VK_OEM_COMMA, 4}, {L".", L">", VK_OEM_PERIOD, 4}, {L"/", L"?", VK_OEM_2, 4}, {L"Bksp", NULL, VK_BACK, 6},
};
static const KEY mRow5[] = {
  {L"Ctrl", NULL, VK_CTRL_STICKY, 6}, {L"Esc", NULL, VK_ESCAPE, 5}, {L"Tab", NULL, VK_TAB, 5},
  {L"Space", NULL, VK_SPACE, 12}, {L"<", NULL, VK_LEFT, 4}, {L">", NULL, VK_RIGHT, 4},
  {L"^", NULL, VK_UP, 4}, {L"v", NULL, VK_DOWN, 4}, {L"Enter", NULL, VK_RETURN, 8},
};

static const struct { const KEY *Keys; int Count; } mRows[] = {
  {mRow1, ARRAYSIZE(mRow1)}, {mRow2, ARRAYSIZE(mRow2)}, {mRow3, ARRAYSIZE(mRow3)},
  {mRow4, ARRAYSIZE(mRow4)}, {mRow5, ARRAYSIZE(mRow5)},
};
#define ROWS ARRAYSIZE(mRows)

static BOOL  mShift;
static BOOL  mCtrl;
static HFONT mFont;

static int RowUnits(int Row)
{
  int U = 0;
  int I;

  for (I = 0; I < mRows[Row].Count; I++) {
    U += mRows[Row].Keys[I].Width;
  }
  return U;
}

/* Key rectangle in client coordinates */
static void KeyRect(HWND Wnd, int Row, int Index, RECT *R)
{
  RECT C;
  int  Units = RowUnits(Row);
  int  Before = 0;
  int  I;
  int  RowH;

  GetClientRect(Wnd, &C);
  RowH = (C.bottom - C.top) / ROWS;
  for (I = 0; I < Index; I++) {
    Before += mRows[Row].Keys[I].Width;
  }
  R->left   = C.left + (C.right - C.left) * Before / Units + 2;
  R->right  = C.left + (C.right - C.left) * (Before + mRows[Row].Keys[Index].Width) / Units - 2;
  R->top    = Row * RowH + 2;
  R->bottom = (Row + 1) * RowH - 2;
}

static void SendVk(WORD Vk, BOOL Up)
{
  INPUT In;

  ZeroMemory(&In, sizeof(In));
  In.type       = INPUT_KEYBOARD;
  In.ki.wVk     = Vk;
  In.ki.dwFlags = Up ? KEYEVENTF_KEYUP : 0;
  if (Vk == VK_LEFT || Vk == VK_RIGHT || Vk == VK_UP || Vk == VK_DOWN) {
    In.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
  }
  SendInput(1, &In, sizeof(In));
}

static void Press(HWND Wnd, const KEY *K)
{
  if (K->Vk == VK_SHIFT_STICKY) {
    mShift = !mShift;
  } else if (K->Vk == VK_CTRL_STICKY) {
    mCtrl = !mCtrl;
  } else {
    if (mCtrl) {
      SendVk(VK_CONTROL, FALSE);
    }
    if (mShift) {
      SendVk(VK_SHIFT, FALSE);
    }
    SendVk(K->Vk, FALSE);
    SendVk(K->Vk, TRUE);
    if (mShift) {
      SendVk(VK_SHIFT, TRUE);
    }
    if (mCtrl) {
      SendVk(VK_CONTROL, TRUE);
    }
    mShift = FALSE;
    mCtrl  = FALSE;
  }
  InvalidateRect(Wnd, NULL, TRUE);
}

static void Paint(HWND Wnd)
{
  PAINTSTRUCT Ps;
  HDC         Dc = BeginPaint(Wnd, &Ps);
  RECT        C;
  RECT        R;
  HBRUSH      Back = CreateSolidBrush(RGB(32, 32, 32));
  HBRUSH      Key  = CreateSolidBrush(RGB(70, 70, 70));
  HBRUSH      On   = CreateSolidBrush(RGB(0, 120, 215));
  HGDIOBJ     OldFont = SelectObject(Dc, mFont);
  int         Row;
  int         I;

  GetClientRect(Wnd, &C);
  FillRect(Dc, &C, Back);
  SetBkMode(Dc, TRANSPARENT);
  SetTextColor(Dc, RGB(240, 240, 240));

  for (Row = 0; Row < (int)ROWS; Row++) {
    for (I = 0; I < mRows[Row].Count; I++) {
      const KEY *K = &mRows[Row].Keys[I];
      BOOL Lit = (K->Vk == VK_SHIFT_STICKY && mShift) || (K->Vk == VK_CTRL_STICKY && mCtrl);

      KeyRect(Wnd, Row, I, &R);
      FillRect(Dc, &R, Lit ? On : Key);
      DrawTextW(Dc, (mShift && K->Shifted) ? K->Shifted : K->Label, -1, &R,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
  }

  SelectObject(Dc, OldFont);
  DeleteObject(Back);
  DeleteObject(Key);
  DeleteObject(On);
  EndPaint(Wnd, &Ps);
}

static LRESULT CALLBACK WndProc(HWND Wnd, UINT Msg, WPARAM W, LPARAM L)
{
  POINT P;
  RECT  R;
  int   Row;
  int   I;

  switch (Msg) {
  case WM_MOUSEACTIVATE:
    return MA_NOACTIVATE; /* keep focus in the target window */

  case WM_LBUTTONDOWN:
    P.x = (short)LOWORD(L);
    P.y = (short)HIWORD(L);
    for (Row = 0; Row < (int)ROWS; Row++) {
      for (I = 0; I < mRows[Row].Count; I++) {
        KeyRect(Wnd, Row, I, &R);
        if (PtInRect(&R, P)) {
          Press(Wnd, &mRows[Row].Keys[I]);
          return 0;
        }
      }
    }
    return 0;

  case WM_PAINT:
    Paint(Wnd);
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(Wnd, Msg, W, L);
}

void WINAPI WinMainCRTStartup(void)
{
  HINSTANCE Inst = GetModuleHandleW(NULL);
  WNDCLASSW Wc;
  MSG       Msg;
  HWND      Wnd;
  int       Sw = GetSystemMetrics(SM_CXSCREEN);
  int       Sh = GetSystemMetrics(SM_CYSCREEN);
  int       H  = Sh * 3 / 10;

  ZeroMemory(&Wc, sizeof(Wc));
  Wc.lpfnWndProc   = WndProc;
  Wc.hInstance     = Inst;
  Wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
  Wc.lpszClassName = L"J5Osk";
  RegisterClassW(&Wc);

  mFont = CreateFontW(-(H / ROWS / 3), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");

  Wnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, L"J5Osk", L"Keyboard",
                        WS_POPUP | WS_VISIBLE, 0, Sh - H, Sw, H, NULL, NULL, Inst, NULL);
  if (Wnd == NULL) {
    ExitProcess(1);
  }

  while (GetMessageW(&Msg, NULL, 0, 0) > 0) {
    TranslateMessage(&Msg);
    DispatchMessageW(&Msg);
  }
  ExitProcess(0);
}
