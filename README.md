# Sebi Fix Open TaskBar Win Mod

[![Windhawk Mod](https://img.shields.io/badge/Windhawk-Mod-blue.svg)](https://windhawk.net/)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%20Windows%2011-0078d7.svg)](https://microsoft.com/windows)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Un mod para **[Windhawk](https://windhawk.net/)** que corrige de forma definitiva el molesto fallo de Windows donde la barra de tareas con ocultación automática deja de desplegarse al pasar el ratón por el borde inferior cuando hay ventanas maximizadas (como **Brave, Discord, Chrome, Spotify, VS Code**, etc.).

---

## 🛑 El problema en Windows 10 y 11

Cuando tienes la barra de tareas configurada en **"Ocultar automáticamente la barra de tareas"**:
1. Al maximizar una ventana con marco personalizado (aplicaciones basadas en Chromium, Electron o XAML/WinUI), la ventana maximizada suele sobreponerse en el orden Z (*Z-Order*) o bloquear la pequeña franja invisible de 1 o 2 píxeles que usa Windows Explorer (`Shell_TrayWnd`) para detectar el ratón.
2. Al mover el cursor hacia el borde inferior de la pantalla, la barra de tareas **no se despliega**.
3. **El truco manual habitual:** Minimizar las ventanas, abrir la barra y hacer clic físico con el ratón en el botón de Inicio de Windows para "desbugear" Explorer (el teclado con la tecla Win no lo solucionaba). Al cabo de un rato o al cambiar de aplicación maximizada, el fallo volvía a aparecer.

---

## ⚡ La solución del Mod

Este mod se inyecta directamente dentro de `explorer.exe` a través de Windhawk y resuelve el problema en la raíz mediante dos mecanismos combinados:

1. **Garantía de Z-Order en primer plano (`HWND_TOPMOST`):**
   Cada vez que una ventana se enfoca o se maximiza, el mod asegura que la barra de tareas (`Shell_TrayWnd`) y las barras secundarias no queden enterradas por debajo del marco de las aplicaciones.
2. **Sensor activo en el borde inferior (`WH_MOUSE_LL`):**
   En el momento en que el cursor del ratón toca los píxeles del borde inferior de la pantalla:
   - Sitúa la barra de tareas en primer plano absoluto (`HWND_TOPMOST`).
   - Reactiva el temporizador nativo de auto-hide de Explorer (`kTrayUITimerUnhide`), produciendo exactamente el mismo efecto de reseteo que hacías al pulsar el botón de Windows con el ratón, pero de forma instantánea y automática.
3. **Filtro inteligente para videojuegos y vídeos (F11):**
   Distingue entre una ventana maximizada estándar (Brave, Discord) y una ventana a pantalla completa real (un juego o un vídeo en F11 sin barra de título). De esta forma, el ratón no desplegará accidentalmente la barra mientras juegas o ves películas.
4. **Soporte multimonitor:**
   Detecta automáticamente en qué monitor se encuentra el ratón y actúa sobre la barra de tareas de ese monitor específico (`Shell_TrayWnd` o `Shell_SecondaryTrayWnd`).
5. **Rendimiento ultra ligero:**
   Basado 100% en eventos del sistema y hooks de bajo nivel. No realiza bucles activos ni consumo innecesario de procesador (0% CPU).

---

## 🛠️ Instalación en Windhawk

1. Instala y abre **[Windhawk](https://windhawk.net/)**.
2. En la barra lateral, haz clic en **"Crear mod"** (o en el menú de desarrollo).
3. Reemplaza todo el código que aparece por defecto con el contenido del archivo [`SebiFixOpenTaskBarWinMod.wh.cpp`](./SebiFixOpenTaskBarWinMod.wh.cpp).
4. Haz clic en el botón azul **"Compilar el mod"** (o presiona `Ctrl + B`).
5. Asegúrate de que el interruptor **"Habilitar mod"** en la parte superior izquierda esté activado.
6. ¡Listo! Ya puedes probar a maximizar Brave, Discord o cualquier app y mover el ratón al borde inferior.

---

## ⚙️ Opciones de configuración (Settings)

El mod incluye ajustes configurables directamente desde la interfaz de Windhawk:

| Opción | Tipo | Por defecto | Descripción |
| :--- | :---: | :---: | :--- |
| **`edgeThreshold`** | Entero | `2` | Distancia en píxeles desde el borde inferior de la pantalla para activar el despliegue. |
| **`keepTopmostOnFocus`** | Booleano | `true` | Restaura automáticamente la barra en primer plano cada vez que cambias o maximizas una ventana. |
| **`ignoreFullscreenGames`** | Booleano | `true` | Evita que la barra se despliegue sobre videojuegos o reproductores a pantalla completa real (F11). |

---

## 📄 Licencia

Este proyecto está bajo la licencia [MIT](https://opensource.org/licenses/MIT).
Creado por **Sebilebi**.
