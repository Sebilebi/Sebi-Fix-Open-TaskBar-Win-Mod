// ==WindhawkMod==
// @id              sebi-fix-open-taskbar-win-mod
// @name            Sebi Fix Open TaskBar Win Mod
// @description     Soluciona el error donde la barra de tareas oculta no responde al raton al tener ventanas maximizadas (Brave, Discord, etc.).
// @version         1.0
// @author          Sebilebi
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lshell32
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Sebi Fix Open TaskBar Win Mod

Corrige el conocido fallo de Windows 10 y Windows 11 donde la barra de tareas con ocultación automática
deja de desplegarse al pasar el ratón por el borde inferior cuando hay ventanas maximizadas (Brave, Discord, Spotify, etc.).

### Cómo funciona:
1. **Garantía de Z-Order en primer plano:** Evita que las ventanas maximizadas con marcos personalizados tapen la pequeña franja invisible de detección de la barra de tareas (`Shell_TrayWnd`).
2. **Detección activa del borde inferior:** En cuanto el ratón toca el borde inferior de la pantalla, sitúa la barra en primer plano (`HWND_TOPMOST`) y reactiva el temporizador nativo de auto-hide (`kTrayUITimerUnhide`).
3. **Respeto a videojuegos y vídeos:** Si una aplicación está en pantalla completa real (juegos sin maximizar o vídeos en F11), no interrumpe la pantalla completa.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- edgeThreshold: 2
  $name: "Margen del borde (pixeles)"
  $description: "Distancia en pixeles desde el borde inferior de la pantalla para activar el despliegue de la barra (por defecto 2)."
- keepTopmostOnFocus: true
  $name: "Mantener barra en primer plano"
  $description: "Mantiene la barra en primer plano en el Z-order al cambiar o maximizar ventanas para que no quede atrapada por debajo."
- ignoreFullscreenGames: true
  $name: "Ignorar juegos a pantalla completa"
  $description: "Si esta activado, la barra no se desplegara sobre videojuegos o ventanas que esten en pantalla completa real sin maximizar."
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <shellapi.h>
#include <atomic>

struct {
    int edgeThreshold;
    bool keepTopmostOnFocus;
    bool ignoreFullscreenGames;
} settings;

HANDLE g_hThread = NULL;
DWORD g_dwThreadId = 0;
HHOOK g_hMouseHook = NULL;
HWINEVENTHOOK g_hEventHookForeground = NULL;
HWINEVENTHOOK g_hEventHookLocation = NULL;

void LoadSettings() {
    settings.edgeThreshold = Wh_GetIntSetting(L"edgeThreshold");
    if (settings.edgeThreshold <= 0) {
        settings.edgeThreshold = 2;
    }
    settings.keepTopmostOnFocus = Wh_GetIntSetting(L"keepTopmostOnFocus") != 0;
    settings.ignoreFullscreenGames = Wh_GetIntSetting(L"ignoreFullscreenGames") != 0;
}

// Comprueba si la ventana en primer plano es un juego a pantalla completa o vídeo en F11 (no maximizado estándar)
bool IsRealFullscreenWindow(HWND hWnd, const RECT& rcMon) {
    if (!hWnd || hWnd == GetDesktopWindow() || hWnd == GetShellWindow()) {
        return false;
    }

    // Si la ventana está maximizada con el botón de maximizar, NO es pantalla completa de juego
    WINDOWPLACEMENT wp = { sizeof(wp) };
    if (GetWindowPlacement(hWnd, &wp) && wp.showCmd == SW_SHOWMAXIMIZED) {
        return false;
    }

    LONG style = GetWindowLongW(hWnd, GWL_STYLE);
    if (style & WS_MAXIMIZE) {
        return false;
    }

    // Comprueba si el rectángulo de la ventana cubre toda la pantalla del monitor
    RECT rcWnd;
    if (GetWindowRect(hWnd, &rcWnd)) {
        if (rcWnd.left <= rcMon.left && rcWnd.top <= rcMon.top &&
            rcWnd.right >= rcMon.right && rcWnd.bottom >= rcMon.bottom) {
            return true;
        }
    }

    return false;
}

