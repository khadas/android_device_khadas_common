#include <stdio.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <cutils/log.h>

#define BASE_OFFSET 8*1024
#define BACKUP_OFFSET 512*1024
#define TEST_BASE_PARMARTER
#define DEFAULT_BRIGHTNESS  50
#define DEFAULT_CONTRAST  50
#define DEFAULT_SATURATION  50
#define DEFAULT_HUE  50

#define BUFFER_LENGTH    256
#define RESOLUTION_AUTO 1<<0
#define COLOR_AUTO (1<<1)
#define HDCP1X_EN (1<<2)
#define RESOLUTION_WHITE_EN (1<<3)

enum {
    HWC_DISPLAY_PRIMARY     = 0,
    HWC_DISPLAY_EXTERNAL    = 1,    // HDMI, DP, etc.
    HWC_DISPLAY_VIRTUAL     = 2,

    HWC_NUM_PHYSICAL_DISPLAY_TYPES = 2,
    HWC_NUM_DISPLAY_TYPES          = 3,
};
enum {
    HWC_DISPLAY_PRIMARY_BIT     = 1 << HWC_DISPLAY_PRIMARY,
    HWC_DISPLAY_EXTERNAL_BIT    = 1 << HWC_DISPLAY_EXTERNAL,
    HWC_DISPLAY_VIRTUAL_BIT     = 1 << HWC_DISPLAY_VIRTUAL,
};

struct lut_data{
    uint16_t size;
    uint16_t lred[1024];
    uint16_t lgreen[1024];
    uint16_t lblue[1024];
};

struct drm_display_mode {
    /* Proposed mode values */
    int clock;		/* in kHz */
    int hdisplay;
    int hsync_start;
    int hsync_end;
    int htotal;
    int vdisplay;
    int vsync_start;
    int vsync_end;
    int vtotal;
    int vrefresh;
    int vscan;
    unsigned int flags;
    int picture_aspect_ratio;
};

enum output_format {
    output_rgb=0,
    output_ycbcr444=1,
    output_ycbcr422=2,
    output_ycbcr420=3,
    output_ycbcr_high_subsampling=4,  // (YCbCr444 > YCbCr422 > YCbCr420 > RGB)
    output_ycbcr_low_subsampling=5	, // (RGB > YCbCr420 > YCbCr422 > YCbCr444)
    invalid_output=6,
};

enum output_depth {
    Automatic=0,
    depth_24bit=8,
    depth_30bit=10,
};

struct overscan {
    unsigned int maxvalue;
    unsigned short leftscale;
    unsigned short rightscale;
    unsigned short topscale;
    unsigned short bottomscale;
};

struct hwc_inital_info{
    char device[128];
    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    float fps;
};
struct bcsh_info {
    unsigned short brightness;
    unsigned short contrast;
    unsigned short saturation;
    unsigned short hue;
};

struct disp_info{
    struct drm_display_mode resolution;// 52 bytes
    struct overscan scan;//10 bytes
    enum output_format  format; // 4 bytes
    enum output_depth depthc; // 4 bytes
    unsigned int feature;//4 //4 bytes
    struct hwc_inital_info hwc_info; //140 bytes
    struct bcsh_info bcsh;
    unsigned int reserve[128];/*0~3 use for bcsh*/ //459x4
    struct lut_data mlutdata;/*6k+4*/
};


struct file_base_paramer
{
    struct disp_info main;
    struct disp_info aux;
};

static char const *const device_template[] =
{
	"/dev/block/platform/1021c000.rksdmmc/by-name/baseparameter",
	"/dev/block/platform/30020000.rksdmmc/by-name/baseparameter",
	"/dev/block/platform/ff0f0000.rksdmmc/by-name/baseparameter",
	"/dev/block/platform/ff520000.rksdmmc/by-name/baseparameter",
	"/dev/block/platform/fe330000.sdhci/by-name/baseparameter",
	"/dev/block/platform/ff520000.dwmmc/by-name/baseparameter",
	"/dev/block/platform/ff0f0000.dwmmc/by-name/baseparameter",
	"/dev/block/rknand_baseparameter",
	NULL
};

