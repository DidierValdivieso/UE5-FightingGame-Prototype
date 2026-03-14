# Dónde se llama StartWidgetSequence (inicio del WidgetSequenceManager)

## Resumen de llamadas

| # | Archivo | Cuándo | ¿Guardado con IsFrontendWidgetVisible? |
|---|---------|--------|----------------------------------------|
| 1 | **ElderbreachBrawlersGameInstance.cpp** | `OnStart()` → timer 0.35s (o 0.5s) → `TryStartMenuSequence()`; reintenta cada 0.25s hasta 20 veces si no hay PC | Sí |
| 2 | **MainGameMode.cpp** `BeginPlay()` | Si mapa frontend (L_MainMenu): timer **0.5s** → fallback “por si PostLogin no inició los widgets” | Sí |
| 3 | **MainGameMode.cpp** `BeginPlay()` | Si mapa arena sin LevelManager: `StartMenuWidgetsWithDelay()` → timer **1.0s** | Sí |
| 4 | **MainGameMode.cpp** `BeginPlay()` | Si mapa arena, sin pending combat, sin controller config: timer **0.5s** (arranque directo en arena) | Sí |
| 5 | **MainGameMode.cpp** `PostLogin()` | Si mapa frontend: timer **0.15s** → `StartWidgetSequence(NewPlayer)` | **No** → puede duplicar |
| 6 | **StageSelectWidget.cpp** | Usuario pulsa “Atrás” → `NavigateBackToFrontend()` → `StartWidgetSequence(PC)` | Intencionado |
| 7 | **FinalFightWidget.cpp** | Usuario pulsa “Main Menu” → `StartWidgetSequence(PC)` | Intencionado |

## Problema

- En **L_MainMenu** pueden correr a la vez:
  - **GameInstance**: primer intento a 0.35s (y reintentos cada 0.25s).
  - **MainGameMode BeginPlay**: fallback a 0.5s.
  - **PostLogin**: a 0.15s tras el login del jugador.
- **PostLogin** no comprueba si el menú ya está visible, así que si el jugador hace login después de que ya se haya mostrado el menú (p. ej. por el GameInstance), a los 0.15s se vuelve a llamar **StartWidgetSequence** → **inicio duplicado**.

## Solución aplicada

1. **StartWidgetSequence** hace early return si el frontend ya está visible (llamada idempotente).
2. **PostLogin** solo llama a **StartWidgetSequence** si `!Mgr->IsFrontendWidgetVisible()`.
