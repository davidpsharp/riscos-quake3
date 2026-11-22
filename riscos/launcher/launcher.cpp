/*
 * ioquake3 for RISC OS: the launcher (!ioquake3.Launcher), run by !Run when there is no game
 * data: no baseq3.pak0/pk3 (Quake III Arena), baseoa.pak0/pk3 (OpenArena) or demoq3.pak0/pk3
 * (the free demo). With data, !Run starts the game itself and this isn't used.
 *
 * A small desktop task with the icon on the icon bar (menu: Info, Help, Quit). It offers at once
 * to fetch the free Quake III Arena demo:
 *
 *   1. download id Software's Linux demo installer (49 MB) from a mirror of id's FTP site,
 *      over HTTP, with a progress bar, hashing it as it arrives;
 *   2. check its SHA-256 against the one known;
 *   3. take id's licence (Help/Q3A_EULA.txt) out of it and open it in the text editor: nothing
 *      is installed unless the user accepts it;
 *   4. take demoq3/pak0.pk3 out of it (gzip, then tar), checking its SHA-256 too;
 *   5. delete the installer, start the game (through !Run, which now finds the data) and quit:
 *      the game has an icon of its own.
 *
 * Dropping the installer file (linuxq3ademo-1.11-6.x86.gz.sh) on the icon does the same from
 * step 2. Nothing of id's is in this package: the data comes from id's archive at the user's
 * request (id's licence doesn't allow passing it on).
 *
 * Adapted from riscos-doom3's launcher (riscos/launcher, 22 Nov 2026), whose Wimp code (icon,
 * menu, Info window, the message window and its buttons, the handler swap around Wimp_Poll) is
 * from David Sharp's OpenTS RISC OS port (port/riscos/desktop.cpp).
 */
#include "sha256.h"

#include <kernel.h>
#include <swis.h>
#include <unixlib/local.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include <string>
#include <vector>

int __riscosify_control = __RISCOSIFY_NO_SUFFIX;

#ifndef LAUNCHER_VERSION
#define LAUNCHER_VERSION "0.1.0-dev"
#endif

