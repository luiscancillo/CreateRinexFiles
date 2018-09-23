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
#define MT_SATNAV_GPS_l1_CA 10    //Satellite navigation data from GPS L1 C/A
#define MT_SATNAV_GLONASS_L1_CA 11    //Satellite navigation data from GLONASS L1 C/A
#define MT_MT_SATNAV_GALILEO_FNAV 12    //Satellite navigation data from Galileo F/NAV
#define MT_SATNAV_BEIDOU_D1 13    //Satellite navigation data from Beidou D1 & D2
#define MT_GRDVER 50     //Observation or navigation raw data files version
#define MT_PGM 51        //Program used to generate data (toRINEX Vx.x)
#define MT_DVTYPE 52    //Device type
#define MT_DVVER 53     //Device version
#define MT_LLA 54       //Latitude, Longitude, Altitude given by receiver
#define MT_DATE 55       //Date of the file
#define MT_INTERVALMS 56    //measurements interval in milliseconds
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
	{MT_SATNAV_GPS_l1_CA, "MT_SATNAV_GPS_l1_CA"},
	{MT_SATNAV_GLONASS_L1_CA, "MT_SATNAV_GLONASS_L1_CA"},
	{MT_MT_SATNAV_GALILEO_FNAV, "MT_MT_SATNAV_GALILEO_FNAV"},
	{MT_SATNAV_BEIDOU_D1, "MT_SATNAV_BEIDOU_D1"},
	{MT_GRDVER, "MT_GRDVER"},
	{MT_PGM, "MT_PGM"},
	{MT_DVTYPE, "MT_DVTYPE"},
	{MT_DVVER, "MT_DVVER"},
	{MT_LLA, "MT_LLA"},
	{MT_DATE, "MT_DATE"},
    {MT_INTERVALMS, "MT_INTERVALMS"},
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
//Constants usefull for computations
const double SPEED_OF_LIGTH_MxNS = 299792458.0 * 1E-9;    //in m/nanosec
const double DOPPLER_FACTOR = 1E6 / 299792458.0;    //in Mm/sec
/*
const double C1CADJ = 299792458.0;	//to adjust C1C (pseudorrange L1 in meters) = C1CADJ (the speed of light) * clkOff
const double L1CADJ = 1575420000.0;	//to adjust L1C (carrier phase in cycles) =  L1CADJ (L1 carrier frequency) * clkOff
const double L1WLINV = 1575420000.0 / 299792458.0; //the inverse of L1 wave length to convert m/s to Hz.
*/
const double ThisPI = 3.1415926535898;

const long long NUMBER_NANOSECONDS_DAY = 24LL * 60LL * 60LL * 1000000000LL;
const long long NUMBER_NANOSECONDS_WEEK = 7LL * NUMBER_NANOSECONDS_DAY;

const double ECEF_A = 6378137.0;               //WGS-84 semi-major axis
const double ECEF_E2 = 6.6943799901377997e-3;  //WGS-84 first eccentricity squared
/*
const double a1 = 4.2697672707157535e+4;  //a1 = a*e2
const double a2 = 1.8230912546075455e+9;  //a2 = a1*a1
const double a3 = 1.4291722289812413e+2;  //a3 = a1*e2/2
const double a4 = 4.5577281365188637e+9;  //a4 = 2.5*a2
const double a5 = 4.2840589930055659e+4;  //a5 = a1+a3
const double a6 = 9.9330562000986220e-1;  //a6 = 1-e2
*/
const double dgrToRads = atan(1) / 45.0;   // = PI / 180 a factor to convert geo degrees to radiands
//Log messages
const string LOG_MSG_ERROPEN("Error opening GRD file ");
const string LOG_MSG_NINO("SATNAV record in OBS file");
const string LOG_MSG_NONI("SATOBS record in NAV file");
const string LOG_MSG_ERRO("Error reading ORD: ");
const string LOG_MSG_ONMS("Incorrect nav message parameters or size");
const string LOG_MSG_NAVIG(". Navigation message ignored");
const string LOG_MSG_MT_SATNAV_GPS_l1_CA(" MT_SATNAV_GPS_l1_CA data");
const string LOG_MSG_UNKSELSYS("Unknown selected sys ");
const string LOG_MSG_MT_UNK("MT_ unknown:");
const string LOG_MSG_MT_SATNAV_GLONASS_L1_CA(" MT_SATNAV_GLONASS_L1_CA data");
const string MSG_SPACE(" ");
const string MSG_COMMA(",");
const string MSG_SLASH("/");
const string MSG_COLON(": ");
const string defaultSiteName = "PNT_";
///GPS constants related to navigation messages
#define GPS_l1_CA_MSGSIZE 40
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
//An offset to translate FCN 93-106 to 25-38
#define GLO_OSNOFFSET 68
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
    bool collectHeaderData(RinexData &);
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
    string receiver;
	string siteName;
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
    struct GLONASSfreq {	//A type to match GLONASS slots and carrier frequency numbers received in almanac strings
        int nA;				//The slot number for a set of almanac data
        int strFhnA;		//The string number from where the carrier frequency number must be extracted,
    };
	GLONASSfreq nAhnA[GLO_MAXSATELLITES];	//for each channel, the nA and frequency data
	int carrierFreq[GLO_MAXSATELLITES];
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
    bool collectGPSEpochNav(RinexData &rinex);
    bool collectGLOEpochNav(RinexData &rinex);
    bool extractGLOEphemeris(int sat, int (&bom)[8][4], int& slot);
    double scaleGLOEphemeris(int (&bom)[8][4], double (&bo)[8][4]);
	bool readGLOEpochNav(char &constId, int &satNum, int &strnum, int &frame, unsigned int wd[]);
	int gloSatIdx(int);

    bool addSignal(char system, string signal);
    void skipToEOM();
    void setHdSys(RinexData &);
    void trimBuffer(char*, const char*);
    void llaTOxyz( const double, const double, const double, double &, double &, double &);
    long long getAndSetEpochTime(RinexData& rinex, double& tow, int& numObs, string msg);
    vector<string> getElements(string, string);
};
#endif
