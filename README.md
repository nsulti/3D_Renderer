# RU
Первый проект связанный с разработкой 3Д движка с визаулизацией теней.
Занял много бессонных ночей и слёз (ну ладно шучу, относительно простой, самое сложное это заниматься математикой и следить за failed to allocate у векторов).
PSA: Стараюсь делать так, чтобы при желании читать мои комментарии и просто пострадать можно было развивать этот проект и добавлять новые фигуры. Ну, что-то такого. :)
-- *nsulti*


# EN
My first project involving development of a 3D renderer with shadows.
Took me many sleepless nights and tears (jk it was relatively simple, the hardest part was doing the math and watching the failed to allocate stuff in vectors).
PSA: Trying to make it so, that if you wish to read my comments (no EN translation of them lol) and suffer my genius (not) code, you could actually develop it further and add new shapes. Or something like that. :)
-- *nsulti*


# ПОЛНЫЙ РАЗБОР ПРОЕКТА / FULL PROJECT BREAKDOWN
*КОМПИЛЯЦИЯ / COMPILATION*
**LINUX**:
1. rm -rf build
2. cmake -S . -B build
3. cmake --build build
4. ./3D_renderer

**WINDOWS**:
1. Remove-Item -Recurse -Force build
2. cmake -G "MinGW Makefiles" -B build
3. cmake --build build
4. .\3D_renderer.exe

*ПРОИЗВОДИТЕЛЬНОСТЬ / PERFORMANCE*
Только 1 ядро! / One core only!

Arch Linux (i5-12450H)
![Alt text](/home/nsulti/Videos/output.gif)
(Max: ~16% CPU)

Windows (Ryzen 5 5000U)
![Alt text](/home/nsulti/Videos/win11.gif)
(Max ~5% CPU)

видимо minifb очень оптимизирован под Windows :/
~~надеюсь кому-нибудь этот проект пригодится :)~~


