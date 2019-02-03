#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ncurses.h>

unsigned char registers[16];
unsigned char memory[4096];
unsigned short i = 0;
unsigned short pc = 0x200;
unsigned short stack[16];
unsigned char sp = 0;
unsigned char dt = 0;
unsigned char st = 0;
char display[64 * 32];
unsigned char key = 0x10;
struct timespec last_tick;

WINDOW* display_win;
WINDOW* instr_win;
WINDOW* registers_win;
WINDOW* stack_win;

void run();
unsigned char map_key(int k);
void toggle_pixel(unsigned int x, unsigned int y);
void show_display();
void show_registers();
void show_stack();

void clear_display() {
	for (int p = 0; p < 64 * 32; p++) {
		display[p] = ' ';
	}
}

int main(int argc, char *argv[]) {
	for (int r = 0; r < 16; r++) {
		registers[r] = 0x00;
	}
	for (int i = 0; i < 4096; i++) {
		memory[i] = 0;
	}
	for (int s = 0; s < 16; s++) {
		registers[s] = 0x00;
	}
	clear_display();
	srand(time(NULL));

	memory[0x100] = 0xF0; // ****
	memory[0x101] = 0x90; // *  *
	memory[0x102] = 0x90; // *  *
	memory[0x103] = 0x90; // *  *
	memory[0x104] = 0xF0; // ****

	memory[0x110] = 0x20; //   * 
	memory[0x111] = 0x60; //  ** 
	memory[0x112] = 0x20; //   * 
	memory[0x113] = 0x20; //   * 
	memory[0x114] = 0x70; //  ***

	memory[0x120] = 0xF0; // ****
	memory[0x121] = 0x10; //    *
	memory[0x122] = 0xF0; // ****
	memory[0x123] = 0x80; // *   
	memory[0x124] = 0xF0; // ****

	memory[0x130] = 0xF0; // ****
	memory[0x131] = 0x10; //    *
	memory[0x132] = 0xF0; // ****
	memory[0x133] = 0x10; //    *
	memory[0x134] = 0xF0; // ****

	memory[0x140] = 0x90; // *  *
	memory[0x141] = 0x90; // *  *
	memory[0x142] = 0xF0; // ****
	memory[0x143] = 0x10; //    *
	memory[0x144] = 0x10; //    *

	memory[0x150] = 0xF0; // ****
	memory[0x151] = 0x80; // *   
	memory[0x152] = 0xF0; // ****
	memory[0x153] = 0x10; //    *
	memory[0x154] = 0xF0; // ****

	memory[0x160] = 0xF0; // ****
	memory[0x161] = 0x80; // *   
	memory[0x162] = 0xF0; // ****
	memory[0x163] = 0x90; // *  *
	memory[0x164] = 0xF0; // ****

	memory[0x170] = 0xF0; // ****
	memory[0x171] = 0x10; //    *
	memory[0x172] = 0x20; //   * 
	memory[0x173] = 0x40; //  *  
	memory[0x174] = 0x40; //  *  

	memory[0x180] = 0xF0; // ****
	memory[0x181] = 0x90; // *  *
	memory[0x182] = 0xF0; // ****
	memory[0x183] = 0x90; // *  *
	memory[0x184] = 0xF0; // ****

	memory[0x190] = 0xF0; // ****
	memory[0x191] = 0x90; // *  *
	memory[0x192] = 0xF0; // ****
	memory[0x193] = 0x10; //    *
	memory[0x194] = 0xF0; // ****

	memory[0x1A0] = 0xF0; // ****
	memory[0x1A1] = 0x90; // *  *
	memory[0x1A2] = 0xF0; // ****
	memory[0x1A3] = 0x90; // *  *
	memory[0x1A4] = 0x90; // *  *

	memory[0x1B0] = 0xE0; // *** 
	memory[0x1B1] = 0x90; // *  *
	memory[0x1B2] = 0xE0; // *** 
	memory[0x1B3] = 0x90; // *  *
	memory[0x1B4] = 0xE0; // *** 

	memory[0x1C0] = 0xF0; // ****
	memory[0x1C1] = 0x80; // *   
	memory[0x1C2] = 0x80; // *   
	memory[0x1C3] = 0x80; // *   
	memory[0x1C4] = 0xF0; // ****

	memory[0x1D0] = 0xE0; // *** 
	memory[0x1D1] = 0x90; // *  *
	memory[0x1D2] = 0x90; // *  *
	memory[0x1D3] = 0x90; // *  *
	memory[0x1D4] = 0xE0; // *** 

	memory[0x1E0] = 0xF0; // ****
	memory[0x1E1] = 0x80; // *   
	memory[0x1E2] = 0xF0; // ****
	memory[0x1E3] = 0x80; // *   
	memory[0x1E4] = 0xF0; // ****

	memory[0x1F0] = 0xF0; // ****
	memory[0x1F1] = 0x80; // *   
	memory[0x1F2] = 0xF0; // ****
	memory[0x1F3] = 0x80; // *   
	memory[0x1F4] = 0x80; // *   

	FILE *file = fopen(argv[1], "r");
	i = 0x200;
	unsigned char instr[2];
	while (fread(instr, 1, 2, file) == 2) {
		memory[i++] = instr[0];
		memory[i++] = instr[1];
	}
	fclose(file);
	i = 0x0;

	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	curs_set(0);
	refresh();

	display_win = newwin(32 + 2, 64 + 2, 0, 0);
	box(display_win, 0, 0);
	wrefresh(display_win);
	instr_win = newwin(2 + 2, 13 + 2, 0, 64 + 2);
	box(instr_win, 0, 0);
	wrefresh(instr_win);
	registers_win = newwin(19 + 2, 5 + 2, 2 + 2, 64 + 2);
	box(registers_win, 0, 0);
	wrefresh(registers_win);
	stack_win = newwin(16 + 2, 6 + 2, 2 + 2, 64 + 2 + 5 + 2);
	box(stack_win, 0, 0);
	wrefresh(stack_win);

	run();

	nodelay(stdscr, FALSE);
	getch();
	endwin();
	return EXIT_SUCCESS;
}