const char* GetBaseparameterFile(void)
{
    int i = 0;

    while (device_template[i]) {
        if (!access(device_template[i], R_OK | W_OK))
            return device_template[i];
        ALOGD("temp[%d]=%s access=%d(%s)", i,device_template[i], errno, strerror(errno));
        i++;
    }
    return NULL;
}

static void saveResolutionInfo(struct file_base_paramer *base_paramer, int dpy){
    unsigned int left,top,right,bottom;
    left = top = right = bottom = 95;
    if (dpy == HWC_DISPLAY_PRIMARY) {
        base_paramer->main.resolution.clock = 148500;
        base_paramer->main.resolution.hdisplay = 1920;
        base_paramer->main.resolution.hsync_start = 2008;
        base_paramer->main.resolution.hsync_end = 2052;
        base_paramer->main.resolution.htotal = 2200;
        base_paramer->main.resolution.vdisplay = 1080;
        base_paramer->main.resolution.vsync_start = 1084;
        base_paramer->main.resolution.vsync_end = 1089;
        base_paramer->main.resolution.vtotal = 1125;
        base_paramer->main.resolution.vrefresh = 60;
        base_paramer->main.resolution.vscan = 0;
        base_paramer->main.resolution.flags = 0x5;

        base_paramer->main.scan.maxvalue = 100;
        base_paramer->main.scan.leftscale = (unsigned short)left;
        base_paramer->main.scan.topscale = (unsigned short)top;
        base_paramer->main.scan.rightscale = (unsigned short)right;
        base_paramer->main.scan.bottomscale = (unsigned short)bottom;
    } else {
        base_paramer->aux.resolution.clock = 148500;
        base_paramer->aux.resolution.hdisplay = 1920;
        base_paramer->aux.resolution.hsync_start = 2008;
        base_paramer->aux.resolution.hsync_end = 2052;
        base_paramer->aux.resolution.htotal = 2200;
        base_paramer->aux.resolution.vdisplay = 1080;
        base_paramer->aux.resolution.vsync_start = 1084;
        base_paramer->aux.resolution.vsync_end = 1089;
        base_paramer->aux.resolution.vtotal = 1125;
        base_paramer->aux.resolution.vrefresh = 60;
        base_paramer->aux.resolution.vscan = 0;
        base_paramer->aux.resolution.flags = 0x5;

        base_paramer->aux.scan.maxvalue = 100;
        base_paramer->aux.scan.leftscale = (unsigned short)left;
        base_paramer->aux.scan.topscale = (unsigned short)top;
        base_paramer->aux.scan.rightscale = (unsigned short)right;
        base_paramer->aux.scan.bottomscale = (unsigned short)bottom;
    }
}

static void saveColor(struct file_base_paramer *base_paramer) {
    base_paramer->main.format = output_rgb;
    /*set feature to COLOR_AUTO, uboot will choose it automatic*/
    base_paramer->main.feature |= COLOR_AUTO;
    base_paramer->main.depthc = Automatic;

    /*AUX INFO*/
    base_paramer->aux.format = output_rgb;
    base_paramer->aux.feature |= COLOR_AUTO;
    base_paramer->aux.depthc = Automatic;
}