namespace {

/* --- what's fetched, and what it must be ---------------------------------------------- */

char const DemoLeaf[] = "linuxq3ademo-1.11-6.x86.gz.sh";
// HTTP mirrors of id's FTP site, tried in turn (checked 22 Nov 2026)
char const * const Mirrors[] = {
	"http://ftp.gwdg.de/pub/misc/ftp.idsoftware.com/idstuff/quake3/linux/linuxq3ademo-1.11-6.x86.gz.sh",
	"http://ftp.fu-berlin.de/pc/games/idgames/idstuff/quake3/linux/linuxq3ademo-1.11-6.x86.gz.sh",
};
int const MirrorCount = sizeof(Mirrors) / sizeof(Mirrors[0]);
double const RunSize = 49289300.0;
char const RunSha256[] = "64dee3f69b6e792d1da4fe0ac98fedc7eb1e37ea1027fb609a9fadd06150a4ec";
long const RunArchiveOffset = 5468;	// the gzipped tar after the installer's 164-line shell script
char const Pk4Name[] = "demoq3/pak0.pk3";	// (the name's kept from dhewm3's launcher)
double const Pk4Size = 46853694.0;
char const Pk4Sha256[] = "e77abad2466f45a0a7ea018445528f9b95a0fe7789fa1abc1a7718bbf0754b08";
char const LicenceName[] = "Help/Q3A_EULA.txt";
char const LicenceSha256[] = "23ed30a9ea5875261791549f16875576e7428365ff82dd943cc07339f1173673";
char const LicenceFile[] = "demoq3/Q3A_EULA.txt";	// where it's kept, with the data

/* --- the Wimp (adapted from OpenTS's port/riscos/desktop.cpp) ----------------------------- */

int TaskHandle = 0;
int IconHandle = -1;
int InfoWindow = -1;
bool GameStarted = false;	// then this quits
std::string GameArgs;	// passed on to the game (!Run's arguments)

char const AppSprite[] = "!ioquake3";
char const AppName[] = "ioquake3";
char const AppPurpose[] = "Quake III Arena engine";
char const AppAuthor[] = "id Software; ioquake3 team";

bool Swi(int swi, _kernel_swi_regs & regs)
{
	_kernel_oserror * error = _kernel_swi(swi, &regs, &regs);
	if (error != nullptr) {
		fprintf(stderr, "launcher: SWI &%X: %s\n", swi, error->errmess);
		return false;
	}
	return true;
}

struct Icon {
	int x0, y0, x1, y1;
	unsigned flags;
	union {
		char text[12];
		struct {
			char const * buffer;
			char const * validation;
			int length;
		} ind;
	} data;
};

void Plain_Text(Icon & icon, int x0, int y0, int x1, int y1, unsigned flags, char const * text)
{
	icon.x0 = x0; icon.y0 = y0; icon.x1 = x1; icon.y1 = y1;
	icon.flags = flags;
	memset(icon.data.text, 0, sizeof(icon.data.text));
	strncpy(icon.data.text, text, sizeof(icon.data.text));
}

void Indirected(Icon & icon, int x0, int y0, int x1, int y1, unsigned flags, char const * text, char const * valid)
{
	icon.x0 = x0; icon.y0 = y0; icon.x1 = x1; icon.y1 = y1;
	icon.flags = flags;
	icon.data.ind.buffer = text;
	icon.data.ind.validation = valid;
	icon.data.ind.length = (int)strlen(text) + 1;
}

struct WindowBlock {
	int visible[4];
	int scroll[2];
	int behind;
	unsigned flags;
	unsigned char colours[8];
	int extent[4];
	unsigned title_flags;
	unsigned button_type;
	int sprite_area;
	short min_w, min_h;
	struct {
		char const * buffer;
		char const * validation;
		int length;
	} title;
	int icon_count;
};

// "About this program", as Edit's progInfo: labels beside sunken fields.
void Create_Info_Window(void)
{
	static char version[64];
	snprintf(version, sizeof(version), "%s (%s)", LAUNCHER_VERSION, __DATE__);
	static struct {
		WindowBlock w;
		Icon icons[10];
	} block;
	static char const title[] = "About this program";
	static char const field_valid[] = "R2";
	memset(&block, 0, sizeof(block));
	block.w.visible[0] = 392; block.w.visible[1] = 628;
	block.w.visible[2] = 392 + 668; block.w.visible[3] = 628 + 308;
	block.w.behind = -1;
	block.w.flags = 0x84000012;
	unsigned char const colours[8] = {7, 2, 7, 1, 12, 14, 12, 0};
	memcpy(block.w.colours, colours, sizeof(colours));
	block.w.extent[1] = -308;
	block.w.extent[2] = 668;
	block.w.title_flags = 0x0000013D;
	block.w.sprite_area = 1;
	block.w.title.buffer = title;
	block.w.title.validation = (char const *)-1;
	block.w.title.length = sizeof(title);
	char const * labels[5] = {"Name", "Purpose", "Author", "Ported by", "Version"};
	char const * values[5] = {AppName, AppPurpose, AppAuthor, "David Sharp", version};
	for (int row = 0; row < 5; row++) {
		int const top = -4 - 60 * row;
		Indirected(block.icons[row], 184, top - 52, 660, top, 0x1700613D, values[row], field_valid);
		Plain_Text(block.icons[5 + row], 8, top - 48, 184, top - 8, 0x17000211, labels[row]);
	}
	block.w.icon_count = 10;
	_kernel_swi_regs regs;
	regs.r[1] = (int)&block;
	if (Swi(Wimp_CreateWindow, regs)) {
		InfoWindow = regs.r[0];
	}
}

struct {
	char title[12];
	unsigned char colours[4];
	int width, height, gap;
	struct {
		unsigned flags;
		int submenu;
		unsigned icon_flags;
		char text[12];
	} items[3];
} Menu;

enum { MenuInfo, MenuHelp, MenuQuit };

void Build_Menu(void)
{
	memset(&Menu, 0, sizeof(Menu));
	strncpy(Menu.title, AppName, sizeof(Menu.title));
	Menu.colours[0] = 7; Menu.colours[1] = 2; Menu.colours[2] = 7; Menu.colours[3] = 0;
	Menu.width = 128;
	Menu.height = 44;
	char const * names[3] = {"Info", "Help", "Quit"};
	for (int i = 0; i < 3; i++) {
		Menu.items[i].submenu = i == MenuInfo ? InfoWindow : -1;
		Menu.items[i].icon_flags = 0x07000021;
		strncpy(Menu.items[i].text, names[i], sizeof(Menu.items[i].text));
	}
	Menu.items[MenuQuit].flags = 0x80;	// the last item
}

void Open_Menu(int mouse_x)
{
	_kernel_swi_regs regs;
	regs.r[1] = (int)&Menu;
	regs.r[2] = mouse_x - 64;
	regs.r[3] = 96 + 3 * 44;
	Swi(Wimp_CreateMenu, regs);
}

void Put_Icon_On_Bar(void)
{
	struct {
		int window;
		Icon icon;
	} block;
	block.window = -1;
	Plain_Text(block.icon, 0, 0, 68, 68, 0x1700301A, AppSprite);
	_kernel_swi_regs regs;
	regs.r[0] = 0;
	regs.r[1] = (int)&block;
	if (Swi(Wimp_CreateIcon, regs)) {
		IconHandle = regs.r[0];
	}
}

/* Around Wimp_Poll the OS's default handlers are in, not UnixLib's (as in OpenTS: another
   task's error or an event could otherwise run UnixLib's handler with this task paged out). */
struct HandlerType { int Address, Workspace, Buffer; };
int const Swapped_Handlers[] = {1, 2, 3, 4, 6, 7, 9, 10};
int const Swapped_Count = sizeof(Swapped_Handlers) / sizeof(Swapped_Handlers[0]);

HandlerType Set_Handler(int number, HandlerType const & handler)
{
	_kernel_swi_regs regs;
	regs.r[0] = number;
	regs.r[1] = handler.Address;
	regs.r[2] = handler.Workspace;
	regs.r[3] = handler.Buffer;
	_kernel_swi(OS_ChangeEnvironment, &regs, &regs);
	HandlerType previous = {regs.r[1], regs.r[2], regs.r[3]};
	return previous;
}

void Use_Default_Handlers(HandlerType saved[])
{
	for (int i = 0; i < Swapped_Count; i++) {
		_kernel_swi_regs regs;
		regs.r[0] = Swapped_Handlers[i];
		_kernel_swi(OS_ReadDefaultHandler, &regs, &regs);
		HandlerType handler = {regs.r[1], regs.r[2], regs.r[3]};
		saved[i] = Set_Handler(Swapped_Handlers[i], handler);
	}
}

void Use_Own_Handlers(HandlerType const saved[])
{
	for (int i = 0; i < Swapped_Count; i++) {
		Set_Handler(Swapped_Handlers[i], saved[i]);
	}
}

unsigned Now(void)
{
	_kernel_swi_regs regs;
	_kernel_swi(OS_ReadMonotonicTime, &regs, &regs);
	return (unsigned)regs.r[0];
}

std::string Os_Variable(char const * name)
{
	char buf[512];
	_kernel_swi_regs regs;
	regs.r[0] = (int)name;
	regs.r[1] = (int)buf;
	regs.r[2] = sizeof(buf) - 1;
	regs.r[3] = 0;
	regs.r[4] = 3;
	if (_kernel_swi(OS_ReadVarVal, &regs, &regs) != nullptr) {
		return "";
	}
	buf[regs.r[2]] = '\0';
	return buf;
}

void Oscli(std::string const & command)
{
	_kernel_swi_regs regs;
	regs.r[0] = (int)command.c_str();
	Swi(OS_CLI, regs);
}

void Open_Help(void)
{
	Oscli("Filer_Run " + Os_Variable("ioquake3$Dir") + ".!Help");
}

/* --- files: Unix names under the application directory (UnixLib translates them) -------- */

std::string App_Path(char const * relative)
{
	return std::string("/<ioquake3$Dir>/") + relative;
}

double File_Size(char const * relative)
{
	struct stat st;
	if (stat(App_Path(relative).c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
		return -1;
	}
	return (double)st.st_size;
}

// As !Run decides: any of the three games' pak0
bool Has_Game_Data(void)
{
	return File_Size(Pk4Name) > 0 || File_Size("baseq3/pak0.pk3") > 0 || File_Size("baseoa/pak0.pk3") > 0;
}

// Free space on the disc holding !ioquake3, in bytes (-1: unknown). ioquake3$TestFree (MB) pretends.
double Free_Space(void)
{
	std::string test = Os_Variable("ioquake3$TestFree");
	if (!test.empty()) {
		return atof(test.c_str()) * 1024 * 1024;
	}
	std::string where = Os_Variable("ioquake3$Dir");
	_kernel_swi_regs regs;
	regs.r[0] = 55;
	regs.r[1] = (int)where.c_str();
	if (_kernel_swi(OS_FSControl, &regs, &regs) == nullptr) {
		return (double)(unsigned)regs.r[0] + (double)(unsigned)regs.r[1] * 4294967296.0;
	}
	regs.r[0] = 49;
	regs.r[1] = (int)where.c_str();
	if (_kernel_swi(OS_FSControl, &regs, &regs) == nullptr) {
		return (double)(unsigned)regs.r[0];
	}
	return -1;
}

std::string Megabytes(double bytes)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%.0f MB", bytes / 1e6);
	return buf;
}

/* --- the message window, with buttons and an optional progress bar (after OpenTS's) ------- */

enum Action { ACT_CLOSE, ACT_DOWNLOAD, ACT_CANCEL_JOB, ACT_ACCEPT, ACT_DECLINE, ACT_PLAY, ACT_HELP, ACT_LICENCE };

int const MessageWidth = 1120;
int const MaxLines = 12;
int const MaxButtons = 3;
int MessageWindow = -1;
char MessageLines[MaxLines][100];
char ButtonText[MaxButtons][24];
Action ButtonAction[MaxButtons];
int ButtonCount = 0;
char SpriteName[16];
int StatusIcon = -1;	// the status line, under the text
int BarIcon = -1;	// the progress bar inside its trough
int TroughIcon = -1;
int TroughBox[4];
char StatusText[100];

void Close_Message(void)
{
	if (MessageWindow < 0) {
		return;
	}
	int block[1] = {MessageWindow};
	_kernel_swi_regs regs;
	regs.r[1] = (int)block;
	Swi(Wimp_DeleteWindow, regs);
	MessageWindow = -1;
	StatusIcon = BarIcon = TroughIcon = -1;
}

int Wrap(std::string const & text, size_t width)
{
	int lines = 0;
	size_t pos = 0;
	while (pos < text.size() && lines < MaxLines) {
		size_t end = text.size();
		size_t nl = text.find('\n', pos);
		if (nl != std::string::npos && nl - pos <= width) {
			end = nl;
		} else if (end - pos > width) {
			end = text.rfind(' ', pos + width);
			if (end == std::string::npos || end <= pos) {
				end = pos + width;
			}
		}
		snprintf(MessageLines[lines++], sizeof(MessageLines[0]), "%s", text.substr(pos, end - pos).c_str());
		pos = end;
		while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\n')) {
			if (text[pos++] == '\n') {
				break;
			}
		}
	}
	return lines;
}

