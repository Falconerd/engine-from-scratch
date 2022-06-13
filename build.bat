set render=src\engine\render\render.c src\engine\render\render_init.c src\engine\render\render_util.c
set io=src\engine\io\io.c
set files=src\glad.c src\main.c src\engine\global.c %render% %io%
set libs=W:\lib\SDL2main.lib W:\lib\SDL2.lib W:\lib\freetype.lib

CL /Zi /I W:\include %files% /link %libs% /OUT:mygame.exe

