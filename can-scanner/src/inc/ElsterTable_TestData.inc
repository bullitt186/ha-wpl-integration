
// Achtung: Die Tabelle befindet sich im Aufbau! Nicht alle Indizes sind korrekt.
//
// Die Indizes muessen zwingend vor einem produktiven Einsatz ueberprueft werden.
// Die beste Ueberpruefung erhaelt man mit der ComfortSoft und der Simulation
// der eigenen WP. Da werden die Telegramme der Schnittstelle protokolliert.  
//
// Die Indizes, die ich mittels ComfortSoft zu meiner WP ueberpruefen konnte,
// sind korrekt. Da ich eine WPL33 ohne PV nutze, gibt es viele Indizes, die Ihr
// selbst ueberpruefen muesst.

static const ElsterIndex ElsterTable[] =
{
//  Name                                                 Index   Type
//  Struktur-Definition in KElsterTable.h
  { "FEHLERMELDUNG"                                    , 0x0001, 0},
  { "WASSERDRUCK"                                      , 0x001f, 0},
  { "STUETZSTELLE_MD1"                                 , 0x0915, 0},
  { "SOFTWARE_SIMULATION_18"                           , 0xc363, 0}
};

static const ErrorIndex ErrorList[] =
{
  { 0x0002, "Schuetz klebt"},
  { 0x0003, "ERR HD-SENSOR"},
  { 0x0004, "Hochdruck"},
  { 0x0005, "Verdampferfuehler"},
  { 0x0006, "Relaistreiber"},
  { 0x0007, "Relaispegel"},
  { 0x0008, "Hexschalter"},
  { 0x0009, "Drehzahl Luefter"},
  { 0x000a, "Lueftertreiber"},
  { 0x000b, "Reset Baustein"},
  { 0x000c, "ND"},
  { 0x000d, "ROM"},
  { 0x000e, "QUELLEN MINTEMP"},
  { 0x0010, "Abtauen"},
  { 0x0012, "ERR T-HEI IWS"},
  { 0x0017, "ERR T-FRO IWS"},
  { 0x001a, "Niederdruck"},
  { 0x001b, "ERR ND-DRUCK"},
  { 0x001c, "ERR HD-DRUCK"},
  { 0x001d, "HD-SENSOR-MAX"},
  { 0x001e, "HEISSGAS-MAX"},
  { 0x001f, "ERR HD-SENSOR"},
  { 0x0020, "Einfrierschutz"},
  { 0x0021, "KEINE LEISTUNG"}
};