bool Wimp_Has_Sprite(char const * name)
{
	_kernel_swi_regs regs;
	regs.r[0] = 40;
	regs.r[2] = (int)name;
	return _kernel_swi(Wimp_SpriteOp, &regs, &regs) == nullptr;
}

struct Button {
	char const * text;
	Action action;
};

void Show_Message(std::string const & text, bool error, std::vector<Button> const & buttons, bool progress = false)
{
	Close_Message();
	fprintf(stderr, "launcher: message: %s\n", text.c_str());
	int lines = Wrap(text, 58);
	snprintf(SpriteName, sizeof(SpriteName), "%s", error ? "error" : "information");
	bool sprite = Wimp_Has_Sprite(SpriteName);
	int text_x = sprite ? 128 : 32;
	int text_bottom = -32 - 44 * (lines > 2 ? lines : 2);
	int bar_top = text_bottom - 16;
	int button_y = progress ? bar_top - 44 - 16 - 48 - 28 : text_bottom - 28;
	int height = -button_y + 68 + 28;

	static struct {
		WindowBlock w;
		Icon icons[3 + MaxButtons + MaxLines + 1];
	} block;
	memset(&block, 0, sizeof(block));
	block.w.behind = -1;
	block.w.flags = 0x84000012;
	unsigned char const colours[8] = {7, 2, 7, 1, 12, 14, 12, 0};
	memcpy(block.w.colours, colours, sizeof(colours));
	block.w.extent[1] = -height;
	block.w.extent[2] = MessageWidth;
	block.w.title_flags = 0x0000013D;
	block.w.sprite_area = 1;
	block.w.title.buffer = AppName;
	block.w.title.validation = (char const *)-1;
	block.w.title.length = sizeof(AppName);

	static char const default_valid[] = "R6,3";
	static char const plain_valid[] = "R5,3";
	ButtonCount = buttons.size() < (size_t)MaxButtons ? (int)buttons.size() : MaxButtons;
	int right = MessageWidth - 32;
	int widths[MaxButtons];
	for (int b = 0; b < ButtonCount; b++) {
		snprintf(ButtonText[b], sizeof(ButtonText[b]), "%s", buttons[b].text);
		ButtonAction[b] = buttons[b].action;
		widths[b] = (int)strlen(ButtonText[b]) * 16 + 64;
		if (widths[b] < 200) {
			widths[b] = 200;
		}
	}
	for (int b = ButtonCount - 1; b >= 0; b--) {
		Indirected(block.icons[b], right - widths[b], button_y - 68, right, button_y, 0x1700313D, ButtonText[b],
			b == ButtonCount - 1 ? default_valid : plain_valid);
		right -= widths[b] + 24;
	}
	int n = ButtonCount;
	for (int i = 0; i < lines; i++) {
		int top = -32 - 44 * i;
		Indirected(block.icons[n++], text_x, top - 44, MessageWidth - 32, top, 0x07000111, MessageLines[i],
			(char const *)-1);
		block.icons[n - 1].data.ind.length = sizeof(MessageLines[i]);
	}
	if (sprite) {
		Indirected(block.icons[n++], 32, -32 - 76, 32 + 76, -32, 0x1700011A, SpriteName, (char const *)1);
		block.icons[n - 1].data.ind.length = (int)strlen(SpriteName) + 1;
	}
	if (progress) {
		// the trough: a sunken white box; the bar: a filled green box inside it, resized as it goes
		TroughBox[0] = text_x; TroughBox[1] = bar_top - 44; TroughBox[2] = MessageWidth - 32; TroughBox[3] = bar_top;
		TroughIcon = n;
		static char const trough_valid[] = "R2";
		static char const empty[] = "";
		Indirected(block.icons[n++], TroughBox[0], TroughBox[1], TroughBox[2], TroughBox[3], 0x0700013D, empty,
			trough_valid);
		BarIcon = n;
		Plain_Text(block.icons[n++], TroughBox[0] + 4, TroughBox[1] + 4, TroughBox[0] + 4, TroughBox[3] - 4,
			0xA0000020, "");
		StatusIcon = n;
		StatusText[0] = '\0';
		Indirected(block.icons[n++], text_x, TroughBox[1] - 16 - 48, MessageWidth - 32, TroughBox[1] - 16, 0x07000111,
			StatusText, (char const *)-1);
		block.icons[n - 1].data.ind.length = sizeof(StatusText);
	}
	block.w.icon_count = n;

	_kernel_swi_regs regs;
	regs.r[1] = (int)&block;
	if (!Swi(Wimp_CreateWindow, regs)) {
		return;
	}
	MessageWindow = regs.r[0];

	int size[4];
	static int const vars[] = {4, 5, 11, 12, -1};
	regs.r[0] = (int)vars;
	regs.r[1] = (int)size;
	int screen_w = 1600, screen_h = 1200;
	if (_kernel_swi(OS_ReadVduVariables, &regs, &regs) == nullptr) {
		screen_w = (size[2] + 1) << size[0];
		screen_h = (size[3] + 1) << size[1];
	}
	int open[8] = {MessageWindow, (screen_w - MessageWidth) / 2, (screen_h - height) / 2, 0, 0, 0, 0, -1};
	open[3] = open[1] + MessageWidth;
	open[4] = open[2] + height;
	regs.r[1] = (int)open;
	Swi(Wimp_OpenWindow, regs);
	regs.r[0] = MessageWindow;
	regs.r[1] = -1;
	regs.r[2] = 0;
	regs.r[3] = 0;
	regs.r[4] = 1 << 25;
	regs.r[5] = -1;
	Swi(Wimp_SetCaretPosition, regs);
}