static void saveHwcInitalInfo(struct file_base_paramer *base_paramer, int dpy, char* fb_info, char* device){
    int fb_w=0, fb_h=0;
    int fps=0;
    printf("fb_info=%s devices=%s 2\n",fb_info, device);
    if (fb_info != NULL)
        sscanf(fb_info, "%dx%d@%d\0", &fb_w, &fb_h, &fps);
    else {
        fb_w = 1920;
        fb_h = 1080;
        printf("error: cant get fb_info\n");
    }
    printf("%d %d %d\n", fb_w, fb_h, fps);
    if (dpy == HWC_DISPLAY_PRIMARY){
        int len;
        char property[PROPERTY_VALUE_MAX];
        base_paramer->main.hwc_info.framebuffer_width = fb_w;
        base_paramer->main.hwc_info.framebuffer_height = fb_h;
        base_paramer->main.hwc_info.fps = fps;
        memset(property,0,sizeof(property));
        len = property_get("sys.hwc.device.primary", property, NULL);
        if (len && device==NULL) {
            memcpy(base_paramer->main.hwc_info.device, property, strlen(property));
        } else if (device != NULL){
            sprintf(base_paramer->main.hwc_info.device, "%s", device);
        } else {
            base_paramer->main.hwc_info.device[0]='\0';
        }
    } else {
        int len;
        char property[PROPERTY_VALUE_MAX];
        base_paramer->aux.hwc_info.framebuffer_width = fb_w;
        base_paramer->aux.hwc_info.framebuffer_height = fb_h;
        base_paramer->aux.hwc_info.fps = fps;
        memset(property,0,sizeof(property));
        len = property_get("sys.hwc.device.extend", property, NULL);
        if (len && device==NULL)
            memcpy(base_paramer->aux.hwc_info.device, property, strlen(property));
        else if (device != NULL)
            sprintf(base_paramer->aux.hwc_info.device, "%s", device);
        else
            base_paramer->aux.hwc_info.device[0]='\0';
    }
}

static void saveBcshConfig(struct file_base_paramer *base_paramer, int dpy){
    if (dpy == HWC_DISPLAY_PRIMARY){
        char property[PROPERTY_VALUE_MAX];

        memset(property,0,sizeof(property));
        property_get("persist.sys.brightness.main", property, "0");
        if (atoi(property) > 0)
            base_paramer->main.reserve[0] = atoi(property);
        else
            base_paramer->main.reserve[0] = DEFAULT_BRIGHTNESS;

        memset(property,0,sizeof(property));
        property_get("persist.sys.contrast.main", property, "0");
        if (atoi(property) > 0)
            base_paramer->main.reserve[1] = atoi(property);
        else
            base_paramer->main.reserve[1] = DEFAULT_CONTRAST;

        memset(property,0,sizeof(property));
        property_get("persist.sys.saturation.main", property, "0");
        if (atoi(property) > 0)
            base_paramer->main.reserve[2] = atoi(property);
        else
            base_paramer->main.reserve[2] = DEFAULT_SATURATION;

        memset(property,0,sizeof(property));
        property_get("persist.sys.hue.main", property, "0");
        if (atoi(property) > 0)
            base_paramer->main.reserve[3] = atoi(property);
        else
            base_paramer->main.reserve[3] = DEFAULT_HUE;
    } else {
        char property[PROPERTY_VALUE_MAX];

        memset(property,0,sizeof(property));
        property_get("persist.sys.brightness.aux", property, "0");
        if (atoi(property) > 0)
            base_paramer->aux.reserve[0] = atoi(property);
        else
            base_paramer->aux.reserve[0] = DEFAULT_BRIGHTNESS;

        memset(property,0,sizeof(property));
        property_get("persist.sys.contrast.aux", property, "0");
        if (atoi(property) > 0)
            base_paramer->aux.reserve[1] = atoi(property);
        else
            base_paramer->aux.reserve[1] = DEFAULT_CONTRAST;

        memset(property,0,sizeof(property));
        property_get("persist.sys.saturation.aux", property, "0");
        if (atoi(property) > 0)
            base_paramer->aux.reserve[2] = atoi(property);
        else
            base_paramer->aux.reserve[2] = DEFAULT_SATURATION;

        memset(property,0,sizeof(property));
        property_get("persist.sys.hue.aux", property, "0");
        if (atoi(property) > 0)
            base_paramer->aux.reserve[3] = atoi(property);
        else
            base_paramer->aux.reserve[3] = DEFAULT_HUE;
    }
}

