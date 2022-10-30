

struct canMember
{
    char name[20];
    unsigned short id;
};

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