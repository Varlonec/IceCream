#include "icecream.h"
#include <curses.h>
#include <panel.h>
#include <time.h>

static WINDOW* wmain;
static PANEL* pmain;
static WINDOW* wstatus;
static PANEL* pstatus;

void uiinit() {
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	
	wstatus = newwin(1, COLS, LINES-1, 0);
	pstatus = new_panel(wstatus);

	wmain = newwin(LINES-1, COLS, 0, 0);
	pmain = new_panel(wmain);
	scrollok(wmain, TRUE);
	
	update_panels();
	doupdate();
}

void uicleanup() {
	delwin(wmain);
	delwin(wstatus);
	endwin();
}

void status(const char* fmt, ...) {
	va_list vl;

	va_start(vl, fmt);
	wmove(wstatus, 0, 0);
	vwprintw(wstatus, fmt, vl);
	wprintw(wstatus, "\n");
	va_end(vl);

	va_start(vl, fmt);
	vwprintw(wmain, fmt, vl);
	wprintw(wmain, "\n");
	va_end(vl);

	update_panels();
	doupdate();
}

void log(const char* fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	vwprintw(wmain, fmt, vl);
	update_panels();
	doupdate();
	va_end(vl);
}

static struct {
	WINDOW* progwin;
	PANEL* pprogwin;
	const char* filename;
	const char* url;
	time_t tstart;
} progressinfo;

void pbstart(const char* filename, const char* url) {
	progressinfo.progwin = newwin(8, 52, (LINES-8)/2, (COLS-52)/2);
	progressinfo.pprogwin = new_panel(progressinfo.progwin);
	progressinfo.filename = filename;
	progressinfo.url = url;
	progressinfo.tstart = time(NULL);
}

void pbupdate(double dlnow, double dltotal) {
	char bar[51];

	int bars = (int)((dlnow/dltotal)*50.0);
	if (bars > 50) bars = 50; // Shouldn't be possible, but...
	
	int i;
	for (i = 0; i < bars; i++) bar[i] = '=';
	bar[i] = 0;
	
	time_t tnow = time(NULL);
	double speed = dlnow / (double)(tnow - progressinfo.tstart);
	
	werase(progressinfo.progwin);
	mvwprintw(progressinfo.progwin, 1, 1, "Downloading: %s\n", progressinfo.filename);
	mvwprintw(progressinfo.progwin, 2, 1, "From: %s\n", progressinfo.url);
	
	mvwprintw(progressinfo.progwin, 4, 1, "Speed: %.2lf kB/s\n", speed/1024.0);
	mvwprintw(progressinfo.progwin, 5, 1, "Progress: %d/%d kB (%.2lf%%)\n",
		(int)dlnow/1024, (int)dltotal/1024, (dlnow/dltotal)*100.0);
	mvwprintw(progressinfo.progwin, 6, 1, "%-50s", bar);
	box(progressinfo.progwin, 0, 0);
	
	update_panels();
	doupdate();
}

void pbdone() {
	del_panel(progressinfo.pprogwin);
	delwin(progressinfo.progwin);
	refresh();
}

static void showinfo(WINDOW* info, const Mod& mod) {
	int r = 0;
	int w = 0;
	char* text;
	char* word;
	
	werase(info);
	
	mvwprintw(info, r++, 0, "Name: %s\n", mod.name);
	mvwprintw(info, r++, 0, "\n");
	mvwprintw(info, r++, 0, "Mod: %s\n", mod.mod);
	mvwprintw(info, r++, 0, "\n");
	mvwprintw(info, r++, 0, "Author: %s\n", mod.author);
	mvwprintw(info, r++, 0, "\n");
	mvwprintw(info, r++, 0, "URL: %s\n", mod.url[0]);
	mvwprintw(info, r++, 0, "\n");
	mvwprintw(info, r++, 0, "Filename: %s\n", mod.filename);
	mvwprintw(info, r++, 0, "\n");
	
	mvwprintw(info, r++, 0, "Description:\n");
	text = strdup(mod.desc);
	word = strtok(text, " ");
	while (word) {
		if (w + (int)strlen(word) + 1 >= COLS/2) {
			waddch(info, '\n');
			if (r++ == LINES-2) break;
			wmove(info, r, 0);
			waddstr(info, word);
			w = strlen(word);
		} else {
			waddstr(info, word);
			w += strlen(word) + 1;
		}
		waddch(info, ' ');
		word = strtok(NULL, " ");
	}
	free(text);
	waddch(info, '\n');
}

