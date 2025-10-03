# Hodiny s budíkem (RTC)

Projekt: Hodiny s budíkem založené na modulu Real Time Clock (RTC).  
Autor: Adam Dalibor Jurčík (xjurci08) — implementace je v [Sources/main.c](Sources/main.c).

## Krátký přehled
- Vestavěný projekt pro desku Minerva1 (Fitkit3). Hardware musí být připojen k PC a projekt se nahrává/pouští v NXP Kinetis Design Studio (KDS).
- Dokumentace (v češtině): [dokumentace.pdf](dokumentace.pdf) / [dokumentace.tex](dokumentace.tex).
- Hlavní zdrojový soubor: [Sources/main.c](Sources/main.c). Důležité funkce: [`MCUInit`](Sources/main.c), [`PortsInit`](Sources/main.c), [`UART5Init`](Sources/main.c), [`RTCInit`](Sources/main.c), [`RTC_IRQHandler`](Sources/main.c), [`Play_song`](Sources/main.c), [`LED_on`](Sources/main.c).

### Důležité soubory ve stromu projektu
- Zdroj: [Sources/main.c](Sources/main.c)  
- Hlavičky CMSIS a zařízení: [Includes/core_cm4.h](Includes/core_cm4.h), [Includes/core_cmFunc.h](Includes/core_cmFunc.h), [Includes/system_MK60D10.h](Includes/system_MK60D10.h)  
- Startup a vektory: [Project_Settings/Startup_Code/startup_MK60D10.S](Project_Settings/Startup_Code/startup_MK60D10.S)  
- Linker script: [Project_Settings/Linker_Files/MK60DN512xxx10_flash.ld](Project_Settings/Linker_Files/MK60DN512xxx10_flash.ld)  
- Dokumentace zdroj: [dokumentace.tex](dokumentace.tex) a výsledný PDF: [dokumentace.pdf](dokumentace.pdf)

## Jak spustit / sestavit
- Vývoj a nahrání firmwaru: otevřít projekt v KDS (NXP Kinetis Design Studio) a použít konfiguraci pro Minerva1 / FTDI COM port. Projekt je konfigurovaný pro běh přímo z KDS (debug/flash).  
- Generování dokumentace (LaTeX): v repozitáři je Makefile pro vytvoření PDF:
  - Spustit v terminálu: make (vygeneruje `dokumentace.pdf`).

### Krátký popis funkcionality (viz [Sources/main.c](Sources/main.c))
- Uživatelské rozhraní přes UART (115200 baud) — čtení/pis přes `SendStr`/`ReceiveStr` v [Sources/main.c](Sources/main.c).
- Nastavení času, nastavení alarmu, výběr zvukové signalizace (`Play_song`) a světelné (`LED_on`), opakování alarmu.
- Ovládání alarmu realizuje přerušení od RTC v [`RTC_IRQHandler`](Sources/main.c).

## Poznámky
- Projekt využívá CMSIS hlavičky obsažené v [Includes/](Includes/). Pokud potřebujete upravit inicializaci hodin nebo přerušení, podívejte se na [Project_Settings/Startup_Code/startup_MK60D10.S](Project_Settings/Startup_Code/startup_MK60D10.S) a na `RTC` registry v [Includes/system_MK60D10.h](Includes/system_MK60D10.h).
- Dokumentace popisuje FSM, ovládání přes PuTTY a nastavení opakování alarmu — viz [dokumentace.pdf](dokumentace.pdf).

## Licencování a poznámky k zdrojům
- CMSIS a startup soubory jsou od ARM / Freescale—viz hlavičky v [Includes/](Includes/) a v [Project_Settings/Startup_Code/startup_MK60D10.S](Project_Settings/Startup_Code/startup_MK60D10.S).
