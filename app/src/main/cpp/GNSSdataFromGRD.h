/**  @file GNSSdataFromGRD.h
 * Contains the GNSSdataFromGRD class definition.
 * Using the methods provided, GNSS data acquired from ORD and NRD records are stored in RinexData
 * objects (header data, observations data, etc.) to allow further printing in RINEX file formats.
 * ORD (GNSS Observation Raw Data) files contain mainly observation data records captured during
 * the acquisition process.
 * NRD (GNSS Navigation Raw Data) files contain mainly navigation messages data records also captured
 * during the acquisition process.
 * Both contain also records with relevant data from the device or provided by the user to be used in
 * the RINEX files header.
 *
 *Copyright 2018 Francisco Cancillo
 *<p>
 *This file is part of the toRINEX tool.
 *<p>Ver.	|Date	|Reason for change
 *<p>---------------------------------
 *<p>V1.0	|6/2018	|First release, not fully tested due to lack of device providing nav data
 *<p>V1.1   |6/2019 |Changes have been introduced to cope with second frequencies in receiver
 *                  |and to take into account different states in tracking signals of each
 *                  |constellation and satellite. Only states providing unambiguous measurements
 *                  |will be cnsidered.
 */
#ifndef GNSSDATAFROMOSP_H
#define GNSSDATAFROMOSP_H

#include <math.h>
//from CommonClasses
#include "Logger.h"
#include "RinexData.h"
#include "Utilities.h"

