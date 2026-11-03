#include <pspuser.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psputility_netmodules.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("PSP CAR DASH", 0, 3, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define RGB(r,g,b) (0xFF000000 | ((b)<<16) | ((g)<<8) | (r))
#define C_BLACK    RGB(0,0,0)
#define C_WHITE    RGB(240,240,240)
#define C_GOLD     RGB(235,178,64)
#define C_DIM      RGB(130,130,130)
#define C_GREEN    RGB(70,220,110)
#define C_RED      RGB(245,75,65)
#define C_BLUE     RGB(90,175,255)

#define UDP_PORT 4210

typedef struct {
    int speed_kmh;
    int rpm;
    int coolant_c;
    int throttle_pct;
    int voltage_tenths;
    int intake_c;
    int load_pct;
    int fuel_tenths;
    int oil_temp_c;
    int oil_pressure_tenths;
    int connected;
    unsigned int seq;
} VehicleData;

typedef enum {
    SRC_DEMO = 0,
    SRC_ESP32 = 1
} DataSource;

static volatile int g_running = 1;
static int g_page = 0;
static int g_auto_demo = 1;
static int g_freeze = 0;
static unsigned int g_frame = 0;
static int g_page_dirty = 1;

static DataSource g_source = SRC_DEMO;
static VehicleData g_data;

static int g_wifi_profile = 1;
static int g_net_modules_loaded = 0;
static int g_net_initialized = 0;
static int g_ap_connected = 0;
static int g_udp_sock = -1;
static int g_ap_state = 0;
static int g_last_net_error = 0;
static unsigned int g_last_packet_frame = 0;
static unsigned int g_packet_count = 0;
static char g_psp_ip[16] = "0.0.0.0";

static int exit_callback(int arg1, int arg2, void *common)
{
    g_running = 0;
    sceKernelExitGame();
    return 0;
}

static int callback_thread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void)
{
    int thid = sceKernelCreateThread(
        "callback_thread",
        callback_thread,
        0x11,
        0xFA0,
        0,
        0
    );

    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);
}

static void linef(int x, int y, unsigned int color, const char *fmt, ...)
{
    char tmp[128];
    char out[61];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    memset(out, ' ', 60);
    out[60] = '\0';

    int n = (int)strlen(tmp);
    if (n > 60 - x)
        n = 60 - x;

    if (n > 0)
        memcpy(out + x, tmp, n);

    pspDebugScreenSetXY(0, y);
    pspDebugScreenSetTextColor(color);
    pspDebugScreenPrintf("%s", out);
}

static void bar_text(char *out, int width, int value, int max)
{
    if (value < 0) value = 0;
    if (value > max) value = max;

    int fill = (value * width) / max;

    out[0] = '[';
    for (int i = 0; i < width; ++i)
        out[1 + i] = (i < fill) ? '#' : '.';
    out[1 + width] = ']';
    out[2 + width] = '\0';
}

static const char *big_digit[10][5] = {
    {" _ ","| |","| |","| |","|_|"},
    {"   ","  |","  |","  |","  |"},
    {" _ ","  |"," _|","|  ","|_ "},
    {" _ ","  |"," _|","  |"," _|"},
    {"   ","| |","|_|","  |","  |"},
    {" _ ","|  ","|_ ","  |"," _|"},
    {" _ ","|  ","|_ ","| |","|_|"},
    {" _ ","  |","  |","  |","  |"},
    {" _ ","| |","|_|","| |","|_|"},
    {" _ ","| |","|_|","  |"," _|"}
};