void run() {
	clock_gettime(CLOCK_MONOTONIC, &last_tick);
	while (1) {
		unsigned char instr[2];
		instr[0] = memory[pc++];
		instr[1] = memory[pc++];
		unsigned char instr0 = instr[0] >> 4;
		unsigned char instr1 = instr[0] & 0x0F;
		unsigned char instr2 = instr[1] >> 4;
		unsigned char instr3 = instr[1] & 0x0F;

		wmove(instr_win, 1, 1);
		wprintw(instr_win, "%X%X%X%X", instr0, instr1, instr2, instr3);
		wprintw(instr_win, "   -> %03X", pc);
		wmove(instr_win, 2, 1);
		wprintw(instr_win, "             ");
		wmove(instr_win, 2, 1);
		wrefresh(instr_win);

		if (instr0 == 0x0) {
			if (instr1 == 0x0) {
				if (instr[1] == 0x00) {
					wprintw(instr_win, "         HALT");
					wrefresh(instr_win);
					break;
				} else if (instr[1] == 0xE0) {
					wprintw(instr_win, "CLS");
					clear_display();
					show_display();
				} else if (instr[1] == 0xEE) {
					wprintw(instr_win, "RET");
					--sp;
					pc = stack[sp];
					stack[sp] = 0x000;
				}
			}
		} else if (instr0 == 0x1) {
			unsigned short tmp = instr1;
			tmp <<= 8;
			tmp |= instr[1];
			if (pc - 2 == tmp) {
				wprintw(instr_win, "JP %03X   HALT", tmp);
				wrefresh(instr_win);
				break;
			}
			wprintw(instr_win, "JP %03X", tmp);
			pc = tmp;
		} else if (instr0 == 0x2) {
			stack[sp++] = pc;
			pc = instr1;
			pc <<= 8;
			pc |= instr[1];
			wprintw(instr_win, "CALL %03X", pc);
		} else if (instr0 == 0x3) {
			wprintw(instr_win, "SE V%X, %02X", instr1, instr[1]);
			if (registers[instr1] == instr[1]) {
				pc += 2;
			}
		} else if (instr0 == 0x4) {
			wprintw(instr_win, "SNE V%X, %02X", instr1, instr[1]);
			if (registers[instr1] != instr[1]) {
				pc += 2;
			}
		} else if (instr0 == 0x5) {
			if (instr3 == 0x0) {
				wprintw(instr_win, "SE V%X, V%X", instr1, instr2);
				if (registers[instr1] == registers[instr2]) {
					pc += 2;
				}
			}
		} else if (instr0 == 0x6) {
			wprintw(instr_win, "LD V%X, %02X", instr1, instr[1]);
			registers[instr1] = instr[1];
		} else if (instr0 == 0x7) {
			wprintw(instr_win, "ADD V%X, %02X", instr1, instr[1]);
			unsigned short tmp = registers[instr1];
			tmp += instr[1];
			registers[0xF] = tmp > 0xFF ? 0x01 : 0x00;
			registers[instr1] = tmp & 0xFF;
		} else if (instr0 == 0x8) {
			if (instr3 == 0x0) {
				wprintw(instr_win, "LD V%X, V%X", instr1, instr2);
				registers[instr1] = registers[instr2];
			} else if (instr3 == 0x1) {
				wprintw(instr_win, "OR V%X, V%X", instr1, instr2);
				registers[instr1] = registers[instr1] | registers[instr2];
			} else if (instr3 == 0x2) {
				wprintw(instr_win, "AND V%X, V%X", instr1, instr2);
				registers[instr1] = registers[instr1] & registers[instr2];
			} else if (instr3 == 0x3) {
				wprintw(instr_win, "XOR V%X, V%X", instr1, instr2);
				registers[instr1] = registers[instr1] ^ registers[instr2];
			} else if (instr3 == 0x4) {
				wprintw(instr_win, "ADD V%X, V%X", instr1, instr2);
				unsigned short tmp = registers[instr1];
				tmp += registers[instr2];
				registers[0xF] = tmp > 0xFF ? 0x01 : 0x00;
				registers[instr1] = tmp & 0xFF;
			} else if (instr3 == 0x5) {
				wprintw(instr_win, "SUB V%X, V%X", instr1, instr2);
				registers[0xF] =
						registers[instr1] > registers[instr2] ? 0x01 : 0x00;
				registers[instr1] = registers[instr1] - registers[instr2];
			} else if (instr3 == 0x6) {
				wprintw(instr_win, "SHR V%X {, V%X}", instr1, instr2);
				registers[0xF] = registers[instr1] & 0x01;
				registers[instr1] = registers[instr1] >> 1;
			} else if (instr3 == 0x7) {
				wprintw(instr_win, "SUBN V%X, V%X", instr1, instr2);
				registers[0xF] =
						registers[instr2] > registers[instr1] ? 0x01 : 0x00;
				registers[instr1] = registers[instr2] - registers[instr1];
			} else if (instr3 == 0xE) {
				wprintw(instr_win, "SHL V%X {, V%X}", instr1, instr2);
				registers[0xF] = (registers[instr1] & 0x80) >> 7;
				registers[instr1] = registers[instr1] << 1;
			}
		} else if (instr0 == 0x9) {
			if (instr3 == 0x0) {
				wprintw(instr_win, "SNE V%X, V%X", instr1, instr2);
				if (registers[instr1] != registers[instr2]) {
					pc += 2;
				}
			}
		} else if (instr0 == 0xA) {
			unsigned short tmp = instr1;
			tmp = tmp << 8 | instr[1];
			wprintw(instr_win, "LD I, %03X", tmp);
			i = tmp;
		} else if (instr0 == 0xB) {
			unsigned short tmp = instr1;
			tmp = tmp << 8 | instr[1];
			wprintw(instr_win, "JP V0, %03X", tmp);
			pc = registers[0x0] + tmp;
		} else if (instr0 == 0xC) {
			unsigned char rnd = rand() % 256;
			wprintw(instr_win, "RND V%X, %02X", instr1, instr[1]);
			registers[instr1] = rnd & instr[1];
		} else if (instr0 == 0xD) {
			registers[0x0F] = 0x00;
			wprintw(instr_win, "DRW V%X, V%X, %X", instr1, instr2, instr3);
			unsigned char xpos = registers[instr1];
			unsigned char ypos = registers[instr2];
			unsigned char height = instr3;
			for (unsigned short row = 0; row < height; row++) {
				unsigned char pixels = memory[row + i];
				for (unsigned char p = 0; p < 8; p++) {
					if ((pixels & (0x80 >> p)) != 0) {
						toggle_pixel(xpos + p, ypos + row);
					}
				}
			}
			show_display();
		} else if (instr0 == 0xE) {
			if (instr[1] == 0x9E) {
				wprintw(instr_win, "SKP V%X", instr1);
				if (key == registers[instr1]) {
					key = 0x10;
					pc += 2;
				}
			} else if (instr[1] == 0xA1) {
				wprintw(instr_win, "SKNP V%X", instr1);
				if (key != registers[instr1]) {
					pc += 2;
				} else {
					key = 0x10;
				}
			}
		} else if (instr0 == 0xF) {
			if (instr[1] == 0x07) {
				wprintw(instr_win, "LD V%X, DT", instr1);
				registers[instr1] = dt;
			} else if (instr[1] == 0x0A) {
				wprintw(instr_win, "LD V%X, K  _?_", instr1);
				wrefresh(instr_win);
				nodelay(stdscr, FALSE);
				int k = getch();
				nodelay(stdscr, TRUE);
				registers[instr1] = map_key(k);
			} else if (instr[1] == 0x15) {
				wprintw(instr_win, "LD DT, V%X", instr1);
				dt = registers[instr1];
			} else if (instr[1] == 0x18) {
				wprintw(instr_win, "LD ST, V%X", instr1);
				st = registers[instr1];
			} else if (instr[1] == 0x1E) {
				wprintw(instr_win, "ADD I, V%X", instr1);
				i += registers[instr1];
			} else if (instr[1] == 0x29) {
				wprintw(instr_win, "LD F, V%X", instr1);
				i = 0x100 | (registers[instr1] << 4);
			} else if (instr[1] == 0x33) {
				wprintw(instr_win, "LD B, V%X", instr1);
				unsigned char v = registers[instr1];
				char decimal[4];
				sprintf(decimal, "%03d", v);
				memory[i + 0] = decimal[0] - '0'; // hundreds
				memory[i + 1] = decimal[1] - '0'; // tens
				memory[i + 2] = decimal[2] - '0'; // ones
				wprintw(instr_win, "  %x%x%x", memory[i + 0], memory[i + 1],
						memory[i + 2]);
			} else if (instr[1] == 0x55) {
				wprintw(instr_win, "LD [I], V%X", instr1);
				for (unsigned char x = 0; x <= instr1; x++) {
					memory[i + x] = registers[x];
				}
			} else if (instr[1] == 0x65) {
				wprintw(instr_win, "LD V%X, [I]", instr1);
				for (unsigned char x = 0; x <= instr1; x++) {
					registers[x] = memory[i + x];
				}
			}
		}
		wrefresh(instr_win);
		show_registers();
		show_stack();

		int k = getch();
		if (k != ERR) {
			key = map_key(k);
		}
		move(22, 78);
		if (key < 0x10) {
			printw("%02X", key);
		} else {
			printw("  ");
		}
		refresh();

		struct timespec tick;
		clock_gettime(CLOCK_MONOTONIC, &tick);
		long tick_diff_sec = tick.tv_sec - last_tick.tv_sec;
		long tick_diff_nsec = tick.tv_nsec - last_tick.tv_nsec;
		long tick_diff_msec = tick_diff_sec * 1000 + tick_diff_nsec / 1000000;
		if (tick_diff_msec > 16) {
			if (dt > 0) {
				dt -= 1;
			}
			if (st > 0) {
				st -= 1;
			}
			last_tick.tv_sec = tick.tv_sec;
			last_tick.tv_nsec = tick.tv_nsec;
		}
		move(23, 76);
		printw(st > 0 ? "BEEP" : "    ");
		refresh();

		struct timespec sleep;
		sleep.tv_sec = 0;
		sleep.tv_nsec = 1 * 1000000;
		nanosleep(&sleep, NULL);
	}
}