//@cond DUMMY
//The type of messages that GNSS Raw Data files or setup arguments can contain
#define MT_EPOCH 1     //Epoch data
#define MT_SATOBS 2    //Satellite observations data
#define MT_SATNAV_GPS_L1_CA 10    //Satellite navigation data from GPS L1 C/A
#define MT_SATNAV_GLONASS_L1_CA 11    //Satellite navigation data from GLONASS L1 C/A
#define MT_SATNAV_GALILEO_INAV 12    //Satellite navigation data from Galileo I/NAV
#define MT_SATNAV_GALILEO_FNAV 13    //Satellite navigation data from Galileo F/NAV
#define MT_SATNAV_BEIDOU_D1 14    //Satellite navigation data from Beidou D1
#define MT_SATNAV_GPS_L5_C 15    //Satellite navigation data from GPS L5 C
#define MT_SATNAV_GPS_C2 16    //Satellite navigation data from GPS C2
#define MT_SATNAV_GPS_L2_C 17    //Satellite navigation data from GPS L2 C
#define MT_SATNAV_BEIDOU_D2 18    //Satellite navigation data from Beidou D2
#define MT_SATNAV_UNKNOWN 40    //Satellite navigation data unknown type
#define MT_GRDVER 50     //Observation or navigation raw data files version
#define MT_PGM 51        //Program used to generate data (toRINEX Vx.x)
#define MT_DVTYPE 52    //Device type
#define MT_DVVER 53     //Device version
#define MT_LLA 54       //Latitude, Longitude, Altitude given by receiver
#define MT_DATE 55       //Date of the file
#define MT_INTERVALMS 56    //measurements interval in milliseconds
#define MT_SIGU 57      //signal strength units
#define MT_RINEXVER 70   //RINEX file version to be generated
#define MT_SITE 71
#define MT_RUN_BY 72
#define MT_MARKER_NAME 73
#define MT_MARKER_TYPE 74
#define MT_OBSERVER 75
#define MT_AGENCY 76
#define MT_RECNUM 77
#define MT_COMMENT 80
#define MT_MARKER_NUM 81
#define MT_CLKOFFS 82
#define MT_FIT 95       //If epoch interval shall fit the interval given or not
#define MT_LOGLEVEL 96
#define MT_CONSTELLATIONS 97
#define MT_SATELLITES 98
#define MT_OBSERVABLES 99
#define MT_LAST 9999
struct MsgType {
    int type;
    string description;
};
const MsgType msgTblTypes[] = {
	{MT_EPOCH, "MT_EPOCH"},
	{MT_SATOBS, "MT_SATOBS"},
	{MT_SATNAV_GPS_L1_CA, "MT_SATNAV_GPS_L1_CA"},
	{MT_SATNAV_GLONASS_L1_CA, "MT_SATNAV_GLONASS_L1_CA"},
	{MT_SATNAV_GALILEO_INAV, "MT_SATNAV_GALILEO_INAV"},
    {MT_SATNAV_GALILEO_FNAV, "MT_SATNAV_GALILEO_FNAV"},
	{MT_SATNAV_BEIDOU_D1, "MT_SATNAV_BEIDOU_D1"},
	{MT_SATNAV_GPS_L5_C, "MT_SATNAV_GPS_L5_C"},
    {MT_SATNAV_GPS_C2, "MT_SATNAV_GPS_C2"},
    {MT_SATNAV_GPS_L2_C, "MT_SATNAV_GPS_L2_C"},
    {MT_SATNAV_BEIDOU_D2, "MT_SATNAV_BEIDOU_D2"},
    {MT_SATNAV_UNKNOWN, "MT_SATNAV_UNKNOWN"},
	{MT_GRDVER, "MT_GRDVER"},
	{MT_PGM, "MT_PGM"},
	{MT_DVTYPE, "MT_DVTYPE"},
	{MT_DVVER, "MT_DVVER"},
	{MT_LLA, "MT_LLA"},
	{MT_DATE, "MT_DATE"},
    {MT_INTERVALMS, "MT_INTERVALMS"},
    {MT_SIGU, "MT_SIGU"},
	{MT_RINEXVER, "MT_RINEXVER"},
	{MT_SITE, "MT_SITE"},
	{MT_RUN_BY, "MT_RUN_BY"},
	{MT_MARKER_NAME, "MT_MARKER_NAME"},
	{MT_MARKER_TYPE, "MT_MARKER_TYPE"},
	{MT_OBSERVER, "MT_OBSERVER"},
	{MT_AGENCY, "MT_AGENCY"},
	{MT_RECNUM, "MT_RECNUM"},
	{MT_COMMENT, "MT_COMMENT"},
	{MT_MARKER_NUM, "MT_MARKER_NUM"},
	{MT_CLKOFFS, "MT_CLKOFFS"},
	{MT_FIT, "MT_FIT"},
	{MT_LOGLEVEL, "MT_LOGLEVEL"},
	{MT_CONSTELLATIONS, "MT_CONSTELLATIONS"},
	{MT_SATELLITES, "MT_SATELLITES"},
	{MT_OBSERVABLES, "MT_OBSERVABLES"},
	{MT_LAST, "UNKNOWN msg type"}
};
//To compute Android states of GNSS measurements
#define ST_UNKNOWN 0
//masks to get unambiguous tracking states
#define ST_CODE_LOCK        0x00001
#define ST_SUBFRAME_SYNC    0x00004
#define ST_TOW_DECODED      0x00008
#define ST_TOW_KNOWN        0x04000
#define ST_2ND_CODE_LOCK    0x10000
#define ST_CBSS_SYNC        0x00027        //code, bit, subframe and symbol bits in ST
#define ST_GLO_STRING_SYNC  0x00040
#define ST_GLO_TOD_DECODED  0x00080
#define ST_GLO_TOD_KNOWN    0x80000
#define ST_BDS_D2_SUBFRAME_SYNC 0x00200
#define ST_GAL_E1C_2ND_CODE_LOCK    0x00800
#define ST_GAL_E1B_PAGE_SYNC        0x01000
#define ST_GAL_E1BC_SYNC            0x00C00 //Galileo E1BC code and E1C2nd code bits in ST
#define ST_CBGSS_SYNC        0x00063    //code, bit, GLONASS string and symbol bits in ST
#define ADR_ST_VALID 0X01
#define ADR_ST_RESET 0x02       //accumulated delta range  reset detected
#define ADR_ST_CYCLE_SLIP 0x04  //accumulated delta range cycle slip detected
#define ADR_ST_HALF_CYCLE_RESOLVED 0x08      //accumulated delta range half cycle resolved
#define ADR_STATE_HALF_CYCLE_REPORTED 0x10
//Constants usefull for computations
const double SPEED_OF_LIGTH_MxNS = 299792458.0 * 1E-9;    //in m/nanosec
const double DOPPLER_FACTOR = 1E6 / 299792458.0;    //in Mm/sec
const double WLFACTOR = 1.0E6 / 299792458.0;
/*
const double C1CADJ = 299792458.0;	//to adjust C1C (pseudorrange L1 in meters) = C1CADJ (the speed of light) * clkOff
const double L1CADJ = 1575420000.0;	//to adjust L1C (carrier phase in cycles) =  L1CADJ (L1 carrier frequency) * clkOff
*/
const double ThisPI = 3.1415926535898;
const long long NUMBER_NANOSECONDS_DAY = 24LL * 60LL * 60LL * 1000000000LL;
const long long NUMBER_NANOSECONDS_WEEK = 7LL * NUMBER_NANOSECONDS_DAY;
const long long NUMBER_NANOSECONDS_3H =     3LL * 60LL * 60LL * 1000000000LL;
const long long NUMBER_NANOSECONDS_2S =     2LL * 1000000000LL;
const long long NUMBER_NANOSECONDS_6S =     6LL * 1000000000LL;
const long long NUMBER_NANOSECONDS_14S =    14LL * 1000000000LL;
const long long NUMBER_NANOSECONDS_18S =    18LL * 1000000000LL;
const long long NUMBER_NANOSECONDS_100MS =  100LL * 1000000LL;
const double ECEF_A = 6378137.0;			//WGS-84 semi-major axis
const double ECEF_E2 = 6.69437999014e-3;	//WGS-84 first eccentricity squared
const double dgrToRads = ThisPI / 180.0;    //a factor to convert degrees to radiands
//Log messages
const string LOG_MSG_PARERR("Params error");
const string LOG_MSG_ERROPEN("Error opening GRD file ");
const string LOG_MSG_NINO("SATNAV record in OBS file");
const string LOG_MSG_NONI("SATOBS record in NAV file");
const string LOG_MSG_ERRO("Error reading ORD: ");
const string LOG_MSG_INVM(" ignored invalid measurements");
const string LOG_MSG_UNK(" ignored unknown measurements or GLO FCN");
const string LOG_MSG_INMP("Incorrect nav message parameters");
const string LOG_MSG_OSIZ(" or size");
const string LOG_MSG_NAVIG(". Ignored");
const string LOG_MSG_UNKSELSYS("Unknown selected sys ");
const string MSG_SPACE(" ");
const string MSG_COMMA(", ");
const string MSG_SLASH("/");
const string MSG_COLON(": ");
const string MSG_NOT_IMPL("NOT IMPLEMENTED");
//const string defaultSiteName = "PNT_";
///GPS constants related to navigation messages
#define GPS_L1_CA_MSGSIZE 40
#define GPS_SUBFRWORDS 10
#define GPS_MAXSUBFRS 5
//GPS prn are in the range 1-32
#define GPS_MINPRN 1
#define GPS_MAXPRN 32
#define GPS_MAXSATELLITES 32
///GLONASS constants related to navigation messages
//Note: GLO_STRWORDS * 4 shall be > GLO_L1_CA_MSGSIZE
#define GLO_L1_CA_MSGSIZE 11
#define GLO_STRWORDS 3
#define GLO_MAXSTRS 5
//Note: GLO satellite number ranges are:
// 1-24 if Orbital Slot Number (OSN) given, or
// 93-106 if Frequency Channel Number (FCN) given
#define GLO_MINOSN 1
#define GLO_MAXOSN 24
#define GLO_MINFCN 93
#define GLO_MAXFCN 106
#define GLO_MAXSATELLITES 38
const double GLO_BAND_FRQ1 = 1602.0; //L1 band cnetral frequency
const double GLO_SLOT_FRQ1 = 0.5625; //L1 frequency factor frq = 1602 + fcn * GLO_SLOT_FRQ in MHz
const double GLO_BAND_FRQ2 = 1246.0; //L2 band cnetral frequency
const double GLO_SLOT_FRQ2 = 0.4375; //L2 frequency factor frq = 1246 + fcn * GLO_SLOT_FRQ in MHz
//An offset to translate FCN 93-106 to 25-38
#define GLO_FCN2OSN (GLO_MINFCN - GLO_MAXOSN - 1)
///SBAS constants related to navigation message
//SBAS prn are in the range 120-151 and 183-192
//TBD
///GALILEO constants related to navigation message
//GALILEO prn are in the range 1-36
//TBD
///BEIDOU constants related to navigation message
//BEIDOU prn are in the range 1-37
//TBD
///QZSS constants related to navigation message
//QZSS prn are in the range 193-200
//TBD
//@endcond