static void draw_big_speed(int speed)
{
    if (speed < 0) speed = 0;
    if (speed > 999) speed = 999;

    int d0 = (speed / 100) % 10;
    int d1 = (speed / 10) % 10;
    int d2 = speed % 10;

    for (int row = 0; row < 5; ++row) {
        char s[24];

        if (speed >= 100) {
            snprintf(s, sizeof(s), "%s %s %s",
                     big_digit[d0][row], big_digit[d1][row], big_digit[d2][row]);
        } else if (speed >= 10) {
            snprintf(s, sizeof(s), "    %s %s",
                     big_digit[d1][row], big_digit[d2][row]);
        } else {
            snprintf(s, sizeof(s), "        %s", big_digit[d2][row]);
        }

        linef(22, 7 + row, C_GOLD, "%s", s);
    }

    linef(27, 12, C_WHITE, "km/h");
}

static int net_live(void)
{
    if (g_source != SRC_ESP32)
        return 0;

    if (!g_ap_connected || g_udp_sock < 0)
        return 0;

    if ((g_frame - g_last_packet_frame) > 120)
        return 0;

    return 1;
}

static void draw_header(const char *page_name)
{
    linef(0, 0, C_GOLD, "============================================================");
    linef(1, 1, C_WHITE, "PSP CAR DASH  W210 E430 / M113 V8 4.3L");
    linef(1, 2, C_GOLD, "BLACK W210  |  US JAMES");
    linef(0, 3, C_GOLD, "------------------------------------------------------------");
    linef(1, 4, C_BLUE, "%s", page_name);

    if (g_source == SRC_ESP32) {
        linef(38, 4, net_live() ? C_GREEN : C_RED,
              "%s", net_live() ? "ESP32 LIVE" : "ESP32 WAIT");
    } else {
        linef(44, 4, C_GREEN, "DEMO");
    }
}

static void draw_footer(void)
{
    linef(0, 30, C_GOLD, "------------------------------------------------------------");
    linef(1, 31, C_WHITE, "< L/R PAGE  START DEMO  SELECT FREEZE  TRIANGLE ESP32  O EXIT>");
    linef(1, 32, C_DIM, "WIFI PROFILE: %d   L/R TRIGGER CHANGE WHEN DISCONNECTED", g_wifi_profile);
}

static void static_drive(void)
{
    pspDebugScreenClear();
    draw_header("DRIVE");
    linef(1, 14, C_WHITE, "RPM");
    linef(1, 17, C_WHITE, "THROTTLE");
    linef(1, 20, C_WHITE, "COOLANT");
    linef(1, 23, C_WHITE, "BATTERY");
    linef(1, 26, C_DIM, "W210 E430  BLACK / US JAMES");
    draw_footer();
}

static void dynamic_drive(const VehicleData *d)
{
    char b1[32], b2[32];

    draw_big_speed(d->speed_kmh);

    bar_text(b1, 28, d->rpm, 6500);
    linef(12, 14, C_GOLD, "%4d r/min  %s", d->rpm, b1);

    bar_text(b2, 28, d->throttle_pct, 100);
    linef(12, 17, C_GOLD, "%3d %%      %s", d->throttle_pct, b2);

    linef(12, 20, d->coolant_c >= 105 ? C_RED : C_GREEN,
          "%3d C      %s",
          d->coolant_c,
          d->coolant_c >= 105 ? "HIGH" : "NORMAL");

    linef(12, 23, C_GREEN, "%2d.%d V",
          d->voltage_tenths / 10,
          d->voltage_tenths % 10);

    if (g_source == SRC_ESP32)
        linef(39, 23, net_live() ? C_GREEN : C_RED, "SEQ %u", d->seq);
    else
        linef(39, 23, C_DIM, "DEMO");
}