static void printParameter(struct file_base_paramer *base_paramer){
    printf("-main: \n");
    printf("\tresolution: %dx%d@p-%d-%d-%d-%d-%d-%d-%x\n", base_paramer->main.resolution.hdisplay,
            base_paramer->main.resolution.vdisplay, base_paramer->main.resolution.hsync_start,
            base_paramer->main.resolution.hsync_end, base_paramer->main.resolution.htotal,
            base_paramer->main.resolution.vsync_start, base_paramer->main.resolution.vsync_end,
            base_paramer->main.resolution.vtotal, base_paramer->main.resolution.flags);
    printf("\tcorlor: format %d depth %d \n", base_paramer->main.format, base_paramer->main.depthc);
    printf("\tfbinfo: %dx%d@%f device:%s\n", base_paramer->main.hwc_info.framebuffer_width,
            base_paramer->main.hwc_info.framebuffer_height, base_paramer->main.hwc_info.fps, base_paramer->main.hwc_info.device);
    printf("\tbcsh: %d %d %d %d \n", base_paramer->main.bcsh.brightness, base_paramer->main.bcsh.contrast,
            base_paramer->main.bcsh.saturation, base_paramer->main.bcsh.hue);
    printf("\toverscan: %d %d %d %d \n", base_paramer->main.scan.leftscale, base_paramer->main.scan.topscale,
            base_paramer->main.scan.rightscale, base_paramer->main.scan.bottomscale);
    printf("\tfeature:  0x%x \n", base_paramer->main.feature);

    printf("-aux: \n");
    printf("\tresolution: %dx%d@p-%d-%d-%d-%d-%d-%d-%x\n", base_paramer->aux.resolution.hdisplay, 
            base_paramer->aux.resolution.vdisplay,
            base_paramer->aux.resolution.hsync_start,
            base_paramer->aux.resolution.hsync_end, base_paramer->aux.resolution.htotal,
            base_paramer->aux.resolution.vsync_start, base_paramer->main.resolution.vsync_end, 
            base_paramer->aux.resolution.vtotal,
            base_paramer->aux.resolution.flags);
    printf("\tcorlor: format %d depth %d \n", base_paramer->aux.format, base_paramer->aux.depthc);
    printf("\tfbinfo: %dx%d@%f device:%s\n", base_paramer->aux.hwc_info.framebuffer_width, 
            base_paramer->aux.hwc_info.framebuffer_height, base_paramer->aux.hwc_info.fps, 
            base_paramer->aux.hwc_info.device);
    printf("\tbcsh: %d %d %d %d \n", base_paramer->aux.bcsh.brightness, base_paramer->aux.bcsh.contrast, 
            base_paramer->aux.bcsh.saturation, base_paramer->aux.bcsh.hue);
    printf("\toverscan: %d %d %d %d \n", base_paramer->aux.scan.leftscale, base_paramer->aux.scan.topscale,
            base_paramer->aux.scan.rightscale, base_paramer->aux.scan.bottomscale);
    printf("\tfeature:  0x%x \n", base_paramer->aux.feature);
}

static void usage(){
    fprintf(stderr, "\nsaveParameter: read and write baseparameter partition tool\n");
    fprintf(stderr, "\nUsage:\n");
    fprintf(stderr, "\t-h\t Help info\n");
    fprintf(stderr, "\t-d\t Choose Display to Setting (e: 0 or 1)\n");	
    fprintf(stderr, "\t-f\t Framebuffer Resolution (e: 1920x1080@60)\n");
    fprintf(stderr, "\t-D\t Display Attach Devices (e: HDMI-A,TV)\n");
    fprintf(stderr, "\t-c\t Color (e: RGB-8bit or YCBCR444-10bit)\n");
    fprintf(stderr, "\t-u\t Is Enable Auto Resolution (2:auto resolution; 1:set one fixed resolution)\n");
    fprintf(stderr, "\t-o\t Overscan (e: overscan \"100,100,100,100\")\n");
    fprintf(stderr, "\t-b\t BCSH (e: \"50,50,50,50\") \n");
    fprintf(stderr, "\t-p\t Print Baseparamter\n");
    fprintf(stderr, "\t-R\t Reset Baseparameter (1:only reset user setting baseparameter partition; 2:reset baseparameter paratition include backup)\n");
    fprintf(stderr, "\nExample: saveBaseParameter -d 0 -f 1920x1080@60 -D \"HDMI-A,TV\" -c Auto -u 2 -o \"100,100,100,100\" -b \"50,50,50,50\"\n");
    fprintf(stderr, "\n===== Rockchip All Rights Reserved =====\n\n");
}

