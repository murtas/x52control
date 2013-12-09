#include "x52session.h"
#include "x52interface.h"

#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define X52PLUGINNAME_STR   "x52control [v" VERSION "]"
#define X52SIGNATURE_STR    "copai.equipment.x52control"
#define X52DESCRIPTION_STR  "Saitek X52 and X52Pro interface plugin, " \
                "compiled on " __DATE__ ". Send feedback " \
                "to murteira.diogo@gmail.com."

enum init_constants_e {
    update_interval_ms = 200,
};

extern void init_ctor(void(*fn)(void));

typedef float (*floopfn)(float, float, int, void*);
extern void XPLMRegisterFlightLoopCallback(floopfn, float, void*);
extern void XPLMUnregisterFlightLoopCallback(floopfn, void*);

extern void* control_init(void);
extern void control_update(void* arg);
extern void control_end(void* arg);

static void* current_arg = 0;

XPLMDataRef ACF_LandingLights   = NULL;
XPLMDataRef ACF_TaxiLights      = NULL;
XPLMDataRef ACF_GearType        = NULL;
XPLMDataRef ACF_GearDeploy      = NULL;
XPLMDataRef ACF_GearRetract     = NULL;
XPLMDataRef ACF_Reverse         = NULL;
XPLMDataRef ACF_Flaps           = NULL;
XPLMDataRef ACF_NoseWheelSteer  = NULL;
XPLMDataRef ACF_HSISource       = NULL;
XPLMDataRef ACF_Com1Freq        = NULL;
XPLMDataRef ACF_Com2Freq        = NULL;
XPLMDataRef ACF_Nav1Freq        = NULL;
XPLMDataRef ACF_Nav2Freq        = NULL;

// Date Time Data
struct tm tm; 
time_t t;

// XPlane Data
int   acf_gear_count;
int   acf_gear_type[10];
float acf_gear_deploy[10];
int   acf_gear_retract;
float acf_flaps;
int   acf_nosewheel_steer_on;
int   acf_engine_reverse;
int   acf_landing_lights_on, acf_taxi_lights_on;
int   acf_hsi_source;
int   acf_com1_freq, acf_com2_freq;
int   acf_nav1_freq, acf_nav2_freq;

char  mfd[3][16];
int   leds = 0;
int   i = 0;


static float
try_update(float elapsed_lastcall, float elapsed_lastloop, int loop, void* arg)
{
    control_update(arg);

    // Date and Time
    t  = time(NULL);
    tm = *localtime(&t);

    // Aircraft Data
    XPLMGetDatavi(ACF_GearType,   acf_gear_type,   0, 10);
    XPLMGetDatavf(ACF_GearDeploy, acf_gear_deploy, 0, 10);
    acf_gear_retract        = XPLMGetDatai(ACF_GearRetract);
    acf_flaps               = XPLMGetDataf(ACF_Flaps);
    acf_nosewheel_steer_on  = XPLMGetDatai(ACF_NoseWheelSteer);
    acf_engine_reverse      = XPLMGetDatai(ACF_Reverse);
    acf_landing_lights_on   = XPLMGetDatai(ACF_LandingLights);
    acf_taxi_lights_on      = XPLMGetDatai(ACF_TaxiLights);

    // Radios
    acf_hsi_source = XPLMGetDatai(ACF_HSISource);
    acf_com1_freq  = XPLMGetDatai(ACF_Com1Freq);
    acf_com2_freq  = XPLMGetDatai(ACF_Com2Freq);
    acf_nav1_freq  = XPLMGetDatai(ACF_Nav1Freq);
    acf_nav2_freq  = XPLMGetDatai(ACF_Nav2Freq);

    // Number of landing gears in current aircraft
    acf_gear_count = 0;
    while (acf_gear_type[acf_gear_count] && acf_gear_count < 9)
        acf_gear_count++;

    /********************************
     *              LEDS            *
     ********************************/

    leds = 0;

    x52i_clr_led(x52i_led_pov2_amber | x52i_led_T3T4_amber | 
                 x52i_led_T5T6_amber | x52i_led_D_amber | 
                 x52i_led_E_amber    | x52i_led_clutch_amber);
    
    if (acf_gear_retract)
    {
        for (i = 0; i < acf_gear_count && acf_gear_deploy[i] > 0.97f; i++);

        if (i == acf_gear_count)
            leds |= x52i_led_E_green | x52i_led_clutch_green;
        else
            leds |= x52i_led_E_amber | x52i_led_clutch_amber;
    }

    switch (acf_engine_reverse)
    {
        case 0:
            leds |= x52i_led_D_green;
            break;
        case 1: 
            leds |= x52i_led_D_red;
            break;
    }
    

    switch (acf_nosewheel_steer_on)
    {
        case 0:
            leds |= x52i_led_pov2_green;
            break;
        case 1:
            leds |= x52i_led_pov2_amber;
            break;
    }
    
    switch (acf_taxi_lights_on)
    {
        case 1:
            leds |= x52i_led_T3T4_green;
            break;
    }

    switch (acf_landing_lights_on)
    {
        case 1:
            leds |= x52i_led_T5T6_green;
            break;  
    }

    x52i_set_led(leds);

    /********************************
     *    Multi Function Display    *
     ********************************/

    snprintf(mfd[0], 16, "  COM1: %3.2f", (float)acf_com1_freq/100.f);
    snprintf(mfd[1], 16, " FLAPS: %3.0f%%", acf_flaps*100.0);
    switch (acf_hsi_source)
    {
        case 0:
            snprintf(mfd[2], 16, "HSI NAV1: %3.2f", (float)acf_nav1_freq/100.f);
            break;
        case 1:
            snprintf(mfd[2], 16, "HSI NAV2: %3.2f", (float)acf_nav2_freq/100.f);
            break;
        case 2:
            snprintf(mfd[2], 16, "HSI GPS");
            break;
    }

    x52i_set_text(x52i_text_line1, mfd[0]);            
    x52i_set_text(x52i_text_line2, mfd[1]);            
    x52i_set_text(x52i_text_line3, mfd[2]);            

    x52i_set_date((uint8_t)tm.tm_mday, (uint8_t)(tm.tm_mon + 1), (uint8_t)(tm.tm_year - 100));
    x52i_set_time((uint8_t)tm.tm_hour, (uint8_t)tm.tm_min, x52i_mode_24h);

    x52i_commit();
    
    return (update_interval_ms/1000.f);
}