/**GNSSdataFromOSP class defines data and methods used to acquire RINEX header and epoch data from a ORD file containing data acquired.
 * Such header and epoch data can be used to generate and print RINEX files.
 *<p>
 * A program using GNSSdataFromGRD would perform the following steps:
 *	-# Declare a GNSSdataFromGRD object stating the receiver, the file name with the raw data, and optionally the logger to be used
 *	-# Collect header data and save them into an object of RinexData class using collectHeaderData methods
 *	-# Header data acquired can be used to generate / print RINEX or RTK files (see available methods in RinexData and RTKobservation classes for that)
 *	-# As header data may be sparse among the raw data file, rewind it before performing any other data acquisition
 *	-# Acquire an epoch data and save them into an object of RinexData class using acqEpochData methods
 *	-# Epoch data acquired can be used to generate / print RINEX file epoch (see available methods in RinexData classes)
 *	-# Repeat above steps 5 and 6 while epoch data are available in the input file.
 *<p>
 * This version implements processing of ...
 * Each ORD message starts with ...
 */
class GNSSdataFromGRD {
public:
    GNSSdataFromGRD(Logger*);
    GNSSdataFromGRD();
    ~GNSSdataFromGRD(void);
    bool openInputGRD(string, string);
    void rewindInputGRD();
    void closeInputGRD();
    bool collectHeaderData(RinexData &, int, int);
    bool collectEpochObsData(RinexData &);
    bool collectNavData(RinexData &);
    void processHdData(RinexData &, int, string);
    void processFilterData(RinexData &);
    string getMsgDescription(int );

private:
    FILE* grdFile;  //GNSS raw data file
    int msgCount;   //a counter of messages read from the file
    struct GNSSsystem {	//Defines data for each GNSS system that can provide data to the RINEX file. Used for all versions
        char sysId;	//system identification: G (GPS), R (GLONASS), S (SBAS), E (Galileo). See RINEX V302 document: 3.5 Satellite numbers
        vector <string> obsType;	//identifier of each obsType type: C1C, L1C, D1C, S1C... (see RINEX V302 document: 5.1 Observation codes)
        //constructor
        GNSSsystem (char sys, const vector<string> &obsT) {
            sysId = sys;
            obsType.insert(obsType.end(), obsT.begin(), obsT.end());
        };
    };
    vector <GNSSsystem> systems;
	bool fitInterval;
    int clkoffset;      //0: obs not corrected; 1: obs corrected
	bool applyBias;		//true if clkoffet = 1: apply clock bias to pseudoranges and print it in epoch clock record
    vector<string> selSatellites;
    vector<string> selObservables;
	int clockDiscontinuityCount;
	//Data structures to capture GPS navigation messages
	struct GPSSubframeData {
		bool hasData;
		unsigned int words[GPS_SUBFRWORDS];
	};
	struct GPSFrameData {
		bool hasData;
		GPSSubframeData gpsSatSubframes[GPS_MAXSUBFRS];
	};
	GPSFrameData gpsSatFrame[GPS_MAXSATELLITES];
	//Data structures to capture GLONASS navigation messages
	struct GLOStrData {
		bool hasData;
		unsigned int words[GLO_STRWORDS];
	};
	struct GLOFrameData {
		bool hasData;
		GLOStrData gloSatStrings[GLO_MAXSTRS];
	};
	GLOFrameData gloSatFrame[GLO_MAXSATELLITES];
	//a table to stablish OSN (Orbital Slot Number) - FCN (Frequency Channel Number)
    //OSN are in the range 1 to 24. FCN are in the range -7 to + 6
    //Values are extracted directly from almanac data.
    //Also can be derived from measurements: OSN is satNum (if < 24) and FCN can be computed from the carrier frequency value.
    //Android gives as satellite number values 100 + FCN when OSN is unknown
    struct GLONASSfreq {	//A type to match GLONASS slots and carrier frequency numbers
        int nA;				//The slot number (n in Almanac strings 6, 8, 10, 12, 14)
        int strFhnA;		//The string number from where the carrier frequency number must be extracted
        int hnA;            //the carrier frequency number (Hn in Almanac strings 7, 9, 11, 13, 15)
    };
	GLONASSfreq nAhnA[GLO_MAXSATELLITES];	//for each channel, the nA and frequency data
    //TODO check if this varible shall be finally removed in Android
    //the carrier frequency number (Hn in Almanac strings 7, 9, 11, 13, 15), or computed from observationa
    //	int frqNum[GLO_MAXSATELLITES];
    //Constant data used to convert GPS broadcast navigation data to "true" values
    double GPS_SCALEFACTOR[8][4];	//the scale factors to apply to GPS broadcast orbit data to obtain ephemeris (see GPS ICD)
    double GPS_URA[16];			    //the User Range Accuracy values corresponding to URA index in the GPS SV broadcast data (see GPS ICD)
    double GLO_SCALEFACTOR[4][4];	//the scale factors to apply to GLONASS broadcast orbit data to obtain ephemeris (see GLONASS ICD)
    //Logger
    Logger* plog;		//the place to send logging messages
    bool dynamicLog;	//true when created dynamically here, false when provided externally