int main(int argc, char** argv){
    struct file_base_paramer base_paramer;
    struct file_base_paramer backup_paramer;
    int file;
    char* device=NULL;
    char* fb_info=NULL;
    bool isEnablesaveReso=false;
    int resetBaseParameter=0;
    int display=0;
    char* corlor_info=NULL;
    bool autoCorlor=false;
    int autoResolution=0;
    int value;
    int isPrintBaseInfo=0;
    char* overscan=NULL;
    char* bcsh=NULL;

    int res;
    // printf("----- parsing auguments\n");
    while ((res = getopt(argc, argv, "b:f:d:D:u:c:R:o:hap")) >= 0) {
        //	printf("res = %d\n", res);
        const char *colonPos;
        switch (res) {
            case 'f':
                fb_info = optarg;
                if (strstr(fb_info, "x") == NULL || strstr(fb_info, "@")==NULL){
                    usage();
                    return -1;
                }
                printf("framebuffer %s (-f)\n", fb_info);
                break;
            case 'd':
                display = atoi(optarg);
                if (display > 1) {
                    usage();
                    return 0;
                }
                printf("display %d (-d)\n", display);
                break;
            case 'D':
                device  = optarg;
                printf("device %s (-D)\n", device);
                break;
            case 'u':
                value  = atoi(optarg);
                printf("isEnableSaveResolution %s (-u)\n", optarg);
                if (value == 2)
                    autoResolution = 1;
                else if (value == 1) 
                    isEnablesaveReso =true;
                break;
            case 'c':
                corlor_info = optarg;
                printf("color %s (-c)\n", optarg);
                break;
            case 'R':
                resetBaseParameter = atoi(optarg);
                printf("reset baseparameter %d\n", resetBaseParameter);
                break;
            case 'a':
                autoCorlor=true;
                break;
            case 'p':
                isPrintBaseInfo = 1;
                printf("print baseparameter\n");
                break;
            case 'o':
                overscan = optarg;
                printf("overscan %s (-o)\n", overscan);
                break;
            case 'b':
                bcsh = optarg;
                printf("bcsh %s (-b)\n", bcsh);
                break;
            case 'h':
                usage();
                return 0;
            default:
                break;
        }
    }

    const char *baseparameterfile = GetBaseparameterFile();
    if (!baseparameterfile) {
        sync();
        return -1;
    }

    file = open(baseparameterfile, O_RDWR);
    if (file < 0) {
        printf("base paramter file can not be opened \n");
        sync();
        return -1;
    }
    // caculate file's size and read it
    unsigned int length = lseek(file, 0L, SEEK_END);
    if(length < sizeof(base_paramer)) {
        printf("BASEPARAME data's length is error\n");
        close(file);
        return -1;
    }
    lseek(file, 0L, SEEK_SET);
    read(file, (void*)&(base_paramer.main), sizeof(base_paramer.main));/*read main display info*/
    lseek(file, BASE_OFFSET, SEEK_SET);
    read(file, (void*)&(base_paramer.aux), sizeof(base_paramer.aux));/*read aux display info*/
    lseek(file, BACKUP_OFFSET, SEEK_SET);
    read(file, (void*)&(backup_paramer.main), sizeof(backup_paramer.main));/*read main display info*/
    lseek(file, BACKUP_OFFSET + BASE_OFFSET, SEEK_SET);
    read(file, (void*)&(backup_paramer.aux), sizeof(backup_paramer.aux));/*read aux display info*/

    if (resetBaseParameter > 0) {
        memset(&base_paramer.main, 0, sizeof(base_paramer.main));
        memset(&base_paramer.aux, 0, sizeof(base_paramer.aux));
        // reset baseparameter
        lseek(file, 0L, SEEK_SET);
        write(file, (char*)(&base_paramer.main), sizeof(base_paramer.main));
        lseek(file, BASE_OFFSET, SEEK_SET);
        write(file, (char*)(&base_paramer.aux), sizeof(base_paramer.aux));
        // reset back baseparameter
        if (resetBaseParameter == 2) {
            printf("reset backup parameter\n");
            lseek(file, BACKUP_OFFSET, SEEK_SET);
            write(file, (char*)(&base_paramer.main), sizeof(base_paramer.main));
            lseek(file, BACKUP_OFFSET+BASE_OFFSET, SEEK_SET);
            write(file, (char*)(&base_paramer.aux), sizeof(base_paramer.aux));
        }
        sync();
        close(file);
        printf("reset done.\n");
        return 0;
    }

    if (isPrintBaseInfo > 0) {
        printf("========== base parameter ==========\n");
        printParameter(&base_paramer);
        printf("\n========= backup parameter ==========\n");
        printParameter(&backup_paramer);
        printf("====================================\n");
        return 0;
    }

    printf("----- setting baseparameter\n");
    printf("base_paramer.main.resolution hdisplay=%d vdisplay=%d (%s@%f) 0x%x\n", 
            base_paramer.main.resolution.hdisplay,
            base_paramer.main.resolution.vdisplay,
            base_paramer.main.hwc_info.device,  base_paramer.main.hwc_info.fps,
            base_paramer.main.feature);
    if (isEnablesaveReso) {
        saveResolutionInfo(&base_paramer, display);
    }
    if (autoResolution > 0) {
        if (display == HWC_DISPLAY_PRIMARY) {
            memset(&base_paramer.main.resolution, 0, sizeof(base_paramer.main.resolution));
            base_paramer.main.feature |= RESOLUTION_AUTO;
        }
        if (display == HWC_DISPLAY_EXTERNAL) {
            memset(&base_paramer.aux.resolution, 0, sizeof(base_paramer.aux.resolution));
            base_paramer.aux.feature |= RESOLUTION_AUTO;
        }
    }

    //saveColor(&base_paramer);
    /*enable HDCP1X*/
    if (display == HWC_DISPLAY_PRIMARY && (corlor_info != NULL && strcmp(corlor_info, "Auto"))) {
        //base_paramer.main.feature |= HDCP1X_EN;
        char color[16];
        char depth[16];

        memset(color,0,sizeof(color));
        memset(depth,0,sizeof(depth));
        sscanf(corlor_info, "%s-%s", color, depth);
        base_paramer.main.feature |= RESOLUTION_WHITE_EN;
        if (strncmp(color, "RGB", 3) == 0)
            base_paramer.main.format = output_rgb;
        else if (strncmp(color, "YCBCR444", 8) == 0)
            base_paramer.main.format = output_ycbcr444;
        else if (strncmp(color, "YCBCR422", 8) == 0)
            base_paramer.main.format = output_ycbcr422;
        else if (strncmp(color, "YCBCR420", 8) == 0)
            base_paramer.main.format = output_ycbcr420;
        else {
            base_paramer.main.format = output_ycbcr_high_subsampling;
            base_paramer.main.feature |= COLOR_AUTO;
        }

        printf("info=%s corlor-depth %s-%s\n",corlor_info, color, depth);
        printf("depth=%s\n", depth);
        if (strstr(corlor_info, "8bit") != NULL)
            base_paramer.main.depthc = depth_24bit;
        else if (strstr(corlor_info, "10bit") != NULL)
            base_paramer.main.depthc = depth_30bit;
        else
            base_paramer.main.depthc = Automatic;
    } else if (display == HWC_DISPLAY_PRIMARY){
        base_paramer.main.format = output_ycbcr_high_subsampling;
        base_paramer.main.depthc = Automatic;
        base_paramer.main.feature |= COLOR_AUTO;
    }

    if (display == HWC_DISPLAY_EXTERNAL && (corlor_info!=NULL&& strcmp(corlor_info, "Auto"))) {
        //base_paramer.aux.feature |= HDCP1X_EN;
        base_paramer.aux.feature |= RESOLUTION_WHITE_EN;
        char color[16];
        char depth[16];

        sscanf(corlor_info, "%s-%s", color, depth);
        if (strncmp(color, "RGB", 3) == 0)
            base_paramer.aux.format = output_rgb;
        else if (strncmp(color, "YCBCR444", 8) == 0)
            base_paramer.aux.format = output_ycbcr444;
        else if (strncmp(color, "YCBCR422", 8) == 0)
            base_paramer.aux.format = output_ycbcr422;
        else if (strncmp(color, "YCBCR420", 8) == 0)
            base_paramer.aux.format = output_ycbcr420;
        else {
            base_paramer.aux.format = output_ycbcr_high_subsampling;
            base_paramer.aux.feature |= COLOR_AUTO;
        }

        if (strstr(depth, "8bit") == 0)
            base_paramer.aux.depthc = depth_24bit;
        else if (strstr(depth, "10bit") == 0)
            base_paramer.aux.depthc = depth_30bit;
        else
            base_paramer.aux.depthc = Automatic;
    } else if (display == HWC_DISPLAY_EXTERNAL){
        base_paramer.aux.format = output_ycbcr_high_subsampling;
        base_paramer.aux.depthc = Automatic;
    }

    if (device == NULL || fb_info== NULL) {
        printf("no device & fb_info use default\n");
    } else {
        saveHwcInitalInfo(&base_paramer, display, fb_info, device);
        saveBcshConfig(&base_paramer, display);
    }

    if (overscan != NULL) {
        int left,top,right,bottom;
        sscanf(overscan, "%d,%d,%d,%d", &left, &top, &right, &bottom);
        if (display ==HWC_DISPLAY_PRIMARY) {
            base_paramer.main.scan.leftscale = left;
            base_paramer.main.scan.rightscale = right;
            base_paramer.main.scan.topscale = top;
            base_paramer.main.scan.bottomscale = bottom;
        } else if(display == HWC_DISPLAY_EXTERNAL){
            base_paramer.aux.scan.leftscale = left;
            base_paramer.aux.scan.rightscale = right;
            base_paramer.aux.scan.topscale = top;
            base_paramer.aux.scan.bottomscale = bottom;
        }
    }

    if (bcsh != NULL) {
        int b,c,s,h;
        sscanf(bcsh, "%d,%d,%d,%d", &b, &c, &s, &h);
        if (display ==HWC_DISPLAY_PRIMARY) {
            base_paramer.main.bcsh.brightness = b;
            base_paramer.main.bcsh.contrast = c;
            base_paramer.main.bcsh.saturation = s;
            base_paramer.main.bcsh.hue = h;
        } else if(display == HWC_DISPLAY_EXTERNAL){
            base_paramer.aux.bcsh.brightness = b;
            base_paramer.aux.bcsh.contrast = c;
            base_paramer.aux.bcsh.saturation = s;
            base_paramer.aux.bcsh.hue = h;
        }
    }

    printf("----- checking baseparameter\n");
    printParameter(&base_paramer);

    lseek(file, 0L, SEEK_SET);
    if (display == HWC_DISPLAY_PRIMARY)
        write(file, (char*)(&base_paramer.main), sizeof(base_paramer.main));
    lseek(file, BASE_OFFSET, SEEK_SET);
    if (display == HWC_DISPLAY_EXTERNAL)
        write(file, (char*)(&base_paramer.aux), sizeof(base_paramer.aux));

    lseek(file, BACKUP_OFFSET, SEEK_SET);
    if (display == HWC_DISPLAY_PRIMARY)
        write(file, (char*)(&base_paramer.main), sizeof(base_paramer.main));
    lseek(file, BACKUP_OFFSET+BASE_OFFSET, SEEK_SET);
    if (display == HWC_DISPLAY_EXTERNAL)
        write(file, (char*)(&base_paramer.aux), sizeof(base_paramer.aux));
    close(file);
    sync();
    printf("----- writing baseparameter\n");
    printf("done.\n");

    return 0;
}