static void init_connection(void)
{
    XPLMRegisterFlightLoopCallback(try_update,
        (update_interval_ms/1000.f), current_arg);
}

static void close_connection(void)
{
    control_end(current_arg);
    x52s_disable(0);
}

__attribute__ ((visibility("default"))) int
XPluginStart(char* name, char* sig, char* descr)
{
    init_ctor(close_connection);
    strcpy(name,    X52PLUGINNAME_STR);
    strcpy(sig,     X52SIGNATURE_STR);
    strcpy(descr,   X52DESCRIPTION_STR);
    current_arg = control_init();

    ACF_GearType       = XPLMFindDataRef("sim/aircraft/parts/acf_gear_type");
    ACF_GearDeploy     = XPLMFindDataRef("sim/aircraft/parts/acf_gear_deploy");
    ACF_GearRetract    = XPLMFindDataRef("sim/aircraft/gear/acf_gear_retract");
    ACF_Reverse        = XPLMFindDataRef("sim/cockpit/warnings/annunciators/reverse");
    ACF_Flaps          = XPLMFindDataRef("sim/flightmodel2/controls/flap_handle_deploy_ratio");
    ACF_NoseWheelSteer = XPLMFindDataRef("sim/cockpit2/controls/nosewheel_steer_on");
    ACF_TaxiLights     = XPLMFindDataRef("sim/cockpit/electrical/taxi_light_on");
    ACF_LandingLights  = XPLMFindDataRef("sim/cockpit/electrical/landing_lights_on");
    ACF_HSISource      = XPLMFindDataRef("sim/cockpit2/radios/actuators/HSI_source_select_pilot");
    ACF_Com1Freq       = XPLMFindDataRef("sim/cockpit/radios/com1_freq_hz");
    ACF_Com2Freq       = XPLMFindDataRef("sim/cockpit/radios/com2_freq_hz");
    ACF_Nav1Freq       = XPLMFindDataRef("sim/cockpit/radios/nav1_freq_hz");
    ACF_Nav2Freq       = XPLMFindDataRef("sim/cockpit/radios/nav2_freq_hz");

    return 1;
}

__attribute__ ((visibility("default"))) int
XPluginEnable(void)
{
    return x52s_enable(init_connection);
}

/* we clean up resources using atexit() via init_ctor() */
__attribute__ ((visibility("default"))) void
XPluginDisable(void)
{
    XPLMUnregisterFlightLoopCallback(try_update, current_arg);
}

__attribute__ ((visibility("default"))) void
XPluginStop(void)
{}

__attribute__ ((visibility("default"))) void
XPluginReceiveMessage(uint32_t from, uint32_t msg, void* arg)
{}