static void static_engine(void)
{
    pspDebugScreenClear();
    draw_header("ENGINE");
    linef(1, 7, C_WHITE, "ENGINE");
    linef(19, 7, C_GOLD, "Mercedes-Benz M113 E43");
    linef(1, 9, C_WHITE, "TYPE");
    linef(19, 9, C_GOLD, "V8 4.3L GASOLINE");
    linef(1, 11, C_WHITE, "DISPLACEMENT");
    linef(19, 11, C_GOLD, "4266 cc");

    linef(1, 15, C_WHITE, "RPM");
    linef(1, 17, C_WHITE, "ENGINE LOAD");
    linef(1, 19, C_WHITE, "THROTTLE");
    linef(1, 21, C_WHITE, "COOLANT");
    linef(1, 23, C_WHITE, "INTAKE");
    linef(1, 25, C_WHITE, "OIL TEMP");
    linef(1, 27, C_WHITE, "OIL PRESSURE");
    draw_footer();
}

static void dynamic_engine(const VehicleData *d)
{
    char b[28];

    linef(19, 15, C_GOLD, "%4d r/min", d->rpm);

    bar_text(b, 24, d->load_pct, 100);
    linef(19, 17, C_GOLD, "%3d %%  %s", d->load_pct, b);

    linef(19, 19, C_GOLD, "%3d %%", d->throttle_pct);
    linef(19, 21, d->coolant_c >= 105 ? C_RED : C_GREEN, "%3d C", d->coolant_c);
    linef(19, 23, C_WHITE, "%3d C", d->intake_c);
    linef(19, 25, C_WHITE, "%3d C", d->oil_temp_c);
    linef(19, 27, C_WHITE, "%d.%d bar",
          d->oil_pressure_tenths / 10,
          d->oil_pressure_tenths % 10);
}

static void static_trip(void)
{
    pspDebugScreenClear();
    draw_header("TRIP");
    linef(1, 8, C_WHITE, "TRIP TIME");
    linef(1, 11, C_WHITE, "DISTANCE");
    linef(1, 14, C_WHITE, "CURRENT SPEED");
    linef(1, 17, C_WHITE, "MAX SPEED");
    linef(1, 20, C_WHITE, "AVG SPEED");
    linef(1, 23, C_WHITE, "INSTANT FUEL");
    linef(1, 26, C_DIM, "0-100 timing reserved for validated vehicle data");
    draw_footer();
}

static void dynamic_trip(const VehicleData *d)
{
    unsigned int seconds = g_frame / 60;
    unsigned int minutes = seconds / 60;
    seconds %= 60;

    float km = ((float)d->speed_kmh * (float)g_frame) / (60.0f * 3600.0f);

    linef(22, 8, C_GOLD, "%02u:%02u", minutes % 100, seconds);
    linef(22, 11, C_GOLD, "%6.2f km", km);
    linef(22, 14, C_GOLD, "%3d km/h", d->speed_kmh);
    linef(22, 17, C_WHITE, "104 km/h");
    linef(22, 20, C_WHITE, "72 km/h");
    linef(22, 23, C_WHITE, "%d.%d L/100km",
          d->fuel_tenths / 10,
          d->fuel_tenths % 10);
}

static void static_diag(void)
{
    pspDebugScreenClear();
    draw_header("DIAGNOSTICS / ESP32 LINK");
    linef(1, 7, C_WHITE, "DISPLAY CORE");
    linef(1, 9, C_WHITE, "DATA SOURCE");
    linef(1, 11, C_WHITE, "WIFI PROFILE");
    linef(1, 13, C_WHITE, "AP STATE");
    linef(1, 15, C_WHITE, "PSP IP");
    linef(1, 17, C_WHITE, "UDP PORT");
    linef(1, 19, C_WHITE, "PACKETS");
    linef(1, 21, C_WHITE, "LAST ERROR");
    linef(1, 23, C_WHITE, "OBD-II");
    linef(1, 25, C_WHITE, "CAN / TWAI");
    linef(1, 27, C_WHITE, "SYSTEM VOLTAGE");
    draw_footer();
}