void Tell(std::string const & text, bool error)
{
	Show_Message(text, error, {{"OK", ACT_CLOSE}});
}

// The status line and bar: fraction 0..1 (or < 0 to leave the bar).
void Show_Progress(std::string const & text, double fraction)
{
	if (MessageWindow < 0 || StatusIcon < 0) {
		return;
	}
	_kernel_swi_regs regs;
	if (strcmp(StatusText, text.c_str()) != 0) {
		snprintf(StatusText, sizeof(StatusText), "%s", text.c_str());
		int set[4] = {MessageWindow, StatusIcon, 0, 0};
		regs.r[1] = (int)set;
		Swi(Wimp_SetIconState, regs);
	}
	if (fraction >= 0) {
		if (fraction > 1) {
			fraction = 1;
		}
		static int last_x1 = -1;
		int inner = TroughBox[2] - TroughBox[0] - 8;
		int x1 = TroughBox[0] + 4 + (int)(inner * fraction);
		if (x1 != last_x1) {
			last_x1 = x1;
			regs.r[0] = MessageWindow;
			regs.r[1] = BarIcon;
			regs.r[2] = TroughBox[0] + 4;
			regs.r[3] = TroughBox[1] + 4;
			regs.r[4] = x1;
			regs.r[5] = TroughBox[3] - 4;
			Swi(Wimp_ResizeIcon, regs);
			regs.r[0] = MessageWindow;
			regs.r[1] = TroughBox[0];
			regs.r[2] = TroughBox[1];
			regs.r[3] = TroughBox[2];
			regs.r[4] = TroughBox[3];
			Swi(Wimp_ForceRedraw, regs);
		}
	}
}

/* --- the job: download, check, licence, install ------------------------------------------- */

enum Stage { IDLE, CONNECTING, HEADERS, BODY, HASHING, LICENCE_EXTRACT, LICENCE_WAIT, INSTALLING };
Stage JobStage = IDLE;

struct JobState {
	int mirror = 0;
	int fd = -1;
	unsigned started = 0;	// centiseconds: connect timeout, speed
	std::string host, path;
	std::string header;
	double length = 0, received = 0;
	int redirects = 0;
	FILE * out = nullptr;
	Sha256 sha {};
	// hashing a dropped file, and the archive
	FILE * in = nullptr;
	double done = 0;
	z_stream z {};
	bool z_open = false;
	// tar
	unsigned char hdr[512] {};
	size_t hdr_have = 0;
	double entry_left = 0, pad_left = 0;
	FILE * entry_out = nullptr;
	int entry_kind = 0;	// 0 skip, 1 licence, 2 pk4
	Sha256 entry_sha {};
	bool got_licence = false, got_pk4 = false;
} Job;

/* Files with 1 MB buffers: SDFS on a Pi is slow with UnixLib's default 4 KB writes (the
   install ran at under 1 MB/s). Each role has its own buffer, kept for the next job. */
enum { BUF_OUT, BUF_IN, BUF_ENTRY, BUF_ROLES };
FILE * Open_Big(std::string const & path, char const * mode, int role)
{
	static char * buffers[BUF_ROLES];
	size_t const size = 1024 * 1024;
	FILE * f = fopen(path.c_str(), mode);
	if (f != nullptr) {
		if (buffers[role] == nullptr) {
			buffers[role] = (char *)malloc(size);
		}
		if (buffers[role] != nullptr) {
			setvbuf(f, buffers[role], _IOFBF, size);
		}
	}
	return f;
}

std::string Run_File(void) { return App_Path("download/") + DemoLeaf; }
std::string Pk4_Part(void) { return App_Path("demoq3/pak0.pk3.part"); }

void Close_Job_Files(void)
{
	if (Job.fd >= 0) { close(Job.fd); Job.fd = -1; }
	if (Job.out) { fclose(Job.out); Job.out = nullptr; }
	if (Job.in) { fclose(Job.in); Job.in = nullptr; }
	if (Job.entry_out) { fclose(Job.entry_out); Job.entry_out = nullptr; }
	if (Job.z_open) { inflateEnd(&Job.z); Job.z_open = false; }
}

// Stops the job, leaving nothing half-made behind (and the installer too if remove_run).
void Abandon(bool remove_run)
{
	Close_Job_Files();
	remove(Pk4_Part().c_str());
	if (remove_run) {
		remove(Run_File().c_str());
		rmdir(App_Path("download").c_str());
		remove(App_Path(LicenceFile).c_str());
	}
	JobStage = IDLE;
}

void Fail(std::string const & why, bool remove_run)
{
	Abandon(remove_run);
	Tell(why, true);
}

bool Parse_Url(std::string const & url, std::string & host, std::string & path)
{
	if (url.compare(0, 7, "http://") != 0) {
		return false;
	}
	size_t slash = url.find('/', 7);
	host = url.substr(7, slash == std::string::npos ? std::string::npos : slash - 7);
	path = slash == std::string::npos ? "/" : url.substr(slash);
	return !host.empty();
}

void Next_Mirror(std::string const & why);

void Connect(std::string const & url)
{
	if (!Parse_Url(url, Job.host, Job.path)) {
		Next_Mirror("not an http:// address: " + url);
		return;
	}
	Show_Progress("Looking up " + Job.host + "...", 0);
	struct hostent * he = gethostbyname(Job.host.c_str());	// (blocks briefly)
	if (he == nullptr || he->h_addrtype != AF_INET) {
		Next_Mirror("can't find " + Job.host);
		return;
	}
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr, he->h_addr_list[0], 4);
	Job.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (Job.fd < 0) {
		Next_Mirror("no socket");
		return;
	}
	// RISC OS's stack gives a small receive buffer (so a small TCP window): over ~30 ms to a
	// mirror that held a Pi 4 to about 0.7 MB/s. Ask for the biggest it allows.
	for (int size = 1024 * 1024; size >= 32 * 1024; size /= 2) {
		if (setsockopt(Job.fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == 0) {
			fprintf(stderr, "launcher: receive buffer %d KB\n", size / 1024);
			break;
		}
	}
	int on = 1;
	ioctl(Job.fd, FIONBIO, &on);
	if (connect(Job.fd, (struct sockaddr *)&sa, sizeof(sa)) != 0 && errno != EINPROGRESS && errno != EALREADY) {
		Next_Mirror("can't connect to " + Job.host + ": " + strerror(errno));
		return;
	}
	Job.started = Now();
	Job.header.clear();
	JobStage = CONNECTING;
	Show_Progress("Connecting to " + Job.host + "...", 0);
}

