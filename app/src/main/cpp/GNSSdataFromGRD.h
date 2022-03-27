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
 *Copyright 2015, 2021 by Francisco Cancillo & Luis Cancillo
 *<p>
 *This file is part of the toRINEX APP.
 *<p>Ver.	|Date	|Reason for change
 *<p>---------------------------------
 *<p>V1.0	|6/2018	|First release, not fully tested due to lack of device providing nav data
 *<p>V1.1   |6/2019 |Changes have been introduced to cope with second frequencies in receiver
 *                  |and to take into account different states in tracking signals of each
 *                  |constellation and satellite. Only states providing unambiguous measurements
 *                  |will be cnsidered.
 * <p>V1.2  |11/2019|Added the functionality to extract iono and time corrections to be included in RINEX header
 */
#ifndef GNSSDATAFROMOSP_H
#define GNSSDATAFROMOSP_H

#include <math.h>
//from CommonClasses
#include "Logger.h"
#include "RinexData.h"
#include "Utilities.h"

//@cond DUMMY
//To identify GRD file types
const string ORD_FILE_EXTENSION  = ".ORD";
const string NRD_FILE_EXTENSION  = ".NRD";
const int MIN_ORD_FILE_VERSION = 2;
const int MAX_ORD_FILE_VERSION = 2;
const int MIN_NRD_FILE_VERSION = 2;
const int MAX_NRD_FILE_VERSION = 2;

//The type of messages that GNSS Raw Data files or setup arguments can contain
#define MT_EPOCH 1     //Epoch data
#define MT_SATOBS 2    //Satellite observations data
#define MT_SATNAV_GPS_L1_CA 3    //Satellite navigation data from GPS L1 C/A
#define MT_SATNAV_GLONASS_L1_CA 4    //Satellite navigation data from GLONASS L1 C/A
#define MT_SATNAV_GALILEO_INAV 5    //Satellite navigation data from Galileo I/NAV
#define MT_SATNAV_GALILEO_FNAV 6    //Satellite navigation data from Galileo F/NAV
#define MT_SATNAV_BEIDOU_D1 7    //Satellite navigation data from Beidou D1
#define MT_SATNAV_GPS_L5_C 8    //Satellite navigation data from GPS L5 C
#define MT_SATNAV_GPS_C2 9    //Satellite navigation data from GPS C2
#define MT_SATNAV_GPS_L2_C 10    //Satellite navigation data from GPS L2 C
#define MT_SATNAV_BEIDOU_D2 11    //Satellite navigation data from Beidou D2
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
//Constants useful for computations
const double SPEED_OF_LIGTH_MxNS = 299792458.0 * 1E-9;    //in m/nanosec
const double DOPPLER_FACTOR = 1E6 / 299792458.0;    //in Mm/sec
const double WLFACTOR = 1.0E6 / 299792458.0;
const int MASK8b = 0xFF;    //a bit mask for 8 bits
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
const string LOG_MSG_INVM(" measurement ignored, invalid tracking or carrier phase state");
const string LOG_MSG_UNK(" ignored, wrong satellite or signal identification");
const string LOG_MSG_INMP("Invalid nav message parameters");
const string LOG_MSG_CORR(" Corrections completed.");
const string LOG_MSG_FRM(" Frame completed.");
const string LOG_MSG_SFR(" Subframe saved.");
const string LOG_MSG_IOD(" IODs match.");
const string LOG_MSG_OSIZ(" or size");
const string LOG_MSG_NAVIG(". Ignored");
const string LOG_MSG_UNKSELSYS("Unknown selected sys ");
const string LOG_MSG_SATDIF(" Embedded sat num differs: ");
const string LOG_MSG_WTDIF(" Embedded word type differs: ");
const string MSG_SPACE(" ");
const string MSG_COMMA(", ");
const string MSG_SLASH("/");
const string MSG_COLON(": ");
const string MSG_NOT_IMPL("NOT IMPLEMENTED");