unsigned char map_key(int k) {
	unsigned char key = 0x10;
	if (k >= 'a' && k <= 'f') {
		key = k - 'a' + 0xA;
	} else if (k >= '0' && k <= '9') {
		key = k - '0' + 0x0;
	}
	return key;
}

void toggle_pixel(unsigned int x, unsigned int y) {
	int l = x + y * 64;
	if (l >= 32 * 64) {
		return; // TODO
	}
	char pixel = display[l];
	if (pixel == 'x') {
		display[l] = ' ';
		registers[0xF] = 0x01;
	} else {
		display[l] = 'x';
	}
}

void show_display() {
	for (int l = 0; l < 32; l++) {
		wmove(display_win, l + 1, 1);
		for (int c = 64 * l; c < 64 * (l + 1); c++) {
			if (display[c] == ' ') {
				waddch(display_win, ' ');
			} else {
				waddch(display_win, ' ' | A_REVERSE);
			}
		}
	}
	wrefresh(display_win);
}

void show_registers() {
	for (int r = 0; r < 16; r++) {
		wmove(registers_win, r + 1, 1);
		wprintw(registers_win, "%X  %02X", r, registers[r]);
	}
	wmove(registers_win, 17, 1);
	wprintw(registers_win, "DT %02X", dt);
	wmove(registers_win, 18, 1);
	wprintw(registers_win, "ST %02X", st);
	wmove(registers_win, 19, 1);
	wprintw(registers_win, "I %03X", i);
	wrefresh(registers_win);
}

void show_stack() {
	for (int s = 0; s < 16; s++) {
		wmove(stack_win, s + 1, 1);
		if (s == sp) {
			wprintw(stack_win, "-> %03X", stack[s]);
		} else {
			wprintw(stack_win, "   %03X", stack[s]);
		}
	}
	wrefresh(stack_win);
}