void Next_Mirror(std::string const & why)
{
	fprintf(stderr, "launcher: mirror %d: %s\n", Job.mirror, why.c_str());
	if (Job.fd >= 0) { close(Job.fd); Job.fd = -1; }
	if (Job.out) { fclose(Job.out); Job.out = nullptr; }
	Job.mirror++;
	if (Job.mirror >= MirrorCount) {
		Fail("The demo couldn't be downloaded (" + why + "). Check the network, then click the icon to try again. "
			"!Help says how to install it by hand.", true);
		return;
	}
	Job.redirects = 0;
	Connect(Mirrors[Job.mirror]);
}

void Start_Download(void)
{
	double free_space = Free_Space();
	double needed = RunSize + Pk4Size + 20e6;
	if (free_space >= 0 && free_space < needed) {
		Tell("There isn't room for the demo on the disc that holds !ioquake3: it needs " + Megabytes(needed)
			+ " while it's downloaded and installed (" + Megabytes(Pk4Size) + " after), and " + Megabytes(free_space)
			+ " is free.", true);
		return;
	}
	mkdir(App_Path("download").c_str(), 0755);
	Job = JobState();
	Show_Message("Downloading the Quake III Arena demo (id Software's Linux installer, " + Megabytes(RunSize)
		+ ") from a mirror of id's FTP site.", false, {{"Cancel", ACT_CANCEL_JOB}}, true);
	Connect(Mirrors[0]);
}

std::string Speed_Text(void)
{
	double secs = (Now() - Job.started) / 100.0;
	char buf[100];
	if (secs < 1) {
		snprintf(buf, sizeof(buf), "%s of %s", Megabytes(Job.received).c_str(), Megabytes(RunSize).c_str());
	} else {
		snprintf(buf, sizeof(buf), "%s of %s from %s, %.1f MB/s", Megabytes(Job.received).c_str(),
			Megabytes(RunSize).c_str(), Job.host.c_str(), Job.received / 1e6 / secs);
	}
	return buf;
}

void Begin_Archive(void);

void Download_Finished(void)
{
	fclose(Job.out);
	Job.out = nullptr;
	close(Job.fd);
	Job.fd = -1;
	char hex[65];
	Sha256_Hex(Job.sha, hex);
	fprintf(stderr, "launcher: downloaded %.0f bytes, sha256 %s\n", Job.received, hex);
	if (Job.received != RunSize || strcmp(hex, RunSha256) != 0) {
		Next_Mirror("the download wasn't the file expected (its SHA-256 didn't match)");
		if (JobStage != IDLE) {
			Show_Progress("That copy was damaged: trying another mirror", 0);
		}
		return;
	}
	Begin_Archive();
}

// One null event's worth of downloading: up to 5 cs of reading.
void Download_Tick(void)
{
	if (JobStage == CONNECTING) {
		fd_set w;
		FD_ZERO(&w);
		FD_SET(Job.fd, &w);
		struct timeval tv = {0, 0};
		if (select(Job.fd + 1, nullptr, &w, nullptr, &tv) > 0) {
			int err = 0;
			socklen_t len = sizeof(err);
			getsockopt(Job.fd, SOL_SOCKET, SO_ERROR, &err, &len);
			if (err != 0) {
				Next_Mirror("can't connect to " + Job.host + ": " + strerror(err));
				return;
			}
			std::string request = "GET " + Job.path + " HTTP/1.1\r\nHost: " + Job.host
				+ "\r\nUser-Agent: ioquake3-riscos-launcher\r\nAccept: */*\r\nConnection: close\r\n\r\n";
			if (send(Job.fd, request.data(), request.size(), 0) != (ssize_t)request.size()) {
				Next_Mirror("couldn't send the request to " + Job.host);
				return;
			}
			JobStage = HEADERS;
			Job.started = Now();
			Show_Progress("Asking " + Job.host + " for the file...", 0);
		} else if (Now() - Job.started > 2000) {
			Next_Mirror(Job.host + " didn't answer");
		}
		return;
	}
	static char buf[64 * 1024];
	unsigned until = Now() + 5;
	while ((int)(until - Now()) > 0) {
		ssize_t n = recv(Job.fd, buf, sizeof(buf), 0);
		if (n < 0) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				if (Now() - Job.started > 3000 && Job.received == 0) {
					Next_Mirror(Job.host + " stopped answering");
				}
				break;
			}
			Next_Mirror(std::string("the connection failed: ") + strerror(errno));
			return;
		}
		if (n == 0) {	// closed
			if (JobStage == BODY && Job.received == Job.length) {
				Download_Finished();
			} else {
				Next_Mirror(Job.host + " closed the connection early");
			}
			return;
		}
		char const * p = buf;
		size_t left = (size_t)n;
		if (JobStage == HEADERS) {
			Job.header.append(buf, (size_t)n);
			size_t end = Job.header.find("\r\n\r\n");
			if (end == std::string::npos) {
				if (Job.header.size() > 16384) {
					Next_Mirror("a bad answer from " + Job.host);
					return;
				}
				continue;
			}
			std::string head = Job.header.substr(0, end);
			int status = 0;
			sscanf(head.c_str(), "HTTP/%*d.%*d %d", &status);
			fprintf(stderr, "launcher: %s: HTTP %d\n", Job.host.c_str(), status);
			auto field = [&](char const * name) -> std::string {
				std::string lower = head;
				for (auto & c : lower) c = (char)tolower((unsigned char)c);
				std::string key = std::string("\r\n") + name + ":";
				size_t at = lower.find(key);
				if (at == std::string::npos) return "";
				size_t v = at + key.size();
				size_t e = head.find("\r\n", v);
				std::string value = head.substr(v, e == std::string::npos ? std::string::npos : e - v);
				while (!value.empty() && value[0] == ' ') value.erase(0, 1);
				return value;
			};
			if ((status == 301 || status == 302 || status == 303 || status == 307 || status == 308) && Job.redirects < 5) {
				std::string location = field("location");
				close(Job.fd);
				Job.fd = -1;
				Job.redirects++;
				if (location.compare(0, 7, "http://") != 0) {
					Next_Mirror("it moved to " + location + ", which this can't fetch");
					return;
				}
				Connect(location);
				return;
			}
			if (status != 200) {
				char why[64];
				snprintf(why, sizeof(why), "%s answered %d", Job.host.c_str(), status);
				Next_Mirror(why);
				return;
			}
			Job.length = atof(field("content-length").c_str());
			if (Job.length != RunSize) {
				Next_Mirror(Job.host + " offered a file of another size");
				return;
			}
			Job.out = Open_Big(Run_File(), "wb", BUF_OUT);
			if (Job.out == nullptr) {
				Fail("Couldn't write the download in !ioquake3.download (" + std::string(strerror(errno)) + ").", false);
				return;
			}
			Sha256_Init(Job.sha);
			Job.received = 0;
			JobStage = BODY;
			Job.started = Now();
			size_t body = end + 4;
			p = Job.header.data() + body;
			left = Job.header.size() - body;
			if (left > 0) {
				// (a copy: Job.header is cleared below)
				std::string first(p, left);
				Job.header.clear();
				fwrite(first.data(), 1, first.size(), Job.out);
				Sha256_Update(Job.sha, first.data(), first.size());
				Job.received += first.size();
			}
			continue;
		}
		if (fwrite(p, 1, left, Job.out) != left) {
			Fail("Couldn't write the download: the disc may be full.", true);
			return;
		}
		Sha256_Update(Job.sha, p, left);
		Job.received += left;
		if (Job.received >= Job.length) {
			Show_Progress(Speed_Text(), 1);
			Download_Finished();
			return;
		}
	}
	if (JobStage == BODY) {
		Show_Progress(Speed_Text(), Job.received / RunSize);
	}
}