///GPS definitions related to navigation messages
#define GPS_L1_CA_MSGSIZE 40
#define GPS_SUBFRWORDS 10
#define GPS_MAXSUBFRS 4
//GPS prn are in the range 1-32
#define GPS_MINPRN 1
#define GPS_MAXPRN 32
#define GPS_MAXSATELLITES 32
///GLONASS definitions related to navigation messages
//Note: GLO_STRWORDS * 4 shall be > GLO_L1_CA_MSGSIZE
#define GLO_L1_CA_MSGSIZE 11
#define GLO_STRWORDS 3
#define GLO_MAXSTRS 5
//Note: GLO satellite number ranges are:
// 1-24 if Orbital Slot Number (OSN) is given, or
// 93-106 if Frequency Channel Number (FCN) is given
#define GLO_MINOSN 1
#define GLO_MAXOSN 24
#define GLO_MINFCN 93
#define GLO_MAXFCN 106
#define GLO_MAXSATELLITES GLO_MAXOSN + (GLO_MAXFCN - GLO_MINFCN + 1)
const double GLO_BAND_FRQ1 = 1602.0; //L1 band cnetral frequency
const double GLO_SLOT_FRQ1 = 0.5625; //L1 frequency factor frq = 1602 + fcn * GLO_SLOT_FRQ in MHz
const double GLO_BAND_FRQ2 = 1246.0; //L2 band cnetral frequency
const double GLO_SLOT_FRQ2 = 0.4375; //L2 frequency factor frq = 1246 + fcn * GLO_SLOT_FRQ in MHz
//An offset to translate FCN 93-106 to an index in the range 24-37
#define GLO_FCN2OSN (GLO_MINFCN - GLO_MAXOSN)
///GALILEO constants related to navigation message
//GALILEO prn are in the range 1-36
//For Galileo I/NAV, each page contains 2 page parts, even and odd, with a total of 2x114 = 228 bits, (sync & tail excluded)
// that should be fit into 29 bytes, with MSB first (skip B229-B232)
#define GALINAV_MSGSIZE 29  //size of message passed by Android
//the 228 bits are: 1 e/o + 1 pt + 112 data 1/2 + 1 e/o + 1 pt + 16 data 2/2 + 64 reserved 1 + 24 CRC + 8 reserved 2
#define GALINAV_DATAW 4     //size in 32 bit unsig. int to store the 128 bits of a message word
#define GALINAV_MAXWORDS 10  //message type 1 to 10 of any I/NAV subframe contain ephemeris data
//GAL prn are in the range 1-32
#define GAL_MINPRN 1
#define GAL_MAXPRN 36
#define GAL_MAXSATELLITES 36
///BEIDOU constants related to navigation message
//BDS prn are in the range 1-37
//BDS definitions related to navigation messages
#define BDSD1_MSGSIZE 40
#define BDSD1_SUBFRWORDS 10
#define BDSD1_MAXSUBFRS 5
#define BDS_MINPRN 1
#define BDS_MAXPRN 37
#define BDS_MAXSATELLITES 37
///SBAS constants related to navigation message
//SBAS prn are in the range 120-151 and 183-192
#define SBAS_MINPRN 120
#define SBAS_MAXPRN 192
///QZSS constants related to navigation message
//QZSS prn are in the range 193 - 200
#define QZSS_MINPRN 193
#define QZSS_MAXPRN 200
///IRNSS constants related to navigation message
//IRNSS prn are in the range 1 - 14
//#define IRNSS_MINPRN 193
//#define IRNSS_MAXPRN 200
//@endcond

