#define PLUG_NAME "Pitch Trails"
#define PLUG_MFR "Wave Factory Essentials"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "0.1.0"
#define PLUG_UNIQUE_ID 'WfPt'
#define PLUG_MFR_ID 'WvFy'
#define PLUG_URL_STR "https://github.com/carlwelchdesign"
#define PLUG_EMAIL_STR ""
#define PLUG_COPYRIGHT_STR "Copyright 2026 Wave Factory Essentials"
#define PLUG_CLASS_NAME PitchTrails

#define BUNDLE_NAME "PitchTrails"
#define BUNDLE_MFR "wavefactoryessentials"
#define BUNDLE_DOMAIN "com"
#define SHARED_RESOURCES_SUBPATH "PitchTrails"

#define PLUG_CHANNEL_IO "1-1 2-2"
#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 760
#define PLUG_HEIGHT 460
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY PitchTrails_Entry
#define AUV2_ENTRY_STR "PitchTrails_Entry"
#define AUV2_FACTORY PitchTrails_Factory
#define AUV2_VIEW_CLASS PitchTrails_View
#define AUV2_VIEW_CLASS_STR "PitchTrails_View"

#define AAX_TYPE_IDS 'WP01', 'WP02'
#define AAX_TYPE_IDS_AUDIOSUITE 'WPA1', 'WPA2'
#define AAX_PLUG_MFR_STR "Wave Factory Essentials"
#define AAX_PLUG_NAME_STR "Pitch Trails\nPitchTrails"
#define AAX_PLUG_CATEGORY_STR "Delay"
#define AAX_DOES_AUDIOSUITE 0

#define VST3_SUBCATEGORY "Fx|Delay|Reverb"
#define CLAP_MANUAL_URL ""
#define CLAP_SUPPORT_URL ""
#define CLAP_DESCRIPTION "Pitch-shifting delay and diffusion without drawn automation"
#define CLAP_FEATURES "audio-effect", "delay", "reverb"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64