/* A file dropped on the icon: hash it, then install from it as from a download. */
void Start_From_File(std::string const & riscos_path)
{
	Job = JobState();
	// a RISC OS name (its '/'s are the Unix name's dots): UnixLib wants it in Unix form
	char unix_name[1024];
	if (__unixify_std(riscos_path.c_str(), unix_name, sizeof(unix_name), 0) == nullptr) {
		Tell("Couldn't open " + riscos_path + ".", true);
		return;
	}
	Job.in = Open_Big(unix_name, "rb", BUF_IN);
	if (Job.in == nullptr) {
		Tell("Couldn't open " + riscos_path + ".", true);
		return;
	}
	fseek(Job.in, 0, SEEK_END);
	double size = (double)ftell(Job.in);
	fseek(Job.in, 0, SEEK_SET);
	if (size != RunSize) {
		fclose(Job.in);
		Job.in = nullptr;
		Tell("That isn't the Quake III Arena demo installer (" + std::string(DemoLeaf) + ", " + Megabytes(RunSize)
			+ "). Click the icon to download it instead.", true);
		return;
	}
	Show_Message("Checking the Quake III Arena demo installer...", false, {{"Cancel", ACT_CANCEL_JOB}}, true);
	Sha256_Init(Job.sha);
	Job.done = 0;
	JobStage = HASHING;
}

void Hash_Tick(void)
{
	static unsigned char buf[256 * 1024];
	unsigned until = Now() + 5;
	while ((int)(until - Now()) > 0) {
		size_t n = fread(buf, 1, sizeof(buf), Job.in);
		if (n == 0) {
			char hex[65];
			Sha256_Hex(Job.sha, hex);
			if (strcmp(hex, RunSha256) != 0) {
				Fail("That file isn't an intact copy of the Quake III Arena demo installer (its SHA-256 doesn't match).", false);
				return;
			}
			Begin_Archive();	// reads Job.in from the archive's start
			return;
		}
		Sha256_Update(Job.sha, buf, n);
		Job.done += n;
	}
	Show_Progress("Checked " + Megabytes(Job.done) + " of " + Megabytes(RunSize), Job.done / RunSize);
}

/* The archive: gzip, then tar. Only Help/Q3A_EULA.txt and demoq3/pak0.pk3 are taken. */
void Begin_Archive(void)
{
	if (Job.in == nullptr) {
		Job.in = Open_Big(Run_File(), "rb", BUF_IN);
		if (Job.in == nullptr) {
			Fail("Couldn't read the downloaded installer back.", true);
			return;
		}
	}
	fseek(Job.in, RunArchiveOffset, SEEK_SET);
	memset(&Job.z, 0, sizeof(Job.z));
	if (inflateInit2(&Job.z, 16 + MAX_WBITS) != Z_OK) {
		Fail("Couldn't start unpacking the installer.", true);
		return;
	}
	Job.z_open = true;
	Job.hdr_have = 0;
	Job.entry_left = Job.pad_left = 0;
	Job.entry_kind = 0;
	Job.got_licence = Job.got_pk4 = false;
	Job.done = 0;
	mkdir(App_Path("demoq3").c_str(), 0755);
	JobStage = LICENCE_EXTRACT;
	Show_Message("The download is intact. Unpacking id's licence for the demo...", false, {{"Cancel", ACT_CANCEL_JOB}},
		true);
}

void Set_Type(std::string const & unix_path, int type)
{
	char riscos[512];
	if (__riscosify_std(unix_path.c_str(), 0, riscos, sizeof(riscos), nullptr) == nullptr) {
		return;
	}
	_kernel_swi_regs regs;
	regs.r[0] = 18;
	regs.r[1] = (int)riscos;
	regs.r[2] = type;
	_kernel_swi(OS_File, &regs, &regs);
}

std::string Riscos_Name(std::string const & unix_path)
{
	char riscos[512];
	if (__riscosify_std(unix_path.c_str(), 0, riscos, sizeof(riscos), nullptr) == nullptr) {
		return unix_path;
	}
	return riscos;
}

unsigned Raise_Question_At = 0;	// when to bring the question back in front (0: no need)

void Raise_Question(void)
{
	Raise_Question_At = 0;
	if (MessageWindow < 0) {
		return;
	}
	int state[9];
	state[0] = MessageWindow;
	_kernel_swi_regs regs;
	regs.r[1] = (int)state;
	if (Swi(Wimp_GetWindowState, regs)) {
		state[7] = -1;	// in front
		Swi(Wimp_OpenWindow, regs);
	}
}

void Licence_Ready(void)
{
	Set_Type(App_Path(LicenceFile), 0xFFF);
	JobStage = LICENCE_WAIT;
	Show_Message("id Software's licence for the Quake III Arena demo is open in your text editor (it is "
		"!ioquake3.demoq3.Q3A_EULA/txt). The demo is installed only if you accept it.\n"
		"Do you accept id's licence?", false,
		{{"Decline", ACT_DECLINE}, {"Show licence", ACT_LICENCE}, {"Accept", ACT_ACCEPT}});
	Oscli("Filer_Run " + Riscos_Name(App_Path(LicenceFile)));
	Raise_Question_At = Now() + 150;	// the editor opens later, often over the question
}

void Install_Finished(void);