static void dynamic_diag(const VehicleData *d)
{
    linef(22, 7, C_GREEN, "ONLINE");
    linef(22, 9, g_source == SRC_ESP32 ? C_BLUE : C_GREEN,
          "%s", g_source == SRC_ESP32 ? "ESP32 UDP" : "LOCAL DEMO");
    linef(22, 11, C_WHITE, "%d", g_wifi_profile);
    linef(22, 13, g_ap_state == 4 ? C_GREEN : C_DIM, "%d / 4", g_ap_state);
    linef(22, 15, g_ap_connected ? C_GREEN : C_DIM, "%s", g_psp_ip);
    linef(22, 17, C_WHITE, "%d", UDP_PORT);
    linef(22, 19, net_live() ? C_GREEN : C_DIM, "%u", g_packet_count);
    linef(22, 21, g_last_net_error ? C_RED : C_GREEN, "0x%08X", g_last_net_error);
    linef(22, 23, C_DIM, "RESERVED FOR V0.4");
    linef(22, 25, C_DIM, "RESERVED FOR V0.4");
    linef(22, 27, C_GREEN, "%2d.%d V",
          d->voltage_tenths / 10,
          d->voltage_tenths % 10);
}

static void demo_update(VehicleData *d)
{
    d->speed_kmh = 86;
    d->rpm = 3250;
    d->coolant_c = 92;
    d->throttle_pct = 27;
    d->voltage_tenths = 142;
    d->intake_c = 31;
    d->load_pct = 38;
    d->fuel_tenths = 68;
    d->oil_temp_c = 96;
    d->oil_pressure_tenths = 38;
    d->connected = 0;
    d->seq = 0;

    if (!g_auto_demo)
        return;

    float t = (float)g_frame / 60.0f;

    d->speed_kmh = 78 + (int)(20.0f * sinf(t * 0.65f));
    d->rpm = 2700 + (int)(1050.0f * sinf(t * 1.05f));
    d->coolant_c = 92 + (int)(2.0f * sinf(t * 0.09f));
    d->throttle_pct = 29 + (int)(20.0f * sinf(t * 0.81f));
    d->voltage_tenths = 141 + (int)(2.0f * sinf(t * 0.17f));
    d->intake_c = 31 + (int)(2.0f * sinf(t * 0.21f));
    d->load_pct = 42 + (int)(18.0f * sinf(t * 0.73f));
    d->fuel_tenths = 70 + (int)(18.0f * sinf(t * 0.58f));
    d->oil_temp_c = 96 + (int)(2.0f * sinf(t * 0.11f));
    d->oil_pressure_tenths = 36 + (d->rpm / 1800);

    if (d->speed_kmh < 0) d->speed_kmh = 0;
    if (d->rpm < 700) d->rpm = 700;
    if (d->throttle_pct < 0) d->throttle_pct = 0;
    if (d->throttle_pct > 100) d->throttle_pct = 100;
    if (d->load_pct < 0) d->load_pct = 0;
    if (d->load_pct > 100) d->load_pct = 100;
}

static void show_connect_status(const char *msg, int color)
{
    linef(0, 29, color, "%s", msg);
}

static int network_stack_init(void)
{
    int r;

    if (!g_net_modules_loaded) {
        r = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
        if (r < 0) {
            g_last_net_error = r;
            return 0;
        }

        r = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
        if (r < 0) {
            g_last_net_error = r;
            return 0;
        }

        g_net_modules_loaded = 1;
    }

    if (!g_net_initialized) {
        r = sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024);
        if (r < 0) {
            g_last_net_error = r;
            return 0;
        }

        r = sceNetInetInit();
        if (r < 0) {
            g_last_net_error = r;
            return 0;
        }

        r = sceNetApctlInit(0x8000, 48);
        if (r < 0) {
            g_last_net_error = r;
            return 0;
        }

        g_net_initialized = 1;
    }

    return 1;
}

static void network_disconnect(void)
{
    if (g_udp_sock >= 0) {
        close(g_udp_sock);
        g_udp_sock = -1;
    }

    if (g_ap_connected || g_ap_state != 0)
        sceNetApctlDisconnect();

    g_ap_connected = 0;
    g_ap_state = 0;
    strcpy(g_psp_ip, "0.0.0.0");
    g_source = SRC_DEMO;
    g_page_dirty = 1;
}