/**GNSSdataFromOSP class defines data and methods used to acquire RINEX header and epoch data from a
 * ORD file containing data acquired.
 * Such header and epoch data can be used to generate and print RINEX files.
 *<p>
 * A program using GNSSdataFromGRD would perform the following steps:
 *	-# Declare a GNSSdataFromGRD object stating the receiver, the file name with the raw data, and
 *		optionally the logger to be used
 *	-# Collect header data and save them into an object of RinexData class using collectHeaderData methods
 *	-# Header data acquired can be used to generate / print RINEX or RTK files (see available methods
 *		in RinexData and RTKobservation classes for that)
 *	-# As header data may be sparse among the raw data file, rewind it before performing any other data acquisition
 *	-# Acquire an epoch data and save them into an object of RinexData class using acqEpochData methods
 *	-# Epoch data acquired can be used to generate / print RINEX file epoch (see available methods in
 *		RinexData classes)
 *	-# Repeat above steps 5 and 6 while epoch data are available in the input file.
 *<p>
 * This version implements processing of ...TODO
 * Each ORD message starts with ...TODO
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
    bool processHdData(RinexData &, int, string);
    void processFilterData(RinexData &);
    string getMsgDescription(int );
    int getMsgType(string );

private:
    FILE* grdFile;  //GNSS raw data file
	int ordVersion;	//GNSS observation raw data version
	int nrdVersion;	//GNSS navigation raw data version
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
        uint32_t words[GPS_SUBFRWORDS];
	};
	struct GPSFrameData {
		bool hasData;
		GPSSubframeData gpsSatSubframes[GPS_MAXSUBFRS];
	};
	GPSFrameData gpsSatFrame[GPS_MAXSATELLITES];
	//number of GPS weeks roll over
	int nGPSrollOver;
	//Data structures to capture GLONASS navigation messages
	struct GLOStrData {
		bool hasData;
		uint32_t words[GLO_STRWORDS];
	};
	struct GLOFrameData {
		int frmNum;
		GLOStrData gloSatStrings[GLO_MAXSTRS];
	};
	GLOFrameData gloSatFrame[GLO_MAXSATELLITES];
	//A table to stablish OSN (Orbital Slot Number) - FCN (Frequency Channel Number) for GLONASS satellites
    //OSN values are in the range 1 to 24. FCN are in the range -7 to + 6
    //Values are extracted directly from string 4 (OSN), from observation carrier frequency data (FCN),
    //or from almanac data (OSN and FCN).
    //When the Android satellite numbers is in the range 1 to 24, this number is the OSN
    //Android gives as satellite number the value 100 + FCN when its OSN is unknown.
    //The index in the table is:
    // - if satellite number is in the range 1 to 24, the own satellite number - 1
    // - if satellite number is in the range 93 to 106, satellite number - 69 (values 24 to 37)
    struct GLONASSosnfcn {
        int osn;    //the OSN
        int fcn;    //the FCN
        bool fcnSet;    //FCN values have been set
    };
    GLONASSosnfcn glonassOSN_FCN[GLO_MAXSATELLITES];
    //A table to store data from GLONASS almanac on the nA (is the OSN) and HnA (is the FCN)
    //nA-1 is the index in the table. It is extracted from strings 6, 8, 10, 12 or 14
    //When and HnA value is known, it is translated to the above tableOSNFCN
    struct GLONASSfreq {	//A type to match GLONASS slots and carrier frequency numbers
        int satNum;      //the satellite number source of data
        int strFhnA;	//The string number from where the carrier frequency number for this nA must be extracted
        int frmFhnA;    //The frame number for the above string number
    };
	GLONASSfreq nAhnA[GLO_MAXOSN];
    //Data structures to capture Galileo I/NAV navigation messages
    struct GALINAVpageData {
        bool hasData;
        uint32_t data[GALINAV_DATAW];
    };
    struct GALINAVFrameData {
        bool hasData;
        GALINAVpageData pageWord[GALINAV_MAXWORDS];
    };
    GALINAVFrameData galInavSatFrame[GAL_MAXSATELLITES];
    //number of GAL weeks roll over
    int nGALrollOver;
    //Data structures to capture BDS navigation messages
    struct BDSD1SubframeData {
        bool hasData;
        unsigned int words[BDSD1_SUBFRWORDS];
    };
    struct BDSD1FrameData {
        bool hasData;
        //index for subframes:0 is subframe 1; 1 is subfr 2; 2 is subfr 3; 4 is subfr 5 page 9; 5 is subfr 5 page 10
        BDSD1SubframeData bdsSatSubframes[BDSD1_MAXSUBFRS];
    };
    BDSD1FrameData bdsSatFrame[BDS_MAXSATELLITES];
    //number of BDS weeks roll over
    int nBDSrollOver;
    //Constant data used to convert to RINEX broadcast orbit ephemeris values the broadcast orbit navigation data from satellite messages which contains only mantissas
    //Note: BO_xxxx constant values defined in RinexData.h
    double GPS_SCALEFACTOR[BO_LINSTOTAL][BO_MAXCOLS];	//the scale factors to apply to GPS broadcast orbit data to obtain ephemeris (see GPS ICD)
    double GLO_SCALEFACTOR[BO_LINSTOTAL][BO_MAXCOLS];	//the scale factors to apply to GLONASS broadcast orbit data to obtain ephemeris (see GLONASS ICD)
    double GAL_SCALEFACTOR[BO_LINSTOTAL][BO_MAXCOLS];	//the scale factors to apply to Galileo broadcast orbit data to obtain ephemeris (see Galileo OS ICD)
    double BDS_SCALEFACTOR[BO_LINSTOTAL][BO_MAXCOLS];	//the scale factors to apply to BDS broadcast orbit data to obtain ephemeris (see BDS ICD)
    double GPS_URA[16];			    //the User Range Accuracy values corresponding to URA index in the GPS SV broadcast data (see GPS ICD)
    double BDS_URA[16];			    //the User Range Accuracy values corresponding to URA index in the BDS SV broadcast data (see BDS ICD)
    //Logger
    Logger* plog;		//the place to send logging messages
    bool dynamicLog;	//true when created dynamically here, false when provided externally
    void setInitValues();

    bool collectGPSL1CAEphemeris(RinexData &rinex, int msgType);
    bool collectGPSL1CACorrections(RinexData &rinex, int msgType);
    bool readGPSL1CANavMsg(char &constId, int &satNum, int &strnum, int &frame, string &logMsg);
    void extractGPSL1CAEphemeris(int sv, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS]);
    double scaleGPSEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]);

    bool collectGLOL1CAEphemeris(RinexData &rinex, int msgType);
    bool collectGLOL1CACorrections(RinexData &rinex, int msgType);
    bool readGLOL1CANavMsg(char &constId, int &satNum, int &satIdx, int &strnum, int &frame, string &logMsg);
    void extractGLOL1CAEphemeris(int sat, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], int& slot);
    double scaleGLOEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]);
    int gloSatIdx(int);
    int gloOSN(int satNum, char band = '1', double carrFrq = 0.0, bool updTbl = false);

    bool collectGALINEphemeris(RinexData &rinex, int msgType);
    bool collectGALINCorrections(RinexData &rinex, int msgType);
	bool readGALINNavMsg(char &constId, int &satNum, int &strnum, int &frame, string &logMsg);
	void extractGALINEphemeris(int sv, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS]);
    double scaleGALEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]);

    bool collectBDSD1Ephemeris(RinexData &rinex, int msgType);
    bool collectBDSD1Corrections(RinexData &rinex, int msgType);
    bool readBDSD1NavMsg(char &constId, int &satNum, int &strnum, int &frame, string &logMsg);
    void extractBDSD1Ephemeris(int sv, int (&bom)[BO_LINSTOTAL][BO_MAXCOLS]);
    double scaleBDSEphemeris(int (&bom)[BO_LINSTOTAL][BO_MAXCOLS], double (&bo)[BO_LINSTOTAL][BO_MAXCOLS]);

    bool isGoodGRDver(string extension, int version);
    bool addSignal(char system, string signal);
    void skipToEOM();
    void setHdSys(RinexData &);
    bool trimBuffer(char*, const char*);
    void llaTOxyz( const double, const double, const double, double &, double &, double &);
    double collectAndSetEpochTime(RinexData& rinex, double& tow, int& numObs, string msg);
    vector<string> getElements(string, string);
    bool isPsAmbiguous(char constellId, char* signalId, int synchState, double tRx, double &tRxGNSS, long long &tTx);
    bool isCarrierPhInvalid (char constellId, char* signalId, int carrierPhaseState);
    bool isKnownMeasur(char constellId, int satNum, char frqId, char attribute);
    uint32_t getBits(uint32_t *stream, int bitpos, int len);
    };
#endif