// Feeds unpacked bytes through the tar reader. Returns false to stop (paused or finished).
bool Tar_Feed(unsigned char const * p, size_t n)
{
	while (n > 0) {
		if (Job.entry_left > 0) {
			size_t take = (double)n < Job.entry_left ? n : (size_t)Job.entry_left;
			if (Job.entry_out != nullptr) {
				if (fwrite(p, 1, take, Job.entry_out) != take) {
					Fail("Couldn't write the demo's files: the disc may be full.", true);
					return false;
				}
				Sha256_Update(Job.entry_sha, p, take);
			}
			p += take; n -= take;
			Job.entry_left -= take;
			if (Job.entry_kind == 2) {
				Job.done += take;
			}
			if (Job.entry_left == 0 && Job.entry_kind != 0) {
				char hex[65];
				Sha256_Hex(Job.entry_sha, hex);
				fclose(Job.entry_out);
				Job.entry_out = nullptr;
				if (Job.entry_kind == 1) {
					if (strcmp(hex, LicenceSha256) != 0) {
						Fail("The installer's licence file isn't the one expected.", true);
						return false;
					}
					Job.got_licence = true;
					Job.entry_kind = 0;
					Licence_Ready();
					// the rest of this block is kept for later: 'p, n' are lost, so stop here only
					// at a block boundary (see below)
				} else {
					if (strcmp(hex, Pk4Sha256) != 0) {
						Fail("The demo's data file didn't unpack intact (its SHA-256 doesn't match).", true);
						return false;
					}
					Job.got_pk4 = true;
					Job.entry_kind = 0;
					Install_Finished();
					return false;
				}
			}
			continue;
		}
		if (Job.pad_left > 0) {
			size_t take = (double)n < Job.pad_left ? n : (size_t)Job.pad_left;
			p += take; n -= take;
			Job.pad_left -= take;
			continue;
		}
		size_t take = 512 - Job.hdr_have < n ? 512 - Job.hdr_have : n;
		memcpy(Job.hdr + Job.hdr_have, p, take);
		Job.hdr_have += take; p += take; n -= take;
		if (Job.hdr_have < 512) {
			continue;
		}
		Job.hdr_have = 0;
		if (Job.hdr[0] == 0) {	// the end
			Fail("The installer didn't hold the demo's data file.", true);
			return false;
		}
		char name[257];
		memcpy(name, Job.hdr, 100);
		name[100] = '\0';
		if (memcmp(Job.hdr + 257, "ustar", 5) == 0 && Job.hdr[345]) {
			char prefix[156];
			memcpy(prefix, Job.hdr + 345, 155);
			prefix[155] = '\0';
			std::string full = std::string(prefix) + "/" + name;
			snprintf(name, sizeof(name), "%s", full.c_str());
		}
		char const * leaf = strncmp(name, "./", 2) == 0 ? name + 2 : name;
		char size_text[13];
		memcpy(size_text, Job.hdr + 124, 12);
		size_text[12] = '\0';
		double size = (double)strtoull(size_text, nullptr, 8);
		char kind = (char)Job.hdr[156];
		Job.entry_left = (kind == '0' || kind == 0) ? size : 0;
		Job.pad_left = Job.entry_left > 0 ? (512 - ((unsigned long long)size % 512)) % 512 : 0;
		Job.entry_kind = 0;
		if (Job.entry_left > 0 && strcmp(leaf, LicenceName) == 0 && !Job.got_licence) {
			Job.entry_kind = 1;
			Job.entry_out = Open_Big(App_Path(LicenceFile), "wb", BUF_ENTRY);
		} else if (Job.entry_left > 0 && strcmp(leaf, Pk4Name) == 0 && Job.got_licence && JobStage == INSTALLING) {
			if (size != Pk4Size) {
				Fail("The demo's data file in the installer is the wrong size.", true);
				return false;
			}
			Job.entry_kind = 2;
			Job.entry_out = Open_Big(Pk4_Part(), "wb", BUF_ENTRY);
		}
		if (Job.entry_kind != 0) {
			if (Job.entry_out == nullptr) {
				Fail("Couldn't write the demo's files in !ioquake3.demoq3 (" + std::string(strerror(errno)) + ").", true);
				return false;
			}
			Sha256_Init(Job.entry_sha);
		}
	}
	return true;
}

// Bytes unpacked but not yet fed to the tar reader (kept across the licence question).
static unsigned char Unpacked[256 * 1024];
static size_t UnpackedAt = 0, UnpackedEnd = 0;

void Archive_Tick(void)
{
	static unsigned char packed[256 * 1024];
	unsigned until = Now() + 5;
	while ((int)(until - Now()) > 0) {
		if (UnpackedAt < UnpackedEnd) {
			// feed in 512-byte steps, so that stopping for the licence loses nothing
			size_t const most = Job.got_licence ? UnpackedEnd - UnpackedAt : 512;
			size_t step = UnpackedEnd - UnpackedAt < most ? UnpackedEnd - UnpackedAt : most;
			Stage before = JobStage;
			bool go_on = Tar_Feed(Unpacked + UnpackedAt, step);
			UnpackedAt += step;
			if (!go_on || JobStage != before) {
				return;
			}
			continue;
		}
		if (Job.z.avail_in == 0) {
			size_t got = fread(packed, 1, sizeof(packed), Job.in);
			if (got == 0) {
				Fail("The installer ended early.", true);
				return;
			}
			Job.z.next_in = packed;
			Job.z.avail_in = (uInt)got;
		}
		Job.z.next_out = Unpacked;
		Job.z.avail_out = sizeof(Unpacked);
		int r = inflate(&Job.z, Z_NO_FLUSH);
		if (r != Z_OK && r != Z_STREAM_END) {
			Fail("The installer couldn't be unpacked (zlib error).", true);
			return;
		}
		UnpackedAt = 0;
		UnpackedEnd = sizeof(Unpacked) - Job.z.avail_out;
		if (r == Z_STREAM_END && UnpackedEnd == 0) {
			Fail("The installer didn't hold the demo's data file.", true);
			return;
		}
	}
	if (JobStage == INSTALLING) {
		Show_Progress("Installed " + Megabytes(Job.done) + " of " + Megabytes(Pk4Size), Job.done / Pk4Size);
	}
}

void Accept_Licence(void)
{
	JobStage = INSTALLING;
	Job.done = 0;
	Show_Message("Installing the Quake III Arena demo's data (demoq3.pak0/pk3)...", false, {{"Cancel", ACT_CANCEL_JOB}}, true);
}

void Start_Game(void);

void Install_Finished(void)
{
	Close_Job_Files();
	rename(Pk4_Part().c_str(), App_Path(Pk4Name).c_str());
	remove(Run_File().c_str());
	rmdir(App_Path("download").c_str());
	JobStage = IDLE;
	fprintf(stderr, "launcher: the demo is installed\n");
	if (!Has_Game_Data()) {
		Tell("The demo's data was unpacked but isn't where it should be (!ioquake3.demoq3.pak0/pk3).", true);
		return;
	}
	Show_Message("The Quake III Arena demo is installed. Starting the game...", false, {{"OK", ACT_CLOSE}});
	Start_Game();
}

/* --- the game ---------------------------------------------------------------------------- */

// !Run again: it finds the data now and runs the game, which has its own icon, so this quits.
void Start_Game(void)
{
	if (GameStarted) {
		return;
	}
	std::string command = "Run " + Os_Variable("ioquake3$Dir") + ".!Run " + GameArgs;
	fprintf(stderr, "launcher: starting the game: %s\n", command.c_str());
	_kernel_swi_regs regs;
	regs.r[0] = (int)command.c_str();
	if (!Swi(Wimp_StartTask, regs)) {
		Tell("The game couldn't be started.", true);
		return;
	}
	GameStarted = true;
	Close_Message();
}