static int network_connect(void)
{
    int r;

    show_connect_status("ESP32 LINK: INITIALIZING NETWORK...", C_BLUE);

    if (!network_stack_init()) {
        show_connect_status("ESP32 LINK: NETWORK INIT FAILED", C_RED);
        return 0;
    }

    r = sceNetApctlConnect(g_wifi_profile);
    if (r < 0) {
        g_last_net_error = r;
        show_connect_status("ESP32 LINK: APCTL CONNECT FAILED", C_RED);
        return 0;
    }

    g_ap_state = 0;

    for (int i = 0; i < 200 && g_running; ++i) {
        r = sceNetApctlGetState(&g_ap_state);

        if (r < 0) {
            g_last_net_error = r;
            show_connect_status("ESP32 LINK: APCTL STATE FAILED", C_RED);
            return 0;
        }

        char msg[64];
        snprintf(msg, sizeof(msg),
                 "ESP32 LINK: PROFILE %d   AP STATE %d/4",
                 g_wifi_profile, g_ap_state);
        show_connect_status(msg, g_ap_state == 4 ? C_GREEN : C_BLUE);

        if (g_ap_state == 4)
            break;

        sceKernelDelayThread(50 * 1000);
    }

    if (g_ap_state != 4) {
        g_last_net_error = -1001;
        show_connect_status("ESP32 LINK: TIMEOUT - CHECK WIFI PROFILE", C_RED);
        sceNetApctlDisconnect();
        return 0;
    }

    g_ap_connected = 1;

    union SceNetApctlInfo info;
    memset(&info, 0, sizeof(info));

    if (sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0) {
        strncpy(g_psp_ip, info.ip, sizeof(g_psp_ip) - 1);
        g_psp_ip[sizeof(g_psp_ip) - 1] = '\0';
    }

    g_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_udp_sock < 0) {
        g_last_net_error = g_udp_sock;
        show_connect_status("ESP32 LINK: UDP SOCKET FAILED", C_RED);
        network_disconnect();
        return 0;
    }

    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(UDP_PORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    r = bind(g_udp_sock, (struct sockaddr *)&local, sizeof(local));
    if (r < 0) {
        g_last_net_error = r;
        show_connect_status("ESP32 LINK: UDP BIND FAILED", C_RED);
        network_disconnect();
        return 0;
    }

    g_last_net_error = 0;
    g_packet_count = 0;
    g_last_packet_frame = g_frame;
    g_source = SRC_ESP32;
    g_page_dirty = 1;

    show_connect_status("ESP32 LINK: CONNECTED - WAITING FOR UDP 4210", C_GREEN);
    return 1;
}

static int parse_packet(const char *buf, VehicleData *out)
{
    unsigned int seq;
    int speed, rpm, coolant, throttle, voltage10;
    int intake, load, fuel10, oiltemp, oilpress10;

    int n = sscanf(
        buf,
        "PCD1,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
        &seq,
        &speed,
        &rpm,
        &coolant,
        &throttle,
        &voltage10,
        &intake,
        &load,
        &fuel10,
        &oiltemp,
        &oilpress10
    );

    if (n != 11)
        return 0;

    if (speed < 0 || speed > 300) return 0;
    if (rpm < 0 || rpm > 8000) return 0;
    if (coolant < -40 || coolant > 150) return 0;
    if (throttle < 0 || throttle > 100) return 0;
    if (voltage10 < 60 || voltage10 > 180) return 0;
    if (load < 0 || load > 100) return 0;

    out->seq = seq;
    out->speed_kmh = speed;
    out->rpm = rpm;
    out->coolant_c = coolant;
    out->throttle_pct = throttle;
    out->voltage_tenths = voltage10;
    out->intake_c = intake;
    out->load_pct = load;
    out->fuel_tenths = fuel10;
    out->oil_temp_c = oiltemp;
    out->oil_pressure_tenths = oilpress10;
    out->connected = 1;

    return 1;
}