// Pone la barra de tareas en primer plano (HWND_TOPMOST)
void EnsureTaskbarsTopmost() {
    HWND hMainTray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (hMainTray) {
        SetWindowPos(hMainTray, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    HWND hSecTray = nullptr;
    while ((hSecTray = FindWindowExW(nullptr, hSecTray, L"Shell_SecondaryTrayWnd", nullptr)) != nullptr) {
        SetWindowPos(hSecTray, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

// Busca la barra correspondiente al monitor indicado
HWND FindTaskbarForMonitor(HMONITOR hMon) {
    HWND hPrimary = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (hPrimary) {
        HMONITOR hPrimaryMon = MonitorFromWindow(hPrimary, MONITOR_DEFAULTTOPRIMARY);
        if (hPrimaryMon == hMon) {
            return hPrimary;
        }
    }

    HWND hSec = nullptr;
    while ((hSec = FindWindowExW(nullptr, hSec, L"Shell_SecondaryTrayWnd", nullptr)) != nullptr) {
        if (MonitorFromWindow(hSec, MONITOR_DEFAULTTONEAREST) == hMon) {
            return hSec;
        }
    }

    return hPrimary;
}

// Hook de eventos de Windows para mantener la barra topmost cuando cambian las ventanas
void CALLBACK WinEventProc(HWINEVENTHOOK /*hWinEventHook*/,
                           DWORD event,
                           HWND hwnd,
                           LONG idObject,
                           LONG /*idChild*/,
                           DWORD /*idEventThread*/,
                           DWORD /*dwmsEventTime*/) {
    if (!settings.keepTopmostOnFocus) {
        return;
    }

    if (event == EVENT_SYSTEM_FOREGROUND) {
        EnsureTaskbarsTopmost();
    } else if (event == EVENT_OBJECT_LOCATIONCHANGE && idObject == OBJID_WINDOW) {
        if (hwnd && IsZoomed(hwnd)) {
            EnsureTaskbarsTopmost();
        }
    }
}

// Hook de ratón de bajo nivel para detectar cuando el cursor toca el borde inferior
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_MOUSEMOVE || wParam == WM_NCMOUSEMOVE)) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
        POINT pt = pMouse->pt;

        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfoW(hMon, &mi)) {
            // Comprobamos si el cursor está en el borde inferior
            if (pt.y >= (mi.rcMonitor.bottom - settings.edgeThreshold)) {
                HWND hFore = GetForegroundWindow();
                if (settings.ignoreFullscreenGames && IsRealFullscreenWindow(hFore, mi.rcMonitor)) {
                    // Si es un juego a pantalla completa real, respetamos
                } else {
                    HWND hTaskbar = FindTaskbarForMonitor(hMon);
                    if (hTaskbar) {
                        // 1. Forzar a la barra a estar en primer plano en el Z-Order
                        SetWindowPos(hTaskbar, HWND_TOPMOST, 0, 0, 0, 0,
                                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                        // 2. Disparar el temporizador nativo de auto-hide de Explorer (timer ID 3 = kTrayUITimerUnhide)
                        SetTimer(hTaskbar, 3, 0, nullptr);
                    }
                }
            }
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// Hilo en segundo plano que hospeda los hooks
DWORD WINAPI TaskbarMonitorThread(LPVOID /*lpParam*/) {
    Wh_Log(L"SebiFixOpenTaskBarWinMod: Monitor thread iniciado");

    g_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandleW(nullptr), 0);
    if (!g_hMouseHook) {
        Wh_Log(L"Error al instalar WH_MOUSE_LL hook: %u", GetLastError());
    }

    g_hEventHookForeground = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        nullptr, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT
    );

    g_hEventHookLocation = SetWinEventHook(
        EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE,
        nullptr, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT
    );

    // Asegurar estado inicial
    EnsureTaskbarsTopmost();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hMouseHook) {
        UnhookWindowsHookEx(g_hMouseHook);
        g_hMouseHook = NULL;
    }
    if (g_hEventHookForeground) {
        UnhookWinEvent(g_hEventHookForeground);
        g_hEventHookForeground = NULL;
    }
    if (g_hEventHookLocation) {
        UnhookWinEvent(g_hEventHookLocation);
        g_hEventHookLocation = NULL;
    }

    Wh_Log(L"SebiFixOpenTaskBarWinMod: Monitor thread finalizado");
    return 0;
}

BOOL Wh_ModInit() {
    Wh_Log(L"SebiFixOpenTaskBarWinMod: Init");

    LoadSettings();

    g_hThread = CreateThread(nullptr, 0, TaskbarMonitorThread, nullptr, 0, &g_dwThreadId);
    if (!g_hThread) {
        Wh_Log(L"Error al crear TaskbarMonitorThread: %u", GetLastError());
        return FALSE;
    }

    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"SebiFixOpenTaskBarWinMod: Uninit");

    if (g_dwThreadId) {
        PostThreadMessageW(g_dwThreadId, WM_QUIT, 0, 0);
    }
    if (g_hThread) {
        WaitForSingleObject(g_hThread, 2000);
        CloseHandle(g_hThread);
        g_hThread = NULL;
        g_dwThreadId = 0;
    }
}

void Wh_ModSettingsChanged() {
    Wh_Log(L"SebiFixOpenTaskBarWinMod: SettingsChanged");
    LoadSettings();
}