void Icon_Clicked(void)
{
	if (JobStage == LICENCE_WAIT) {
		Licence_Ready();
		return;
	}
	if (JobStage != IDLE) {
		return;	// its window is open
	}
	if (Has_Game_Data()) {
		Start_Game();
		return;
	}
	Show_Message("ioquake3 needs game data. Download the free Quake III Arena demo?\n"
		"It is id Software's own Linux installer (" + Megabytes(RunSize) + "), fetched from a mirror of id's "
		"FTP site and checked. You'll be asked to accept id's licence before it is installed; the demo takes "
		+ Megabytes(Pk4Size) + " in !ioquake3.\n"
		"To play the full game or OpenArena instead, put its baseq3 or baseoa folder in !ioquake3 (see Help).",
		false, {{"Cancel", ACT_CLOSE}, {"Help", ACT_HELP}, {"Download", ACT_DOWNLOAD}});
}

void Message_Button(int button)
{
	Action action = button >= 0 && button < ButtonCount ? ButtonAction[button] : ACT_CLOSE;
	if (button < 0) {	// the close icon: as the first button (Cancel/Decline)
		action = ButtonAction[0];
	}
	switch (action) {
		case ACT_CLOSE:
			Close_Message();
			break;
		case ACT_HELP:
			Open_Help();
			break;
		case ACT_DOWNLOAD:
			Start_Download();
			break;
		case ACT_CANCEL_JOB:
			Abandon(true);
			Close_Message();
			break;
		case ACT_LICENCE:
			Oscli("Filer_Run " + Riscos_Name(App_Path(LicenceFile)));
			break;
		case ACT_DECLINE:
			Abandon(true);
			Tell("Nothing was installed, and the download has been deleted.", false);
			break;
		case ACT_ACCEPT:
			Accept_Licence();
			break;
		case ACT_PLAY:
			Start_Game();
			break;
	}
}

void Data_Load(int * block)
{
	char const * path = (char const *)&block[11];
	std::string file = path;
	// acknowledge it (Message_DataLoadAck)
	block[3] = block[2];
	block[4] = 4;
	_kernel_swi_regs regs;
	regs.r[0] = 17;
	regs.r[1] = (int)block;
	regs.r[2] = block[1];
	Swi(Wimp_SendMessage, regs);
	if (JobStage != IDLE) {
		Tell("Wait for the download or install to finish first.", false);
		return;
	}
	Start_From_File(file);
}

void Poll_Loop(void)
{
	int block[64];
	while (!GameStarted) {
		bool busy = (JobStage != IDLE && JobStage != LICENCE_WAIT) || Raise_Question_At != 0;
		_kernel_swi_regs regs;
		regs.r[0] = busy ? 0 : 1;	// null events only while there's work
		regs.r[1] = (int)block;
		HandlerType own[Swapped_Count];
		Use_Default_Handlers(own);
		_kernel_oserror * error = _kernel_swi(Wimp_Poll, &regs, &regs);
		Use_Own_Handlers(own);
		if (error != nullptr) {
			fprintf(stderr, "launcher: Wimp_Poll: %s\n", error->errmess);
			return;
		}
		switch (regs.r[0]) {
			case 0:	// Null_Reason
				if (Raise_Question_At != 0 && (int)(Now() - Raise_Question_At) >= 0) {
					Raise_Question();
				}
				switch (JobStage) {
					case CONNECTING: case HEADERS: case BODY: Download_Tick(); break;
					case HASHING: Hash_Tick(); break;
					case LICENCE_EXTRACT: case INSTALLING: Archive_Tick(); break;
					default: break;
				}
				break;
			case 2:	// Open_Window_Request
				regs.r[1] = (int)block;
				Swi(Wimp_OpenWindow, regs);
				break;
			case 3:	// Close_Window_Request
				if (block[0] == MessageWindow) {
					Message_Button(-1);
					break;
				}
				regs.r[1] = (int)block;
				Swi(Wimp_CloseWindow, regs);
				break;
			case 8:	// Key_Pressed
				if (block[0] == MessageWindow && (block[6] == 13 || block[6] == 27)) {
					Message_Button(block[6] == 13 ? ButtonCount - 1 : 0);
					break;
				}
				regs.r[0] = block[6];
				Swi(Wimp_ProcessKey, regs);
				break;
			case 6:	// Mouse_Click
				if (block[3] == MessageWindow && block[4] >= 0 && block[4] < ButtonCount && (block[2] & (4 | 1))) {
					Message_Button(block[4]);
					break;
				}
				if (block[3] == -2 && block[4] == IconHandle) {
					if (block[2] & 2) {
						Open_Menu(block[0]);
					} else if (block[2] & (4 | 1)) {
						Icon_Clicked();
					}
				}
				break;
			case 9:	// Menu_Selection
				if (block[0] == MenuQuit) {
					Abandon(false);
					return;
				}
				if (block[0] == MenuHelp) {
					Open_Help();
				}
				{
					int pointer[5];
					regs.r[1] = (int)pointer;
					if (Swi(Wimp_GetPointerInfo, regs) && (pointer[2] & 1)) {
						Open_Menu(pointer[0] + 64);
					}
				}
				break;
			case 17: case 18:	// User_Message(_Recorded)
				if (block[4] == 0) {	// Message_Quit
					Abandon(false);
					return;
				}
				if (block[4] == 3 && block[5] == -2 && block[6] == IconHandle) {	// Message_DataLoad
					Data_Load(block);
				}
				break;
			default:
				break;
		}
	}
}

}	// namespace

int main(int argc, char ** argv)
{
	// the arguments go to the game
	for (int i = 1; i < argc; i++) {
		GameArgs += std::string(argv[i]) + " ";
	}
	int messages[] = {3, 0};
	_kernel_swi_regs regs;
	regs.r[0] = 380;
	regs.r[1] = 0x4B534154;
	regs.r[2] = (int)"ioquake3 launcher";
	regs.r[3] = (int)messages;
	if (!Swi(Wimp_Initialise, regs)) {
		return 1;
	}
	TaskHandle = regs.r[1];
	Create_Info_Window();
	Build_Menu();
	Put_Icon_On_Bar();
	fprintf(stderr, "launcher: %s on the icon bar; game data %s\n", LAUNCHER_VERSION,
		Has_Game_Data() ? "installed" : "not installed");
	// !Run only runs this without data: offer the demo at once
	Icon_Clicked();
	Poll_Loop();
	regs.r[0] = TaskHandle;
	regs.r[1] = 0x4B534154;
	_kernel_swi(Wimp_CloseDown, &regs, &regs);
	return 0;
}