    void setInitValues();
    bool extractGPSEphemeris(int sv, int (&bom)[8][4]);
    double scaleGPSEphemeris(int (&bom)[8][4], double (&bo)[8][4]);
    bool collectGPSL1CAEpochNav(RinexData &rinex, int msgType);
    bool collectGLOL1CAEpochNav(RinexData &rinex);
    bool extractGLOEphemeris(int sat, int (&bom)[8][4], int& slot);
    double scaleGLOEphemeris(int (&bom)[8][4], double (&bo)[8][4]);
	bool readGLOL1CAEpochNav(char &constId, int &satNum, int &strnum, int &frame, unsigned int wd[]);
	int gloSatIdx(int);

    bool addSignal(char system, string signal);
    void skipToEOM();
    void setHdSys(RinexData &);
    void trimBuffer(char*, const char*);
    void llaTOxyz( const double, const double, const double, double &, double &, double &);
    double collectAndSetEpochTime(RinexData& rinex, double& tow, int& numObs, string msg);
    vector<string> getElements(string, string);
    bool isPsAmbiguous(char constellId, char* signalId, int synchState, double tRx, double &tRxGNSS, long long &tTx);
    bool isCarrierPhInvalid (char constellId, char* signalId, int carrierPhaseState);
    bool isKnownMeasur(char constellId, int satNum, char frqId, char attribute);
    int gloNumFromFCN(int satNum, char band, double carrFrq, bool updTbl);
    };
#endif
