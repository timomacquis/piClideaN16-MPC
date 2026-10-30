/* E01: bounded per-instance modulation. No allocation or device work here. */
#define MOD_SLOTS 4
#define MOD_FIELDS 18
#define MOD_PER_LANE (1+2*MOD_FIELDS)
#define MOD_PARAMS ((LANES+1)*MOD_PER_LANE)
#define MOD_DEST_NOTE 7
#define MOD_VIS_COUNT (LANES+5+64)
#define MOD_WAVE_COUNT 11
#define MOD_DEST_COUNT 11
#define MOD_CHAIN_RATE 8
#define MOD_CHAIN_DEPTH 9
#define MOD_CHAIN_OFFSET 10
#define MOD_CHAIN_POINTS 2050
typedef struct { int f[MOD_FIELDS]; int extra[4]; } ModOsc;
typedef struct { double previous,next; int64_t cycle; unsigned seed; int ready; } ModWalk;
typedef struct { ModOsc osc[2]; int mix; } ModLane;
