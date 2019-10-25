#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define shortSep "\n_________________________________________________\n"
#define longSep "\n//////////////////////////////////////////////////////////////////////////\n"
#define msgTam 120

#define NR_LEDS 8
#define NR_BYTES_COLOR 6
#define BUFFER_LENGTH (NR_LEDS)*(NR_BYTES_COLOR+3)+1

#define BLINKSTICK_PATH "/dev/usb/blinkstick0" 



int colors (int opt){
	int fdBlink, color;
	
	switch(opt){
		case 0 : color = 0x000088; break; //azul
		case 1 : color = 0x008800; break; //verde
		case 2 : color = 0x880000; break; //rojo
		case 3 : color = 0x000000; break; //negro
		default : color = 0x000000; break;
	}
	
	if ((fdBlink = open(BLINKSTICK_PATH, O_WRONLY)) < 0)
		return -1;

	char buffer[BUFFER_LENGTH * 10];
	sprintf(buffer, "0:%d,1:%d,2:%d,3:%d,4:%d,5:%d,6:%d,7:%d",color,color,color,color,color,color,color,color);
	write(fdBlink, buffer, strlen(buffer));
	
	return 0;
	
}
int genNum(){

	int n;
	n = rand() & 0xff;
	n |= (rand() & 0xff) << 8;
	n |= (rand() & 0xff) << 16;
	printf("/////////%d///////////\n", n);
	return n;
}

int randomColors(){
	srand(time(NULL));
	char buffer[BUFFER_LENGTH * 10];
	int fdBlink;
	
	if ((fdBlink = open(BLINKSTICK_PATH, O_WRONLY)) < 0)
		return -1;
		
	sprintf(buffer, "0:%d,1:%d,2:%d,3:%d,4:%d,5:%d,6:%d,7:%d",
		genNum(),genNum(),genNum(),genNum(),genNum(),genNum(),genNum(),genNum());
	write(fdBlink, buffer, strlen(buffer));
	
	return 0;
}

int fullEpilepsia(){
	int segs;
	printf("¿Cuantos segundos durará?\n");
	scanf("%d", &segs);
	for (int i = 0; i < segs*2; ++i){
		randomColors();
		sleep(1);	
	}
	
	return 0;
}


int printMenu(){
	int elec;

	char *buf = (char*) malloc(msgTam);
	
	printf(longSep);
	
	buf = "Opción 1:	Salir.\n";
	printf(buf);

	printf(shortSep);
	
	buf = "Opción 2:	Leds a Azul.\n";
	printf(buf);
	
	printf(shortSep);

	buf = "Opción 3:	Leds a Verde.\n";
	printf(buf);
	
	printf(shortSep);
	
	buf = "Opción 4:	Leds a Rojo.\n";
	printf(buf);
	
	printf(shortSep);

	buf = "Opción 5:	Apagar Leds.\n";
	printf(buf);
	
	printf(shortSep);
	
	buf = "Opción 6:	Full Random.\n";
	printf(buf);
	
	printf(shortSep);
	
	buf = "Opción 7:	Epilepsia.\n";
	printf(buf);
	
	printf(longSep);
	buf = "Elige una opción:\n>> ";
	printf(buf);
	scanf("%d", &elec);
	
	return elec;
}


int main (int argc, char **argv){
	int op;
	
	while((op = printMenu()) > 0){
		switch(op){
			case 1 : return 0;
			case 2 : colors(0); break;
			case 3 : colors(1); break;
			case 4 : colors(2); break;
			case 5 : colors(3); break;
			case 6 : randomColors(); break;
			case 7 : fullEpilepsia(); break;
			default : printf("Valor incorrecto. Escoja un valor de la lista.\n");
		}
	}
	return 0;
}