int showmenu(const char* title, vector<char*>& options) {
	int ch;
	int line = 0;
	int offset = 0;
	int typed = 0;
	WINDOW* menu = newwin(LINES-1, COLS, 0, 0);
	PANEL* pmenu = new_panel(menu);
	status(" ");
	
	do {
		werase(menu);
		wprintw(menu, title);

		int start = offset;
		int end = offset + (LINES-4);
		if (end > (int)options.size()) end = options.size();
		for (int x = start; x < end; x++) {
			mvwprintw(menu, x-start+3, 0, "%d: %s\n", x+1, options[x]);
			if (x == line) mvwchgat(menu, x-start+3, 0, -1, A_REVERSE, 0, NULL);
		}
		update_panels();
		doupdate();
		
		ch = getch();
		switch (ch) {
		case KEY_UP:
			line--;
			if (line < 0) line = 0;
			if (line-1 <= start) {
				offset--;
				if (offset < 0) offset = 0;
			}
			if (typed) status(" ");
			typed = 0;
			break;
		case KEY_DOWN:
			line++;
			if (line >= end) line = end-1;
			if (line-start+5 >= LINES) {
				offset++;
			}
			if (typed) status(" ");
			typed = 0;
			break;
		case KEY_PPAGE:
			line -= LINES-4;
			if (line < 0) line = 0;
			offset = line-1;
			if (offset < 0) offset = 0;
			break;
		case KEY_NPAGE:
			line += LINES-4;
			if (line >= (int)options.size())
				line = options.size()-1;
			offset = line-LINES+6;
			if (offset < 0) offset = 0;
			break;
		case KEY_HOME:
			line = 0;
			offset = 0;
			break;
		case KEY_END:
			line = options.size()-1;
			offset = line-LINES+6;
			if (offset < 0) offset = 0;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			typed *= 10;
			typed += ch - '0';
			line = typed-1;
			if (line >= (int)options.size()) line = options.size()-1;
			status("%d", typed);
			break;
		}
	} while (ch != '\n');
	
	del_panel(pmenu);
	delwin(menu);
	refresh();
	return line;
}

int showmenu(list<Mod>& options) {
	int ch;
	int line = 0;
	int offset = 0;
	int typed = 0;
	WINDOW* menu = newwin(LINES-1, (COLS-1)/2, 0, 0);
	PANEL* pmenu = new_panel(menu);
	WINDOW* info = newwin(LINES-1, COLS/2, 0, (COLS-1)/2);
	PANEL* pinfo = new_panel(info);
	status(" ");
	
	do {
		werase(menu);
		wprintw(menu, "Select Mods");

		int start = offset;
		int end = offset + (LINES-4);
		if (end > (int)options.size()) end = options.size();

		list<Mod>::iterator i = options.begin();
		int x = 0;
		while (x < start) x++, i++;
		if (x++ == 0) {
			mvwprintw(menu, 3, 0, "Done\n");
			if (line == 0) mvwchgat(menu, 3, 0, -1, A_REVERSE, 0, NULL);
		}
		while (x <= end && i != options.end()) {
			mvwprintw(menu, x-start+3, 0, "%d: %s\n", x, i->name);
			if (x == line) {
				mvwchgat(menu, x-start+3, 0, -1, A_REVERSE, 0, NULL);
				showinfo(info, *i);
			}
			x++, i++;
		}
		end = x;
		update_panels();
		doupdate();
		
		ch = getch();
		switch (ch) {
		case KEY_UP:
			line--;
			if (line < 0) line = 0;
			if (line-1 <= start) {
				offset--;
				if (offset < 0) offset = 0;
			}
			if (typed) status(" ");
			typed = 0;
			break;
		case KEY_DOWN:
			line++;
			if (line >= end) line = end-1;
			if (line-start+5 >= LINES) {
				offset++;
			}
			if (typed) status(" ");
			typed = 0;
			break;
		case KEY_PPAGE:
			line -= LINES-4;
			if (line < 0) line = 0;
			offset = line-1;
			if (offset < 0) offset = 0;
			break;
		case KEY_NPAGE:
			line += LINES-4;
			if (line > (int)options.size())
				line = options.size();
			offset = line-LINES+6;
			if (offset < 0) offset = 0;
			break;
		case KEY_HOME:
			line = 0;
			offset = 0;
			break;
		case KEY_END:
			line = options.size();
			offset = line-LINES+6;
			if (offset < 0) offset = 0;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			typed *= 10;
			typed += ch - '0';
			line = typed;
			if (line > (int)options.size()) line = options.size();
			status("%d", typed);
			break;
		}
	} while (ch != '\n');
	
	del_panel(pinfo);
	delwin(info);
	del_panel(pmenu);
	delwin(menu);
	update_panels();
	doupdate();
	return line-1;
}

int confirm(struct BukkitInfo* binfo, vector<Mod>& mods) {
	char yn;
	
	status(" ");
	wprintw(wmain, "\nAbout to download Bukkit %s, and add the following mods in this order:\n", binfo->version);
	for (unsigned int i = 0; i < mods.size(); i++) {
		wprintw(wmain, "%d: %s\n", i+1, mods[i].name);
	}
	wprintw(wmain, "Are you sure? (y/N): ");
	update_panels();
	doupdate();
	
	yn = getch();
	
	if (tolower(yn) != 'y') {
		wprintw(wmain, "Exiting.\n");
		return 0;
	}
	return 1;
}



// EOF