static void network_poll(void)
{
    if (g_source != SRC_ESP32 || g_udp_sock < 0)
        return;

    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(g_udp_sock, &rfds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int r = select(g_udp_sock + 1, &rfds, NULL, NULL, &tv);
    if (r <= 0)
        return;

    if (!FD_ISSET(g_udp_sock, &rfds))
        return;

    char buf[192];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    int len = recvfrom(
        g_udp_sock,
        buf,
        sizeof(buf) - 1,
        0,
        (struct sockaddr *)&from,
        &fromlen
    );

    if (len <= 0)
        return;

    buf[len] = '\0';

    VehicleData tmp = g_data;

    if (parse_packet(buf, &tmp)) {
        g_data = tmp;
        g_packet_count++;
        g_last_packet_frame = g_frame;
    }
}

static void render_static(void)
{
    switch (g_page) {
        case 0: static_drive(); break;
        case 1: static_engine(); break;
        case 2: static_trip(); break;
        default: static_diag(); break;
    }

    g_page_dirty = 0;
}

static void render_dynamic(const VehicleData *d)
{
    switch (g_page) {
        case 0: dynamic_drive(d); break;
        case 1: dynamic_engine(d); break;
        case 2: dynamic_trip(d); break;
        default: dynamic_diag(d); break;
    }
}

int main(void)
{
    setup_callbacks();

    pspDebugScreenInit();
    pspDebugScreenSetBackColor(C_BLACK);
    pspDebugScreenClear();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    demo_update(&g_data);

    SceCtrlData pad = {0};
    SceCtrlData oldpad = {0};

    unsigned int render_div = 0;

    while (g_running) {
        sceCtrlPeekBufferPositive(&pad, 1);
        unsigned int pressed = pad.Buttons & ~oldpad.Buttons;

        if (pressed & PSP_CTRL_RIGHT) {
            g_page = (g_page + 1) % 4;
            g_page_dirty = 1;
        }

        if (pressed & PSP_CTRL_LEFT) {
            g_page = (g_page + 3) % 4;
            g_page_dirty = 1;
        }

        if (pressed & PSP_CTRL_START) {
            g_auto_demo = !g_auto_demo;
        }

        if (pressed & PSP_CTRL_SELECT) {
            g_freeze = !g_freeze;
        }

        if (pressed & PSP_CTRL_LTRIGGER) {
            if (g_source == SRC_DEMO) {
                g_wifi_profile--;
                if (g_wifi_profile < 1) g_wifi_profile = 10;
                g_page_dirty = 1;
            }
        }

        if (pressed & PSP_CTRL_RTRIGGER) {
            if (g_source == SRC_DEMO) {
                g_wifi_profile++;
                if (g_wifi_profile > 10) g_wifi_profile = 1;
                g_page_dirty = 1;
            }
        }

        if (pressed & PSP_CTRL_TRIANGLE) {
            if (g_source == SRC_ESP32) {
                network_disconnect();
            } else {
                network_connect();
            }
        }

        if (pressed & PSP_CTRL_CIRCLE) {
            network_disconnect();
            sceKernelExitGame();
            break;
        }

        if (!g_freeze)
            g_frame++;

        if (g_source == SRC_ESP32) {
            network_poll();
        } else {
            demo_update(&g_data);
        }

        if (g_page_dirty)
            render_static();

        render_div++;
        if (render_div >= 4) {
            render_div = 0;
            render_dynamic(&g_data);

            if (g_freeze) {
                linef(47, 4, C_RED, "FREEZE");
            }
        }

        oldpad = pad;
        sceDisplayWaitVblankStart();
    }

    return 0;
}
