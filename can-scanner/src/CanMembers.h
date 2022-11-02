

typedef struct 
{
    const char *name;
    unsigned short id;
} canMember;

// Changing default values of enum constants
enum msgType {
    t_write = 0,
    t_read = 1,
    t_response = 2,
    t_ack = 3,
    t_writeAck = 4,
    t_writeRespond = 5,
    t_system = 6,
    t_systemRespond = 7
};

static const canMember KnownCanMembers[] =
{
    { "DIREKT",             0x000 },
    { "KESSEL",             0x180 },
    { "ATEZ",               0x280 },
    { "BEDIENMODUL",        0x300 },
    { "BEDIENMODUL",        0x301 },
    { "BEDIENMODUL",        0x302 },
    { "BEDIENMODUL",        0x303 },
    { "RAUMFERNFUEHLER",    0x400 },
    { "MANAGER",            0x480 },
    { "HEIZMODUL",          0x500 },
    { "BUSKOPPLER",         0x580 },
    { "MISCHERMODULE",      0x600 },
    { "MISCHERMODULE",      0x601 },
    { "MISCHERMODULE",      0x602 },
    { "MISCHERMODULE",      0x603 },
    { "PC (COMFORTSOFT)",   0x680 },
    { "FREMDGERAET",        0x700 },
    { "DCF-MODUL",          0x780 }
};

static const int KnownCanMembersCount = 18;